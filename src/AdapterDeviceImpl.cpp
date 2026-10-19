// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AdapterDeviceImpl.h"

#include "AdapterDeviceClass.h"
#include "AttributeProperty.h"
#include "TangoAdapter.h"
#include "TangoPropertyHelper.h"

#include <ChimeraTK/Utilities.h>

#include <AttributeProperty.h>

namespace detail {
  using namespace TangoAdapter;
  template<typename TangoType, typename AdapterType>
  void writeInitialSpectrumValue(Tango::DeviceImpl* device, const std::string& memoriedValue,
      Tango::WAttribute& writeAttribute, Tango::Attr& attr) {
    // StringToArray will create a std::vector<bool> which is an optimised version of std::vector
    // But Tango needs a plain array of bool, so we have to convert it here.
    if constexpr(std::is_same_v<Tango::DevBoolean, TangoType>) {
      auto values = stringToArray<TangoType>(memoriedValue);
      // No lint, we cannot use std::array here because we do not know the size at compile time
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<Tango::DevBoolean[]>(values.size());
      std::copy(values.begin(), values.end(), data.get());

      // We can pass in the unique pointer here, set_write_value will copy the data
      writeAttribute.set_write_value(data.get(), values.size());
    }
    else if constexpr(std::is_same_v<Tango::DevString, TangoType>) {
      auto values = stringToArray<std::string>(memoriedValue);
      // No lint, we cannot use std::array here because we do not know the size at compile time
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<Tango::DevString[]>(values.size());
      std::transform(values.begin(), values.end(), data.get(), [&](auto& v) {
        // NOLINTNEXTLINE
        return const_cast<char*>(v.c_str());
      });

      // We can pass in the unique pointer and c_str() here, set_write_value will copy the data
      // Lint: long is required by the Tango API
      // NOLINTNEXTLINE(google-runtime-int)
      writeAttribute.set_write_value(data.get(), static_cast<long>(values.size()));
    }
    else {
      auto values = stringToArray<TangoType>(memoriedValue);
      writeAttribute.set_write_value(values.data(), values.size());
    }

    attr.write(device, writeAttribute);
  }

  /********************************************************************************************************************/

  // Pushes one scalar or a spectrum worth of samples with the given quality, mirroring the read path of the
  // ScalarAttribTempl/SpectrumAttribTempl classes. Lives here because push_change_event is a type template while the
  // updater callback is not.
  template<typename TangoType, typename AdapterType>
  void pushChangeEventForAttribute(Tango::DeviceImpl* device, const std::string& attrName,
      const AttributeProperty& attr, ChimeraTK::TransferElementAbstractor& pv) {
    auto accessor =
        boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<AdapterType>>(pv.getHighLevelImplElement());
    if(!accessor) {
      DEV_WARN_STREAM(device) << "pv type mismatch for attribute " << attrName << std::endl;
      return;
    }

    auto quality = accessor->dataValidity() == ChimeraTK::DataValidity::ok ? Tango::AttrQuality::ATTR_VALID :
                                                                             Tango::AttrQuality::ATTR_INVALID;
    auto timestamp = Tango::TangoTimestamp(std::chrono::system_clock::now());

    if(attr.attrDataFormat == AttrDataFormat::SPECTRUM) {
      const auto length = accessor->getNumberOfSamples();
      // Tango needs a runtime-length C-style array here, cannot use std::array.
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<TangoType[]>(length);
      for(size_t i = 0; i < length; ++i) {
        data.get()[i] = convertToTangoType<TangoType, AdapterType>(accessor->accessData(i));
      }
      // Lint: Disabling because this is Tango code (see AdapterDeviceImpl.h)
      // NOLINTNEXTLINE(google-runtime-int)
      device->push_change_event(attrName, data.release(), timestamp, quality, static_cast<long>(length), 0, true);
    }
    else {
      auto data = std::make_unique<TangoType>(convertToTangoType<TangoType, AdapterType>(accessor->accessData(0)));
      device->push_change_event(attrName, data.release(), timestamp, quality, 1, 0, true);
    }
  }

  /********************************************************************************************************************/

  // DEV_VOID carries no data, so the read path sends a constant true (see ScalarAttribTempl::read) and there is no
  // accessor data to read. Handled separately because a ChimeraTK::Void register carries no data to convert.
  void pushVoidChangeEvent(
      Tango::DeviceImpl* device, const std::string& attrName, ChimeraTK::TransferElementAbstractor& pv) {
    auto quality = Tango::AttrQuality::ATTR_VALID;
    if(auto accessor = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<ChimeraTK::Void>>(
           pv.getHighLevelImplElement())) {
      if(accessor->dataValidity() != ChimeraTK::DataValidity::ok) {
        quality = Tango::AttrQuality::ATTR_INVALID;
      }
    }

    auto data = std::make_unique<Tango::DevBoolean>(true);
    device->push_change_event(
        attrName, data.release(), Tango::TangoTimestamp(std::chrono::system_clock::now()), quality, 1, 0, true);
  }
} // namespace detail

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

namespace TangoAdapter {
  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, std::string& s) : TANGO_BASE_CLASS(cl, s.c_str()) {}

  /********************************************************************************************************************/

  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s) : TANGO_BASE_CLASS(cl, s) {}

  /********************************************************************************************************************/

  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s, const char* d)
  : TANGO_BASE_CLASS(cl, s, d) {}

  /********************************************************************************************************************/

  void AdapterDeviceImpl::delete_device() {
    DEBUG_STREAM << "AdapterDeviceImpl::delete_device() " << device_name << std::endl;
    DEBUG_STREAM << AdapterDeviceClass::getClassName() << ": TangoAdapter::~TangoAdapter" << std::endl;

    // detach_dynamic_attributes_from_device();
    //_appInstance->_applicationInstance.shutdown()();
    // std::for_each(_dynamic_attribute_list.begin(), _dynamic_attribute_list.end(), [](auto* p) { delete p; });
    //_dynamic_attribute_list.clear();
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::init_device() {
    DEBUG_STREAM << AdapterDeviceClass::getClassName() << ": AdapterDeviceImpl::init_device() create device "
                 << device_name << std::endl;

    //	Get the device properties from database
    get_device_property();

    //	Initialize device

    auto& adapter = TangoAdapter::getInstance();
    if(!adapter.getError().empty()) {
      set_state(Tango::DevState::FAULT);
      set_status(adapter.getError());

      return;
    }

    DEBUG_STREAM << AdapterDeviceClass::getClassName() << ":ChimeraTKExample2::init_device() end of init_device "
                 << std::endl;

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::init_device
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::get_device_property() {}

  /********************************************************************************************************************/

  void AdapterDeviceImpl::always_executed_hook() {}

  /********************************************************************************************************************/

  void AdapterDeviceImpl::read_attr_hardware(TANGO_UNUSED(std::vector<long>& attr_list)) {}

  /********************************************************************************************************************/

  void AdapterDeviceImpl::add_dynamic_attributes() {}

  void AdapterDeviceImpl::add_dynamic_commands() {}

  void AdapterDeviceImpl::restoreMemoriedSpectra(const std::list<AttributeProperty>& writeableSpectrums) {
    DEBUG_STREAM << AdapterDeviceClass::getClassName() << ": TangoAdapter::restoreMemoriedSpectra "
                 << writeableSpectrums.size() << std::endl;

    // read spectrum values from memoried properties then write as initialised values
    for(const auto& attProp : writeableSpectrums) {
      DEBUG_STREAM << AdapterDeviceClass::getClassName() << ":name: " << attProp.name << " type:" << attProp.dataType
                   << std::endl;

      // get write attribute name, in case it is different (READ_WITH_WRITE)
      auto& write_attribute = get_device_attr()->get_w_attr_by_name(attProp.name.c_str());
      std::string attName = attProp.name;
      auto& baseAttribute = get_device_class()->get_class_attr()->get_attr(attName);

      // get value of memoried property (__Memorized_<attributename>)
      auto mem_value = getProperty<std::string>(this, "__Memoried_" + attProp.name);

      DEBUG_STREAM << AdapterDeviceClass::getClassName() << ":__Memoried_" << attProp.name
                   << " mem_value: " << mem_value << std::endl;

      if(mem_value.empty()) {
        continue;
      }

      switch(attProp.dataType) {
        case Tango::DEV_UCHAR: {
          ::detail::writeInitialSpectrumValue<Tango::DevUChar, uint8_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }

        case Tango::DEV_ENUM: {
          ::detail::writeInitialSpectrumValue<Tango::DevShort, int8_t>(this, mem_value, write_attribute, baseAttribute);
          break;
        }

        case Tango::DEV_USHORT: {
          ::detail::writeInitialSpectrumValue<Tango::DevUShort, uint16_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_ULONG: {
          ::detail::writeInitialSpectrumValue<Tango::DevULong, uint32_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_ULONG64: {
          ::detail::writeInitialSpectrumValue<Tango::DevULong64, uint64_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }

        case Tango::DEV_SHORT: {
          ::detail::writeInitialSpectrumValue<Tango::DevShort, int16_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_LONG: {
          ::detail::writeInitialSpectrumValue<Tango::DevLong, int32_t>(this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_LONG64: {
          ::detail::writeInitialSpectrumValue<Tango::DevLong64, int64_t>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_FLOAT: {
          ::detail::writeInitialSpectrumValue<Tango::DevFloat, float>(this, mem_value, write_attribute, baseAttribute);
          break;
        }

        case Tango::DEV_DOUBLE: {
          ::detail::writeInitialSpectrumValue<Tango::DevDouble, double>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_BOOLEAN: {
          ::detail::writeInitialSpectrumValue<Tango::DevBoolean, ChimeraTK::Boolean>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        case Tango::DEV_STRING: {
          ::detail::writeInitialSpectrumValue<Tango::DevString, std::string>(
              this, mem_value, write_attribute, baseAttribute);
          break;
        }
        default:
          ERROR_STREAM << AdapterDeviceClass::getClassName()
                       << ":TangoAdapter::restoreMemoriedSpectra - unknown datatype: " << attProp.dataType << std::endl;
      }
    }
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::attachToClassAttributes(const std::shared_ptr<AttributeMapper::DeviceClass>& deviceClass) {
    auto device = deviceClass->getDevice(get_name());
    auto& adapter = TangoAdapter::getInstance();
    auto csPvManager = adapter.getCsPvManager();
    std::list<AttributeProperty> writeableSpectrums;

    auto& updater = adapter.getUpdater();
    DEBUG_STREAM << "Attaching Variables to device instance " << device->name << std::endl;
    for(auto& attr : deviceClass->attributes) {
      auto& source = device->attributeToSource[attr.name];
      DEBUG_STREAM << "    " << source << std::endl;

      auto processVariable = csPvManager->getProcessVariable(source);
      auto pv = ChimeraTK::TransferElementAbstractor(processVariable);
      _attributeToPvMap[attr.name] = pv;
      // AttributeProperty is copy-constructible but not copy-assignable (operator= is deleted)
      _attributeProperties.emplace(attr.name, attr);

      // Properly namespace the pv in the updater so we can distinguish per device
      updater.addVariable(pv, get_name() + "/" + attr.name);

      if(attr.eventing != EventingMode::Off) {
        if(attr.eventing == EventingMode::Change) {
          updater.addVariable(pv, get_name() + "/" + attr.name + "/event",
              [this, attrName = attr.name]() { pushChangeEvent(attrName); });
        }
        else {
          updater.addVariable(pv, get_name() + "/" + attr.name + "/event",
              [this, attrName = attr.name]() { pushDataReadyEvent(attrName); });
        }
      }

      if(attr.attrDataFormat == AttrDataFormat::SPECTRUM &&
          (attr.writeType == Tango::WRITE || attr.writeType == Tango::READ_WRITE)) {
        writeableSpectrums.push_back(attr);
      }
    }

    // only need to write spectrum attributes (manual writing bug in Tango)
    // scalar attributes are memoried and initialized by Tango
    // But only if we are running with a database. If not, there is nothing to
    // restore anyway.
    if(Tango::Util::_UseDb) {
      restoreMemoriedSpectra(writeableSpectrums);
    }

    addMonitoredDeviceProperty(
        device->stateSource, "state", "__ChimeraTK_toTangoState",
        [](const std::type_info& type) {
          bool integral = false;
          ChimeraTK::callForTypeNoVoid(type, [&integral](auto t) {
            if constexpr(std::is_integral_v<decltype(t)>) {
              integral = true;
            }
          });
          return integral;
        },
        [this](auto& pv) {
          ChimeraTK::callForTypeNoVoid(pv.getValueType(), [this, &pv](auto t) {
            // The value type was checked when the source was registered, so this branch can always be taken
            if constexpr(std::is_integral_v<decltype(t)>) {
              auto typedPv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<decltype(t)>>(
                  pv.getHighLevelImplElement());
              // As per contract, this should be a StatusAccessor, so only 0, 1, 2 and 3 are supported and mapped
              // properly. Everything else will be "UNKNOWN" and a message will be logged
              switch(typedPv->accessData(0)) {
                case 0:
                  set_state(Tango::DevState::ON);
                  break;
                case 1:
                  set_state(Tango::DevState::FAULT);
                  break;
                case 2:
                  set_state(Tango::DevState::OFF);
                  break;
                case 3:
                  set_state(Tango::DevState::ALARM);
                  break;
                default:
                  set_state(Tango::DevState::UNKNOWN);
                  TANGO_LOG_WARN << std::format(
                      "Trying to map {} to a TANGO state is not supported", typedPv->accessData(0));
                  break;
              }
            }
          });
        });

    addMonitoredDeviceProperty(
        device->statusSource, "status", "__ChimeraTK_toTangoStatus",
        [](const std::type_info& type) { return type == typeid(std::string); },
        [this](auto& pv) {
          ChimeraTK::callForTypeNoVoid(pv.getValueType(), [this, &pv](auto t) {
            // The value type was checked when the source was registered, so this branch can always be taken
            if constexpr(std::is_same_v<decltype(t), std::string>) {
              auto typedPv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<decltype(t)>>(
                  pv.getHighLevelImplElement());
              set_status(typedPv->accessData(0));
            }
          });
        });
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::addMonitoredDeviceProperty(const std::optional<std::string>& source,
      const std::string& purpose, const std::string& updaterName,
      const std::function<bool(const std::type_info&)>& isSupportedType,
      std::function<void(ChimeraTK::TransferElementAbstractor&)> map) {
    if(!source) {
      return;
    }

    auto& adapter = TangoAdapter::getInstance();

    // The lookup throws a logic_error naming only the process variable when the source is not in the variable
    // household of the application, so it is caught here and rethrown with the purpose it was configured for.
    ChimeraTK::ProcessVariable::SharedPtr processVariable;
    try {
      processVariable = adapter.getCsPvManager()->getProcessVariable(source.value());
    }
    catch(ChimeraTK::logic_error& e) {
      throw ChimeraTK::logic_error(std::format("{} to be used as device {}: {}", source.value(), purpose, e.what()));
    }
    if(!processVariable->isReadable()) {
      throw ChimeraTK::logic_error(
          std::format("{} to be used as device {}, but is not readable", source.value(), purpose));
    }
    if(!isSupportedType(processVariable->getValueType())) {
      throw ChimeraTK::logic_error(
          std::format("{} to be used as device {} has an unsupported value type", source.value(), purpose));
    }

    auto pv = ChimeraTK::TransferElementAbstractor(processVariable);

    // The value which the application has already published and marked as valid when the device is created is
    // applied as well, so the device does not keep the default state and status.
    adapter.getUpdater().addVariable(pv, updaterName, [this, pv, map = std::move(map)]() mutable {
      // A source which has never carried a value only holds an initialisation value which the application has not
      // published, and a value which the application has marked as invalid must not be shown either.
      if(pv.getVersionNumber() == ChimeraTK::VersionNumber(nullptr) ||
          pv.dataValidity() != ChimeraTK::DataValidity::ok) {
        return;
      }

      // This is called from the updater thread, so the device lock has to be taken before setting the state
      // or the status
      Tango::AutoTangoMonitor sync(this);
      map(pv);
    });
  }

  /********************************************************************************************************************/

  ChimeraTK::TransferElementAbstractor AdapterDeviceImpl::getPvForAttribute(const std::string& attributeName) {
    try {
      return _attributeToPvMap.at(attributeName);
    }
    catch(std::out_of_range&) {
      return {};
    }
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::pushChangeEvent(const std::string& attrName) {
    auto pv = getPvForAttribute(attrName);
    if(!pv.isInitialised()) {
      return;
    }

    auto propertyIt = _attributeProperties.find(attrName);
    if(propertyIt == _attributeProperties.end()) {
      return;
    }
    const auto& property = propertyIt->second;

    // Dispatch on the raw Tango type: push_change_event is a type template, and only DEV_VOID needs the special path
    // because a ChimeraTK::Void register carries no data to convert.
    switch(property.dataType) {
      case Tango::DEV_UCHAR:
        detail::pushChangeEventForAttribute<Tango::DevUChar, uint8_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_ENUM:
        detail::pushChangeEventForAttribute<Tango::DevShort, int8_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_USHORT:
        detail::pushChangeEventForAttribute<Tango::DevUShort, uint16_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_SHORT:
        detail::pushChangeEventForAttribute<Tango::DevShort, int16_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_ULONG:
        detail::pushChangeEventForAttribute<Tango::DevULong, uint32_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_LONG:
        detail::pushChangeEventForAttribute<Tango::DevLong, int32_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_ULONG64:
        detail::pushChangeEventForAttribute<Tango::DevULong64, uint64_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_LONG64:
        detail::pushChangeEventForAttribute<Tango::DevLong64, int64_t>(this, attrName, property, pv);
        break;
      case Tango::DEV_FLOAT:
        detail::pushChangeEventForAttribute<Tango::DevFloat, float>(this, attrName, property, pv);
        break;
      case Tango::DEV_DOUBLE:
        detail::pushChangeEventForAttribute<Tango::DevDouble, double>(this, attrName, property, pv);
        break;
      case Tango::DEV_STRING:
        detail::pushChangeEventForAttribute<Tango::DevString, std::string>(this, attrName, property, pv);
        break;
      case Tango::DEV_BOOLEAN:
        detail::pushChangeEventForAttribute<Tango::DevBoolean, ChimeraTK::Boolean>(this, attrName, property, pv);
        break;
      case Tango::DEV_VOID:
        detail::pushVoidChangeEvent(this, attrName, pv);
        break;
      default:
        DEV_ERROR_STREAM(this) << "Unsupported data type for change event on attribute " << attrName << std::endl;
        break;
    }
  }

  /********************************************************************************************************************/

  void AdapterDeviceImpl::pushDataReadyEvent(const std::string& attrName) {
    auto& counter = _dataReadyCounters[attrName];
    if(counter == 0) {
      counter = 1;
    }
    push_data_ready_event(attrName, counter);
    ++counter;
  }
} // namespace TangoAdapter

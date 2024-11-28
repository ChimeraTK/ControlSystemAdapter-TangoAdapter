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

    DEBUG_STREAM << "Attaching Variables to device instance " << device->name << std::endl;
    for(auto& attr : deviceClass->attributes) {
      auto& source = device->attributeToSource[attr.name];
      DEBUG_STREAM << "    " << source << std::endl;

      auto processVariable = csPvManager->getProcessVariable(source);
      auto pv = ChimeraTK::TransferElementAbstractor(processVariable);
      _attributeToPvMap[attr.name] = pv;

      // Properly namespace the pv in the updater so we can distinguish per device
      adapter.getUpdater().addVariable(pv, get_name() + "/" + attr.name, [this, attr]() {
        if(attr.notificationType == AttributeProperty::NotificationType::change) {
          push_chage_event();
        }
      });

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
} // namespace TangoAdapter

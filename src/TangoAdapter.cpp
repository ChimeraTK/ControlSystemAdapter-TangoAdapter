
#include "TangoAdapter.h"

#include "AttributeMapper.h"
#include "AdapterDeviceClass.h"
#include "SpectrumAttribTempl.h"
#include "getAllVariableNames.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ReadAnyGroup.h>
#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/VoidRegisterAccessor.h>

#include <algorithm>
#include <filesystem>

namespace detail {
  template<typename TangoType, typename AdapterType>
  void writeInitialSpectrumValue(Tango::DeviceImpl* device, const std::string& memoriedValue,
      Tango::WAttribute& writeAttribute, Tango::Attr* attr) {

    // StringToArray will create a std::vector<bool> which is an optimised version of std::vector
    // But Tango needs a plain array of bool, so we have to convert it here.
    if constexpr(std::is_same_v<Tango::DevBoolean, TangoType>) {
      auto values = ChimeraTK::stringToArray<TangoType>(memoriedValue);
      // No lint, we cannot use std::array here because we do not know the size at compile time
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<Tango::DevBoolean[]>(values.size());
      std::copy(values.begin(), values.end(), data.get());

      // We can pass in the unique pointer here, set_write_value will copy the data
      writeAttribute.set_write_value(data.get(), values.size());
    } else if constexpr(std::is_same_v<Tango::DevString, TangoType>) {
      auto values = ChimeraTK::stringToArray<std::string>(memoriedValue);
      // No lint, we cannot use std::array here because we do not know the size at compile time
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<Tango::DevString[]>(values.size());
      std::transform(values.begin(), values.end(), data.get(), [&](auto& v) {
        //NOLINTNEXTLINE
        return const_cast<char *>(v.c_str());
      });

      // We can pass in the unique pointer and c_str() here, set_write_value will copy the data
      writeAttribute.set_write_value(data.get(), values.size());
    }
    else {
      auto values = ChimeraTK::stringToArray<TangoType>(memoriedValue);
      writeAttribute.set_write_value(values.data(), values.size());
    }

    attr->write(device, writeAttribute);
  }
}; // namespace detail

namespace ChimeraTK {
  //+----------------------------------------------------------------------------
  //
  // method :    Constructor
  //
  // description :
  //
  //-----------------------------------------------------------------------------
  TangoAdapter::TangoAdapter(TANGO_BASE_CLASS* tangoDevice, std::vector<std::shared_ptr<ChimeraTK::AttributeProperty>>& attributeList)
  : Tango::LogAdapter(tangoDevice), _deviceClass(::TangoAdapter::AdapterDeviceClass::getClassName()) {
    DEBUG_STREAM << _deviceClass <<":TangoAdapter::TangoAdapter starting ... " << std::endl;
    _device = tangoDevice;

    std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
        createPVManager();

    _updater = boost::make_shared<TangoUpdater>(tangoDevice);

    _controlSystemPVManager = pvManagers.first;
    _devicePVManager = pvManagers.second;

    // We do not get ownership of the application here. A plain pointer is used because the reference returned
    // by getApplicationInstance cannot be stored to a variable that has been created outside of the try block,
    // and we want to limit the scope of the try/catch to that single line.
    DEBUG_STREAM << "TangoAdapter starting application from " << std::filesystem::current_path() << std::endl;
    ApplicationBase* appInstance = nullptr;
    try {
      appInstance = &ChimeraTK::ApplicationFactoryBase::getApplicationInstance();
    }
    catch(ChimeraTK::logic_error& e) {
      FATAL_STREAM << "*************************************************************"
                      "***************************************"
                   << std::endl;
      FATAL_STREAM << "Logic error when getting the application instance. The TangoAdapter requires the use of the "
                      "ChimeraTK::ApplicationFactory instead of a static apllication instance."
                   << std::endl;

      FATAL_STREAM << "Replace `static MyApplication theApp` with `static ChimeraTK::ApplicationFactory<MyApplication> "
                      "theAppFactory`."
                   << std::endl;
      FATAL_STREAM << "*************************************************************"
                      "***************************************"
                   << std::endl;

      FATAL_STREAM << e.what() << std::endl;
      _device->set_state(Tango::FAULT);
      _device->set_status(e.what());
      return;
    }
    appInstance->setPVManager(_devicePVManager);
    appInstance->initialise();

    // the variable manager can only be filled after we have the CS manager
    std::set<std::string> names = getAllVariableNames(_controlSystemPVManager);
    std::string tickname;

    INFO_STREAM << _deviceClass <<":TangoAdapter::TangoAdapter list of variable" << std::endl;
    for(const auto& name : names) {
      INFO_STREAM << name << std::endl;
      if(name.find("/tick") != std::string::npos) {
        tickname = name;
      }
    }

    // no configuration, import all
    if(attributeList.empty()) {
      INFO_STREAM << _deviceClass <<":Direct import" << std::endl;
      // FIXME: Why?
      names.erase(tickname);
      ChimeraTK::AttributeMapper::getInstance().setCSPVManager(_controlSystemPVManager);
      ChimeraTK::AttributeMapper::getInstance().directImport(names);
    }
    // configured attributes from property
    else {
      ChimeraTK::AttributeMapper::getInstance().prepareOutput(attributeList);
    }
    // create the dynamic attributes for Tango devices
    create_dynamic_attributes();

    // only need to write spectrum attributes (manual writing bug in Tango)
    // scalar attributes are memoried and initialized by Tango
    // But only if we are running with a database. If not, there is nothing to
    // restore anyway.
    if (Tango::Util::_UseDb) {
      write_inited_values();
    }

    // start the application
    appInstance->run();
    _updater->run();
    _device->set_state(Tango::ON);
    _device->set_status("Application is running.");

    DEBUG_STREAM << _deviceClass <<":TangoAdapter::TangoAdapter end" << std::endl;
  }
  //+----------------------------------------------------------------------------
  //
  // method :  Destructor
  //
  // description :
  //
  //-----------------------------------------------------------------------------
  TangoAdapter::~TangoAdapter() {
    DEBUG_STREAM << _deviceClass <<": TangoAdapter::~TangoAdapter" << std::endl;

    _updater->stop();

    detach_dynamic_attributes_from_device();
    // Attention to stop application here but impossible
    //_appInstance->_applicationInstance.shutdown()();
    std::for_each(_dynamic_attribute_list.begin(), _dynamic_attribute_list.end(), [](auto* p) { delete p; });
    _dynamic_attribute_list.clear();
  }
  //+----------------------------------------------------------------------------
  //
  // method :  TangoAdapter::write_inited_values
  //
  // description : write memoried value at initialisation
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::write_inited_values() {
    DEBUG_STREAM << _deviceClass <<": TangoAdapter::write_inited_values " << _write_spectrum_attr_list.size() << std::endl;

    // read spectrum values from memoried properties then write as initialised values
    for(const auto& [attProp, index] : _write_spectrum_attr_list) {
      DEBUG_STREAM << _deviceClass <<":name: " << attProp->name << " type:" << attProp->dataType << std::endl;
      // get write attribute name
      auto& write_attribute = _device->get_device_attr()->get_w_attr_by_name(attProp->name.c_str());
      // get value of memoried property (__Memorized_<attributename>)
      auto mem_value = getProperty<std::string>(_device, "__Memoried_" + attProp->name);

      DEBUG_STREAM << _deviceClass <<":__Memoried_" << attProp->name << " mem_value: " << mem_value << std::endl;

      if(mem_value.empty()) {
        continue;
      }

      Tango::Attr* base_attr = _dynamic_attribute_list[index];

      switch(attProp->dataType) {
        case Tango::DEV_UCHAR: {
          ::detail::writeInitialSpectrumValue<Tango::DevUChar, uint8_t>(_device, mem_value, write_attribute, base_attr);
          break;
        }

        case Tango::DEV_USHORT: {
          ::detail::writeInitialSpectrumValue<Tango::DevUShort, uint16_t>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_ULONG: {
          ::detail::writeInitialSpectrumValue<Tango::DevULong, uint32_t>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_ULONG64: {
          ::detail::writeInitialSpectrumValue<Tango::DevULong64, uint64_t>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }

        case Tango::DEV_SHORT: {
          ::detail::writeInitialSpectrumValue<Tango::DevShort, int16_t>(_device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_LONG: {
          ::detail::writeInitialSpectrumValue<Tango::DevLong, int32_t>(_device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_LONG64: {
          ::detail::writeInitialSpectrumValue<Tango::DevLong64, int64_t>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_FLOAT: {
          ::detail::writeInitialSpectrumValue<Tango::DevFloat, float>(_device, mem_value, write_attribute, base_attr);
          break;
        }

        case Tango::DEV_DOUBLE: {
          ::detail::writeInitialSpectrumValue<Tango::DevDouble, double>(_device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_BOOLEAN: {
          ::detail::writeInitialSpectrumValue<Tango::DevBoolean, Boolean>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }
        case Tango::DEV_STRING: {
          ::detail::writeInitialSpectrumValue<Tango::DevString, std::string>(
              _device, mem_value, write_attribute, base_attr);
          break;
        }
        default:
          ERROR_STREAM << _deviceClass <<":TangoAdapter::write_inited_values - unknown datatype: " << attProp->dataType << std::endl;
      }
    }
    // check for variables not yet initialised - we must guarantee that all to-application variables are written exactly
    // once at server start.
    for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
      if(!pv->isWriteable()) {
        continue;
      }

      if(pv->getVersionNumber() == ChimeraTK::VersionNumber(nullptr)) {
        // The variable has not yet been written. Do it now, even if we just send a 0.
        pv->write();
      }
    }
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::create_dynamic_attributes()
  //
  // description :    This method creates the dynamic attrs.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::create_dynamic_attributes() {
    DEBUG_STREAM << " TangoAdapter::create_dynamic_attributes " << std::endl;

    auto descList = AttributeMapper::getInstance().getAttDescList();
    for(const auto& attDesc : descList) {
      if(attDesc->attrDataFormat == SCALAR) {
        create_Scalar_Attr(attDesc);
      }
      else if(attDesc->attrDataFormat == SPECTRUM) {
        create_Spectrum_Attr(attDesc);
      }
    }

    attach_dynamic_attributes_to_device();
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::create_Scalar_Attr()
  //
  // description :    This method creates the scalar attr.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::create_Scalar_Attr(std::shared_ptr<AttributeProperty> const& attProp) {
    DEBUG_STREAM << _deviceClass <<":TangoAdapter::create_Scalar_Attr" << std::endl;

    ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->path);

    if(processVariable->isWriteable() && processVariable->isReadable()) {
      attProp->writeType = Tango::READ_WRITE;
    }
    else if(processVariable->isWriteable()) {
      attProp->writeType = Tango::WRITE;
    }
    else {
      attProp->writeType = Tango::READ;
    }

    DEBUG_STREAM << _deviceClass <<":TangoAdapter::create_Scalar_Attr write Type" << attProp->writeType << std::endl;

    switch(attProp->dataType) {
      case Tango::DEV_UCHAR: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevUChar, uint8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint8_t>(pv), attProp->name);
        break;
      }
      // FIXME: HACK. There is no CHAR type in Tango
      case Tango::DEV_ENUM: {
        attProp->dataType = Tango::DEV_SHORT;
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int8_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevShort, int8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int8_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_USHORT: {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint16_t>> pv =
            boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevUShort, uint16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint16_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_SHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevShort, int16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int16_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_DOUBLE: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevDouble, double>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<double>(pv), attProp->name);
        break;
      }
      case Tango::DEV_FLOAT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevFloat, float>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<float>(pv), attProp->name);
        break;
      }
      case Tango::DEV_ULONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevULong64, uint64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint64_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_LONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevLong64, int64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int64_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_ULONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevULong, uint32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint32_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_LONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevLong, int32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int32_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_STRING: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevString, std::string>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<std::string>(pv), attProp->name);
        break;
      }
      case Tango::DEV_BOOLEAN: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
        // Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
        auto cast_pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<ChimeraTK::Boolean>>(pv);
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>(_device, cast_pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<Boolean>(pv), attProp->name);
        break;
      }
      case Tango::DEV_VOID: {
        // DEV_VOID is not a type. It is usually used to signify that a Command does not have a parameter
        // or a return value. Hence we need to map it to something else, let's choose boolean for that
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Void>>(processVariable);

        attProp->dataType = Tango::DEV_BOOLEAN;
        auto* scalar_attr_t = new ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Void>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::VoidRegisterAccessor(pv), attProp->name);
        break;
      }
      default:

        ERROR_STREAM << _deviceClass <<": Not supported data type in scalar: " << attProp->dataType << std::endl;
        break;
    }
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::create_Spectrum_Attr()
  //
  // description :    This method creates the scalar attr.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::create_Spectrum_Attr(std::shared_ptr<AttributeProperty> const& attProp) {
    DEBUG_STREAM << _deviceClass <<": TangoAdapter::create_Spectrum_Attr" << std::endl;

    ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->path);

    if(processVariable->isWriteable() && processVariable->isReadable()) {
      attProp->writeType = Tango::READ_WRITE;
    }
    else if(processVariable->isWriteable()) {
      attProp->writeType = Tango::WRITE;
    }
    else {
      attProp->writeType = Tango::READ;
    }

    switch(attProp->dataType) {
      case Tango::DEV_UCHAR: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevUChar, uint8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint8_t>(pv), attProp->name);
        break;
      }
      // FIXME: HACK. There is no CHAR type in tango
      case Tango::DEV_ENUM: {
        attProp->dataType = Tango::DEV_SHORT;
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int8_t>>(processVariable);
        auto* scalar_attr_t = new SpectrumAttribTempl<Tango::DevShort, int8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int8_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_USHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevUShort, uint16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint16_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_SHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevShort, int16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int16_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_ULONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevULong, uint32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint32_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_LONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevLong, int32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int32_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_DOUBLE: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevDouble, double>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<double>(pv), attProp->name);
        break;
      }
      case Tango::DEV_FLOAT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevFloat, float>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<float>(pv), attProp->name);
        break;
      }
      case Tango::DEV_ULONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevULong64, uint64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint64_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_LONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevLong64, int64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int64_t>(pv), attProp->name);
        break;
      }
      case Tango::DEV_STRING: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevString, std::string, Tango::ConstDevString>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<std::string>(pv), attProp->name);
        break;
      }
      case Tango::DEV_BOOLEAN: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
        // Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
        auto* spectrum_attr_t = new SpectrumAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<Boolean>(pv), attProp->name);
        break;
      }
      default:

        ERROR_STREAM << _deviceClass <<": Not supported data type in spectrum: " << attProp->dataType << std::endl;
        break;
    }

    if(processVariable->isWriteable()) {
      _write_spectrum_attr_list.insert({attProp, _dynamic_attribute_list.size() - 1});
    }
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::attach_dynamic_attributes_to_device()
  //
  // description :    This method attachs the dynamics attributes to the device.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::attach_dynamic_attributes_to_device() {
    DEBUG_STREAM << _deviceClass <<":TangoAdapter::attach_dynamic_attributes_to_device" << std::endl;

    for(auto* attr : _dynamic_attribute_list) {
      _device->add_attribute(attr);
    }

    _device->set_state(Tango::ON);
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::detach_dynamic_attributes_from_device()
  //
  // description :    This method detachs the dynamics attributes from the device.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::detach_dynamic_attributes_from_device() {
    DEBUG_STREAM << _deviceClass <<":TangoAdapter::detach_dynamic_attributes_from_device" << std::endl;

    for(auto* attr : _dynamic_attribute_list) {
      _device->remove_attribute(attr, false, false /*do not cleanup tangodb when removing this dyn. attr*/);
    }
  }
} // namespace ChimeraTK

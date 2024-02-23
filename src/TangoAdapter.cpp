
#include "TangoAdapter.h"

#include "AttributMapper.h"
#include "getAllVariableNames.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ReadAnyGroup.h>
#include <ChimeraTK/RegisterPath.h>

#include <algorithm>
#include <stdexcept>

namespace ChimeraTK {
  //+----------------------------------------------------------------------------
  //
  // method :    Constructor
  //
  // description :
  //
  //-----------------------------------------------------------------------------
  TangoAdapter::TangoAdapter(TANGO_BASE_CLASS* tangoDevice, std::vector<std::string> attributList)
  : Tango::LogAdapter(tangoDevice) {
    DEBUG_STREAM << "TangoAdapter::TangoAdapter starting ... " << std::endl;
    _device = tangoDevice;

    std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
        createPVManager();

    _updater = boost::make_shared<TangoUpdater>(tangoDevice);

    _controlSystemPVManager = pvManagers.first;
    _devicePVManager = pvManagers.second;

    // We do not get ownership of the application here. A plain pointer is used because the reference returned
    // by getApplicationInstance cannot be stored to a variable that has been created outside of the try block,
    // and we want to limit the scope of the try/catch to that single line.
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

    INFO_STREAM << "TangoAdapter::TangoAdapter list of variable" << std::endl;
    for(std::set<std::string>::iterator it = names.begin(); it != names.end(); ++it) {
      INFO_STREAM << *it << std::endl;
      if((*it).find("/tick") != std::string::npos) tickname = *it;
    }

    // no configuration, import all
    if(attributList.size() == 0 || (attributList.size() == 1 && attributList[0] == "")) {
      INFO_STREAM << "Direct import" << std::endl;
      names.erase(tickname);
      ChimeraTK::AttributMapper::getInstance().setCSPVManager(_controlSystemPVManager);
      ChimeraTK::AttributMapper::getInstance().directImport(names);
    }
    // configured attributs from property
    else {
      ChimeraTK::AttributMapper::getInstance().prepareOutput(attributList);
    }
    // create the dynamic attributes for Tango devices
    create_dynamic_attributes();

    // only need to write spectrum attributs (manual writing bug in Tango)
    // scalar attributs are memoried and initialized by Tango
    write_inited_values();

    // start the application
    appInstance->run();
    _updater->run();
    _device->set_state(Tango::ON);
    _device->set_status("Application is running.");

    DEBUG_STREAM << "TangoAdapter::TangoAdapter end" << std::endl;
  }
  //+----------------------------------------------------------------------------
  //
  // method :  Destructor
  //
  // description :
  //
  //-----------------------------------------------------------------------------
  TangoAdapter::~TangoAdapter() {
    DEBUG_STREAM << " TangoAdapter::~TangoAdapter" << std::endl;

    _updater->stop();

    detach_dynamic_attributes_from_device();
    // Attention to stop application here but impossible
    //_appInstance->_applicationInstance.shutdown()();
  }
  //+----------------------------------------------------------------------------
  //
  // method :  TangoAdapter::write_inited_values
  //
  // description : write memoried value at initialisation
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::write_inited_values(void) {
    DEBUG_STREAM << " TangoAdapter::write_inited_values " << _write_spectrum_attr_list.size() << std::endl;

    //read spectrum values from memoried properties then write as initialised values
    for (const auto& [attProp, index] : _write_spectrum_attr_list) {

      DEBUG_STREAM <<"name: "<<attProp->_name<<" type:"<<attProp->_dataType<<std::endl;
      // get write attribut name
      Tango::WAttribute &write_attribute = _device->get_device_attr()->get_w_attr_by_name(attProp->_name.c_str());
      //get value of memoried property (__Memorized_<attributname>)
      std::string mem_value = get_property<std::string>(_device, "__Memoried_" + attProp->_name);

      DEBUG_STREAM <<"__Memoried_"<<attProp->_name<<" mem_value: "<<mem_value<<std::endl;

      if (mem_value.empty()) continue;

      Tango::Attr *base_attr = _dynamic_attribute_list[index];

      switch (attProp->_dataType) {
        case Tango::DEV_UCHAR: {
          std::vector<uint8_t> values = string_to_array<uint8_t>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<uint8_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }

        case Tango::DEV_USHORT: {
          std::vector<Tango::DevUShort> values = string_to_array<Tango::DevUShort>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<uint16_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_ULONG: {
          std::vector<Tango::DevULong> values = string_to_array<Tango::DevULong>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<uint32_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_ULONG64: {
          std::vector<uint64_t> values = string_to_array<uint64_t>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<uint64_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_SHORT: {
          std::vector<Tango::DevShort> values = string_to_array<Tango::DevShort>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<int16_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_LONG: {
          std::vector<Tango::DevLong> values = string_to_array<Tango::DevLong>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<int32_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_LONG64: {
          std::vector<Tango::DevLong64> values = string_to_array<Tango::DevLong64>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<int64_t>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_FLOAT: {
          std::vector<Tango::DevFloat> values = string_to_array<Tango::DevFloat>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<float>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_DOUBLE: {
          std::vector<Tango::DevDouble> values = string_to_array<Tango::DevDouble>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<double>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_BOOLEAN: {
          std::vector<Tango::DevBoolean> values = string_to_array<Tango::DevBoolean >(mem_value);
          //for bool it must do this way
          Tango::DevBoolean *bool_array = new Tango::DevBoolean[values.size()];
          for (unsigned int i = 0;i < values.size();i++) bool_array[i] = values[i];

          write_attribute.set_write_value(bool_array, values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<bool>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        case Tango::DEV_STRING: {
          std::vector<Tango::DevString> values = string_to_array<Tango::DevString>(mem_value);
          write_attribute.set_write_value(values.data(), values.size());
          auto *spectrum_attr_t = static_cast<SpectrumAttribTempl<std::string>*>(base_attr);
          spectrum_attr_t->write(_device, write_attribute);
          break;
        }
        default:
          ERROR_STREAM<<"TangoAdapter::write_inited_values - unknown datatype: "<< attProp->_dataType <<std::endl;

      }

    }
    // check for variables not yet initialised - we must guarantee that all to-application variables are written exactly
    // once at server start.
    for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
      if(!pv->isWriteable()) continue;

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
  void TangoAdapter::create_dynamic_attributes(void) {
    DEBUG_STREAM << " TangoAdapter::create_dynamic_attributes " << std::endl;

    auto descList = AttributMapper::getInstance().getAttDescList();
    for(auto attDesc : descList) {
      if(attDesc->getDataFormat() == SCALAR) {
        create_Scalar_Attr(attDesc);
      }
      else if(attDesc->getDataFormat() == SPECTRUM) {
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
  void TangoAdapter::create_Scalar_Attr(std::shared_ptr<AttributProperty> const& attProp) {
    DEBUG_STREAM << "TangoAdapter::create_Scalar_Attr" << std::endl;

    ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->_path);

    if(processVariable->isWriteable() && processVariable->isReadable()) {
      attProp->_writeType = Tango::READ_WRITE;
    }
    else if(processVariable->isWriteable()) {
      attProp->_writeType = Tango::WRITE;
    }
    else {
      attProp->_writeType = Tango::READ;
    }

    switch(attProp->_dataType) {
      case Tango::DEV_UCHAR: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint8_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_USHORT: {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint16_t>> pv =
            boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint16_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_SHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<int16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int16_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_DOUBLE: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<double>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<double>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_FLOAT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<float>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<float>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_ULONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint64_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_LONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<int64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int64_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_ULONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<uint32_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_LONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<int32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<int32_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_STRING: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<std::string>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<std::string>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_BOOLEAN: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
        // Attribut Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
        auto cast_pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(pv);
        auto* scalar_attr_t = new ScalarAttribTempl<uint8_t>(_device, cast_pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        _updater->addVariable(ChimeraTK::ScalarRegisterAccessor<Boolean>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      default:

        ERROR_STREAM << " Not supported data type in scalar: " << attProp->_dataType << std::endl;
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
  void TangoAdapter::create_Spectrum_Attr(std::shared_ptr<AttributProperty> const& attProp) {
    DEBUG_STREAM << "TangoAdapter::create_Spectrum_Attr" << std::endl;

    ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->_path);

    if(processVariable->isWriteable() && processVariable->isReadable()) {
      attProp->_writeType = Tango::READ_WRITE;
    }
    else if(processVariable->isWriteable()) {
      attProp->_writeType = Tango::WRITE;
    }
    else {
      attProp->_writeType = Tango::READ;
    }

    switch(attProp->_dataType) {
      case Tango::DEV_UCHAR: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<uint8_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint8_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_USHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<uint16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint16_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_SHORT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<int16_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int16_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_ULONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<uint32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint32_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_LONG: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<int32_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int32_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_DOUBLE: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<double>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<double>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_FLOAT: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<float>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<float>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_ULONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<uint64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<uint64_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_LONG64: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<int64_t>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<int64_t>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_STRING: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        auto* spectrum_attr_t = new SpectrumAttribTempl<std::string>(_device, pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<std::string>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
      case Tango::DEV_BOOLEAN: {
        auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
        // Attribut Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
        auto casted_pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(pv);
        auto* spectrum_attr_t = new SpectrumAttribTempl<uint8_t>(_device, casted_pv, attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        _updater->addVariable(ChimeraTK::OneDRegisterAccessor<Boolean>(pv), attProp->_name.c_str(),
            attProp->_attrDataFormat, attProp->_dataType);
        break;
      }
    }

    if(processVariable->isWriteable()) _write_spectrum_attr_list.insert({attProp, _dynamic_attribute_list.size() - 1});
  }

  //+----------------------------------------------------------------------------
  //
  // method :         TangoAdapter::attach_dynamic_attributes_to_device()
  //
  // description :    This method attachs the dynamics attributes to the device.
  //
  //-----------------------------------------------------------------------------
  void TangoAdapter::attach_dynamic_attributes_to_device(void) {
    DEBUG_STREAM << "TangoAdapter::attach_dynamic_attributes_to_device" << std::endl;

    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i) {
      _device->add_attribute(_dynamic_attribute_list[i]);
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
  void TangoAdapter::detach_dynamic_attributes_from_device(void) {
    DEBUG_STREAM << "TangoAdapter::detach_dynamic_attributes_from_device" << std::endl;

    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i)
      _device->remove_attribute(
          _dynamic_attribute_list[i], false, false /*do not cleanup tangodb when removing this dyn. attr*/);
  }
} // namespace ChimeraTK
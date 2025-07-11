// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AdapterDeviceClass.h"

#include "AdapterDeviceImpl.h"
#include "TangoAdapter.h"
#include "TangoLogCompat.h"

#include <algorithm>
#include <regex>

extern "C" {
#ifdef _TG_WINDOWS_

__declspec(dllexport)
#endif
// Naming is for Tango
// NOLINTNEXTLINE(readability-identifier-naming,reserved-identifier)
Tango::DeviceClass*
    _create_AdapterDeviceImpl_class([[maybe_unused]] const char* name) {
  // FIXME: Do we need to support this? It's for using this class in pytango
  assert(false);
  return nullptr;
}
}

namespace TangoAdapter {

  // Apply some heuristics to derive the class name from the executable name
  std::string AdapterDeviceClass::getClassName() {
    std::string ourName{Tango::Util::instance()->get_ds_exec_name()};

    std::regex start{"^ds_?"};
    std::regex end{"_?ds$"};

    ourName = std::regex_replace(ourName, start, "");
    ourName = std::regex_replace(ourName, end, "");

    TANGO_LOG_DEBUG << "Deriving class name from executable name: " << Tango::Util::instance()->get_ds_exec_name()
                    << " -> " << ourName << std::endl;

    return ourName;
  }

  /********************************************************************************************************************/

  AdapterDeviceClass::AdapterDeviceClass(std::string& s) : Tango::DeviceClass(s) {
    TANGO_LOG_DEBUG << "Entering AdapterDeviceClass constructor" << std::endl;
    set_default_property();
    write_class_property();

    TANGO_LOG_DEBUG << "Leaving AdapterDeviceClass constructor" << std::endl;
  }

  /********************************************************************************************************************/

  Tango::DbDatum AdapterDeviceClass::getPropertyWithDefault(
      const Tango::DbData& list, const std::string& propertyName) {
    const auto& position = std::find_if(list.begin(), list.end(), [&](auto x) { return x.name == propertyName; });
    if(position != list.end()) {
      return *position;
    }

    return {name};
  }

  /********************************************************************************************************************/

  Tango::DbDatum AdapterDeviceClass::get_class_property(std::string& prop_name) {
    return getPropertyWithDefault(cl_prop, prop_name);
  }

  /********************************************************************************************************************/

  Tango::DbDatum AdapterDeviceClass::get_default_device_property(std::string& prop_name) {
    return getPropertyWithDefault(dev_def_prop, prop_name);
  }

  /********************************************************************************************************************/

  Tango::DbDatum AdapterDeviceClass::get_default_class_property(std::string& prop_name) {
    return getPropertyWithDefault(cl_def_prop, prop_name);
  }

  /********************************************************************************************************************/

  void AdapterDeviceClass::set_default_property() {}

  /********************************************************************************************************************/

  void AdapterDeviceClass::write_class_property() {
    //	First time, check if database used
    if(!Tango::Util::_UseDb) {
      return;
    }

    Tango::DbData data;
    std::string classname = get_name();

    auto ourClass = TangoAdapter::getInstance().getMapper().getClass(get_name());
    assert(ourClass);

    //	Put title
    Tango::DbDatum title("ProjectTitle");
    std::vector<std::string> str_title{ourClass->title.value_or(ourClass->name)};
    title << str_title;
    data.push_back(title);

    //	Put Description
    Tango::DbDatum description("Description");
    std::vector<std::string> str_desc;
    str_desc.emplace_back(ourClass->description.value_or("ChimeraTK-based DeviceServer"));
    description << str_desc;
    data.push_back(description);

    //  Put inheritance
    Tango::DbDatum inher_datum("InheritedFrom");
    std::vector<std::string> inheritance;
    inheritance.emplace_back("TANGO_BASE_CLASS");
    inher_datum << inheritance;
    data.push_back(inher_datum);

    //	Call database and and values
    get_db_class()->put_property(data);
  }

  /********************************************************************************************************************/

  void AdapterDeviceClass::device_factory(const Tango::DevVarStringArray* devlist_ptr) {
    //	Add your own code

    auto& adapter = TangoAdapter::getInstance();
    auto& mapper = adapter.getMapper();

    auto deviceClass = mapper.getClass(get_name());

    //	Create devices and add it into the device list
    for(unsigned int i = 0; i < devlist_ptr->length(); i++) {
      const auto* deviceName = (*devlist_ptr)[i].in();

      if(!deviceClass->hasDevice(deviceName) &&
          !deviceClass->hasDevice(TangoAdapter::PLAIN_IMPORT_DUMMY_DEVICE.data())) {
        std::cerr << "Device " << deviceName << "not known in attribute mapper. Skipping." << std::endl;
        continue;
      }

      auto device = std::make_unique<AdapterDeviceImpl>(this, deviceName);
      device->init_device();
      if(!deviceClass->hasDevice(deviceName) &&
          !deviceClass->hasDevice(TangoAdapter::PLAIN_IMPORT_DUMMY_DEVICE.data())) {
        // See if we have the "generic" device"
        DEV_ERROR_STREAM(device) << "Device " << deviceName << "not known in attribute mapper. Expect issues"
                                 << std::endl;
        device->set_state(Tango::FAULT);
        device->set_status("Device was not found in mapping file, no variables could be mapped.");
      }
      else {
        // Move the generic device we have to this device.
        if(!deviceClass->hasDevice(deviceName)) {
          auto genericDevice = deviceClass->devicesInDeviceClass[TangoAdapter::PLAIN_IMPORT_DUMMY_DEVICE.data()];
          deviceClass->devicesInDeviceClass.erase(TangoAdapter::PLAIN_IMPORT_DUMMY_DEVICE.data());
          genericDevice->name = deviceName;
          deviceClass->devicesInDeviceClass[deviceName] = genericDevice;
        }
        device->attachToClassAttributes(deviceClass);
      }

      // Check before if database used.
      if(Tango::Util::_UseDb && !Tango::Util::_FileDb) {
        export_device(device.get());
      }
      else {
        export_device(device.get(), deviceName);
      }

      // Hand over device pointer to Tango
      device_list.push_back(device.release());
    }

    //	Manage dynamic attributes if any
    erase_dynamic_attributes(get_class_attr()->get_attr_list());

    for(auto* dev : device_list) {
      // Only set the device state ok ON if it is still in the default constructed
      // value
      if(dev->get_state() == Tango::UNKNOWN) {
        dev->set_state(Tango::ON);
        dev->set_status("Application is running.");
      }
    }
  }

  /********************************************************************************************************************/

  void AdapterDeviceClass::attribute_factory([[maybe_unused]] std::vector<Tango::Attr*>& att_list) {
    auto& adapter = TangoAdapter::getInstance();
    auto& mapper = adapter.getMapper();

    auto deviceClass = mapper.getClass(get_name());

    for(auto& attDesc : deviceClass->attributes) {
      att_list.push_back(attDesc.toTangoAttribute().release());
    }

    //	Create a list of static attributes
    create_static_attribute_list(get_class_attr()->get_attr_list());
  }

  /********************************************************************************************************************/

  void AdapterDeviceClass::pipe_factory() {}

  /********************************************************************************************************************/

  void AdapterDeviceClass::command_factory() {
    auto& adapter = TangoAdapter::getInstance();
    auto& mapper = adapter.getMapper();

    auto deviceClass = mapper.getClass(get_name());

    for(auto& command : deviceClass->commands) {
      command_list.push_back(command->getTangoProxy());
    }
  }
  /********************************************************************************************************************/

  void AdapterDeviceClass::create_static_attribute_list(std::vector<Tango::Attr*>& att_list) {
    for(auto* attr : att_list) {
      auto att_name = attr->get_name();
      std::transform(att_name.begin(), att_name.end(), att_name.begin(), ::tolower);
      defaultAttList.push_back(att_name);
    }

    TANGO_LOG_DEBUG << defaultAttList.size() << " attributes in default list" << std::endl;
  }

  /********************************************************************************************************************/

  void AdapterDeviceClass::erase_dynamic_attributes(std::vector<Tango::Attr*>& att_list) {
    for(auto* dev_impl : device_list) {
      auto* dev = dynamic_cast<AdapterDeviceImpl*>(dev_impl);
      assert(dev != nullptr);

      std::vector<Tango::Attribute*>& dev_att_list = dev->get_device_attr()->get_attribute_list();
      std::vector<Tango::Attribute*>::iterator ite_att;
      for(ite_att = dev_att_list.begin(); ite_att != dev_att_list.end(); ++ite_att) {
        std::string att_name((*ite_att)->get_name_lower());
        if((att_name == "state") || (att_name == "status")) {
          continue;
        }
        auto ite_str = find(defaultAttList.begin(), defaultAttList.end(), att_name);
        if(ite_str == defaultAttList.end()) {
          TANGO_LOG_DEBUG << att_name << " is a UNWANTED dynamic attribute for device " << dev->name() << std::endl;
          Tango::Attribute& att = dev->get_device_attr()->get_attr_by_name(att_name.c_str());
          dev->remove_attribute(att_list[att.get_attr_idx()], true, false);
          --ite_att;
        }
      }
    }
  }
} // namespace TangoAdapter

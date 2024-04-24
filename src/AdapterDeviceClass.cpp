/*----- PROTECTED REGION ID(AdapterDeviceClass.cpp) ENABLED START -----*/
//=============================================================================
//
// file :        AdapterDeviceClass.cpp
//
// description : C++ source for the AdapterDeviceClass.
//               A singleton class derived from DeviceClass.
//               It implements the command and attribute list
//               and all properties and methods required
//               by the AdapterDeviceImpl once per process.
//
// project :
//
// This file is part of Tango device class.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.
//
//
//
//=============================================================================
//                This file is generated by POGO
//        (Program Obviously used to Generate tango Object)
//=============================================================================

#include "AdapterDeviceClass.h"

#include <algorithm>
#include <regex>

// This is required naming by Tango, so disable the linter
// NOLINTBEGIN(readability-identifier-naming)
#ifdef TANGO_LOG_DEBUG
#ifndef cout4
#  define cout4 TANGO_LOG_DEBUG
#endif

#ifndef cout2
#  define cout2 TANGO_LOG_INFO
#endif
#else
#define TANGO_LOG_DEBUG cout2
#endif
// NOLINTEND(readability-identifier-naming)

/*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass.cpp

//-------------------------------------------------------------------
/**
 *	Create AdapterDeviceClass singleton and
 *	return it in a C function for Python usage
 */
//-------------------------------------------------------------------
extern "C" {
#ifdef _TG_WINDOWS_

__declspec(dllexport)
#endif
    // Naming is for Tango
    // NOLINTNEXTLINE(readability-identifier-naming)
    Tango::DeviceClass* _create_AdapterDeviceImpl_class(const char* name) {
  return TangoAdapter::AdapterDeviceClass::init(name);
}
}

namespace TangoAdapter {
  //===================================================================
  //	Initialize pointer for singleton pattern
  //===================================================================
  AdapterDeviceClass* AdapterDeviceClass::_instance = nullptr;

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

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::AdapterDeviceClass(std::string &s)
   * description : 	constructor for the AdapterDeviceClass
   *
   * @param s	The class name
   */
  //--------------------------------------------------------
  AdapterDeviceClass::AdapterDeviceClass(std::string& s) : Tango::DeviceClass(s) {
    cout2 << "Entering AdapterDeviceClass constructor" << std::endl;
    set_default_property();
    write_class_property();

    /*----- PROTECTED REGION ID(AdapterDeviceClass::constructor) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::constructor

    cout2 << "Leaving AdapterDeviceClass constructor" << std::endl;
  }

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::~AdapterDeviceClass()
   * description : 	destructor for the AdapterDeviceClass
   */
  //--------------------------------------------------------
  AdapterDeviceClass::~AdapterDeviceClass() {
    /*----- PROTECTED REGION ID(AdapterDeviceClass::destructor) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::destructor

    _instance = nullptr;
  }

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::init
   * description : 	Create the object if not already done.
   *                  Otherwise, just return a pointer to the object
   *
   * @param	name	The class name
   */
  //--------------------------------------------------------
  AdapterDeviceClass* AdapterDeviceClass::init(const char* name) {
    if(_instance == nullptr) {
      try {
        std::string s(name);
        _instance = new AdapterDeviceClass(s);
      }
      catch(std::bad_alloc&) {
        throw;
      }
    }
    return _instance;
  }

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::instance
   * description : 	Check if object already created,
   *                  and return a pointer to the object
   */
  //--------------------------------------------------------
  AdapterDeviceClass* AdapterDeviceClass::instance() {
    if(_instance == nullptr) {
      std::cerr << "Class is not initialised !!" << std::endl;
      exit(-1);
    }
    return _instance;
  }

  //===================================================================
  //	Command execution method calls
  //===================================================================

  //===================================================================
  //	Properties management
  //===================================================================

  Tango::DbDatum AdapterDeviceClass::getPropertyWithDefault(
      const Tango::DbData& list, const std::string& propertyName) {
    const auto& position = std::find_if(list.begin(), list.end(), [&](auto x) { return x.name == propertyName; });
    if(position != list.end()) {
      return *position;
    }

    return {name};
  }
  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::get_class_property()
   *	Description : Get the class property for specified name.
   */
  //--------------------------------------------------------
  Tango::DbDatum AdapterDeviceClass::get_class_property(std::string& prop_name) {
    return getPropertyWithDefault(cl_prop, prop_name);
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::get_default_device_property()
   *	Description : Return the default value for device property.
   */
  //--------------------------------------------------------
  Tango::DbDatum AdapterDeviceClass::get_default_device_property(std::string& prop_name) {
    return getPropertyWithDefault(dev_def_prop, prop_name);
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::get_default_class_property()
   *	Description : Return the default value for class property.
   */
  //--------------------------------------------------------
  Tango::DbDatum AdapterDeviceClass::get_default_class_property(std::string& prop_name) {
    return getPropertyWithDefault(cl_def_prop, prop_name);
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::set_default_property()
   *	Description : Set default property (class and device) for wizard.
   *                For each property, add to wizard property name and description.
   *                If default value has been set, add it to wizard property and
   *                store it in a DbDatum.
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::set_default_property() {
    std::string prop_name;
    std::string prop_desc;
    std::string prop_def;
    std::vector<std::string> vect_data;

    //	Set Default Class Properties

    //	Set Default device Properties
    prop_name = "AttributeList";
    prop_desc = "AttributeList";
    prop_def = "";
    vect_data.clear();
    if(prop_def.length() > 0) {
      Tango::DbDatum data(prop_name);
      data << vect_data;
      dev_def_prop.push_back(data);
      add_wiz_dev_prop(prop_name, prop_desc, prop_def);
    }
    else {
      add_wiz_dev_prop(prop_name, prop_desc);
    }

    prop_name = "WorkingFolder";
    prop_desc = "Base folder containting DMAP files, server configuration etc.";
    add_wiz_dev_prop(prop_name, prop_desc);
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::write_class_property()
   *	Description : Set class description fields as property in database
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::write_class_property() {
    //	First time, check if database used
    if(!Tango::Util::_UseDb) {
      return;
    }

    Tango::DbData data;
    std::string classname = get_name();

    //	Put title
    Tango::DbDatum title("ProjectTitle");
    std::vector<std::string> str_title{AdapterDeviceClass::getClassName()};
    title << str_title;
    data.push_back(title);

    //	Put Description
    Tango::DbDatum description("Description");
    std::vector<std::string> str_desc;
    str_desc.emplace_back("ChimeraTK-based DeviceServer");
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

  //===================================================================
  //	Factory methods
  //===================================================================

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::device_factory()
   *	Description : Create the device object(s)
   *                and store them in the device list
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::device_factory(const Tango::DevVarStringArray* devlist_ptr) {
    /*----- PROTECTED REGION ID(AdapterDeviceClass::device_factory_before) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::device_factory_before

    //	Create devices and add it into the device list
    for(unsigned long i = 0; i < devlist_ptr->length(); i++) {
      cout4 << "Device name : " << (*devlist_ptr)[i].in() << std::endl;
      device_list.push_back(new AdapterDeviceImpl(this, (*devlist_ptr)[i]));
      device_list.back()->init_device();
    }

    //	Manage dynamic attributes if any
    // erase_dynamic_attributes(devlist_ptr, get_class_attr()->get_attr_list());

    //	Export devices to the outside world
    for(unsigned long i = 1; i <= devlist_ptr->length(); i++) {
      //	Add dynamic attributes if any
      auto* dev = dynamic_cast<AdapterDeviceImpl*>(device_list[device_list.size() - i]);

      assert(dev != nullptr);
      dev->add_dynamic_attributes();

      //	Check before if database used.
      if(Tango::Util::_UseDb && !Tango::Util::_FileDb) {
        export_device(dev);
      }
      else {
        export_device(dev, dev->get_name().c_str());
      }
    }

    /*----- PROTECTED REGION ID(AdapterDeviceClass::device_factory_after) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::device_factory_after
  }
  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::attribute_factory()
   *	Description : Create the attribute object(s)
   *                and store them in the attribute list
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::attribute_factory([[maybe_unused]] std::vector<Tango::Attr*>& att_list) {
    /*----- PROTECTED REGION ID(AdapterDeviceClass::attribute_factory_before) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::attribute_factory_before

    //	Create a list of static attributes
    create_static_attribute_list(get_class_attr()->get_attr_list());
    /*----- PROTECTED REGION ID(AdapterDeviceClass::attribute_factory_after) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::attribute_factory_after
  }
  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::pipe_factory()
   *	Description : Create the pipe object(s)
   *                and store them in the pipe list
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::pipe_factory() {
    /*----- PROTECTED REGION ID(AdapterDeviceClass::pipe_factory_before) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::pipe_factory_before
    /*----- PROTECTED REGION ID(AdapterDeviceClass::pipe_factory_after) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::pipe_factory_after
  }
  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::command_factory()
   *	Description : Create the command object(s)
   *                and store them in the command list
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::command_factory() {
    /*----- PROTECTED REGION ID(AdapterDeviceClass::command_factory_before) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::command_factory_before

    /*----- PROTECTED REGION ID(AdapterDeviceClass::command_factory_after) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::command_factory_after
  }

  //===================================================================
  //	Dynamic attributes related methods
  //===================================================================

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::create_static_attribute_list
   * description : 	Create the a list of static attributes
   *
   * @param	att_list	the ceated attribute list
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::create_static_attribute_list(std::vector<Tango::Attr*>& att_list) {
    for(auto* attr : att_list) {
      auto att_name = attr->get_name();
      std::transform(att_name.begin(), att_name.end(), att_name.begin(), ::tolower);
      defaultAttList.push_back(att_name);
    }

    cout2 << defaultAttList.size() << " attributes in default list" << std::endl;

    /*----- PROTECTED REGION ID(AdapterDeviceClass::create_static_att_list) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::create_static_att_list
  }

  //--------------------------------------------------------
  /**
   * method : 		AdapterDeviceClass::erase_dynamic_attributes
   * description : 	delete the dynamic attributes if any.
   *
   * @param	devlist_ptr	the device list pointer
   * @param	list of all attributes
   */
  //--------------------------------------------------------
  void AdapterDeviceClass::erase_dynamic_attributes(
      const Tango::DevVarStringArray* devlist_ptr, std::vector<Tango::Attr*>& att_list) {
    Tango::Util* tg = Tango::Util::instance();

    for(unsigned long i = 0; i < devlist_ptr->length(); i++) {
      auto* dev_impl = tg->get_device_by_name(((std::string)(*devlist_ptr)[i]).c_str());
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
          cout2 << att_name << " is a UNWANTED dynamic attribute for device " << (*devlist_ptr)[i] << std::endl;
          Tango::Attribute& att = dev->get_device_attr()->get_attr_by_name(att_name.c_str());
          dev->remove_attribute(att_list[att.get_attr_idx()], true, false);
          --ite_att;
        }
      }
    }
    /*----- PROTECTED REGION ID(AdapterDeviceClass::erase_dynamic_attributes) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::erase_dynamic_attributes
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceClass::get_attr_object_by_name()
   *	Description : returns Tango::Attr * object found by name
   */
  //--------------------------------------------------------
  // FIXME: Move into util namespace and the source file
  Tango::Attr* AdapterDeviceClass::get_attr_object_by_name(
      std::vector<Tango::Attr*>& att_list, const std::string& attname) {
    std::vector<Tango::Attr*>::iterator it;
    for(it = att_list.begin(); it < att_list.end(); ++it) {
      if((*it)->get_name() == attname) {
        return (*it);
      }
    }
    //	Attr does not exist
    return nullptr;
  }

  /*----- PROTECTED REGION ID(AdapterDeviceClass::Additional Methods) ENABLED START -----*/

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::Additional Methods
} // namespace TangoAdapter

/*----- PROTECTED REGION ID(AdapterDeviceImpl.cpp) ENABLED START -----*/
//=============================================================================
//
// file :        AdapterDeviceImpl.cpp
//
// description : C++ source for the AdapterDeviceImpl class and its commands.
//               The class is derived from Device. It represents the
//               CORBA servant object which will be accessed from the
//               network. All commands which can be executed on the
//               AdapterDeviceImpl are implemented in this file.
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

#include "AdapterDeviceImpl.h"

#include "AdapterDeviceClass.h"
#include "TangoAdapter.h"

#include <ChimeraTK/Utilities.h>

/*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl.cpp

/**
 *  AdapterDeviceImpl class description:
 *    Test of TangoAdapterfor ChimeraTK
 */

//================================================================
//  The following table gives the correspondence
//  between command and method names.
//
//  Command name  |  Method name
//================================================================
//  State         |  Inherited (no method)
//  Status        |  Inherited (no method)
//================================================================

//================================================================
//  Attributes managed is:
//================================================================
//================================================================

namespace TangoAdapter {
  /*----- PROTECTED REGION ID(AdapterDeviceImpl::namespace_starting) ENABLED START -----*/

  //	static initializations

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::namespace_starting

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::AdapterDeviceImpl()
   *	Description : Constructors for a Tango device
   *                implementing the classAdapterDeviceImpl
   */
  //--------------------------------------------------------
  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, std::string& s) : TANGO_BASE_CLASS(cl, s.c_str()) {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::constructor_1) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::constructor_1
  }
  //--------------------------------------------------------
  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s) : TANGO_BASE_CLASS(cl, s) {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::constructor_2) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::constructor_2
  }
  //--------------------------------------------------------
  AdapterDeviceImpl::AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s, const char* d)
  : TANGO_BASE_CLASS(cl, s, d) {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::constructor_3) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::constructor_3
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::delete_device()
   *	Description : will be called at device destruction or at init command
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::delete_device() {
    DEBUG_STREAM << "AdapterDeviceImpl::delete_device() " << device_name << std::endl;
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::delete_device) ENABLED START -----*/

    //	Delete device allocated objects

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::delete_device
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::init_device()
   *	Description : will be called at device initialization.
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::init_device() {
    DEBUG_STREAM << "AdapterDeviceImpl::init_device() create device " << device_name << std::endl;
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::init_device_before) ENABLED START -----*/

    //	Initialization before get_device_property() call

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::init_device_before

    //	Get the device properties from database
    get_device_property();

    /*----- PROTECTED REGION ID(AdapterDeviceImpl::init_device) ENABLED START -----*/

    //	Initialize device
    // set DMapFilePath from property
    if(dMapFilePath.empty()) {
      if(!Tango::Util::_UseDb) {
        DEBUG_STREAM << "Running without database connection, falling back to devices.dmap" << std::endl;
        dMapFilePath = "devices.dmap";
      }
      else {
        ERROR_STREAM << "init_device: The property dMapFilePath is empty" << std::endl;
        set_state(Tango::FAULT);
        set_status("The property dMapFilePath is not configured");
        return;
      }
    }

    DEBUG_STREAM << "dMapFilePath: " << dMapFilePath << std::endl;
    try {
      ChimeraTK::setDMapFilePath(dMapFilePath);
    }
    catch(ChimeraTK::logic_error& e) {
      ERROR_STREAM << "init_device: " << e.what() << std::endl;
      set_state(Tango::FAULT);
      set_status(e.what());
      return;
    }

    tangoAdapter = std::make_shared<ChimeraTK::TangoAdapter>(this, attributList);
    if(!tangoAdapter) {
      // FIXME: I think this means out of memory... IMHO we should just crash
      set_state(Tango::FAULT);
      set_status("Can not create TangoAdapter");
      return;
    }

    DEBUG_STREAM << "ChimeraTKExample2::init_device() end of init_device " << std::endl;

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::init_device
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::get_device_property()
   *	Description : Read database to initialize property data members.
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::get_device_property() {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::get_device_property_before) ENABLED START -----*/

    //	Initialize property data members

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::get_device_property_before

    //	Read device properties from database.
    Tango::DbData dev_prop;
    dev_prop.emplace_back("AttributList");
    dev_prop.emplace_back("DMapFilePath");

    //	is there at least one property to be read ?
    if(!dev_prop.empty()) {
      //	Call database and extract values
      if(Tango::Util::_UseDb) {
        get_db_device()->get_property(dev_prop);
      }

      //	get instance on AdapterDeviceClass to get class property
      Tango::DbDatum def_prop, cl_prop;
      auto* ds_class = dynamic_cast<AdapterDeviceClass*>(get_device_class());
      assert(ds_class != nullptr);
      int i = -1;

      //	Try to initialize AttributList from class property
      cl_prop = ds_class->get_class_property(dev_prop[++i].name);
      if(!cl_prop.is_empty()) {
        cl_prop >> attributList;
      }
      else {
        //	Try to initialize AttributList from default device value
        def_prop = ds_class->get_default_device_property(dev_prop[i].name);
        if(!def_prop.is_empty()) {
          def_prop >> attributList;
        }
      }
      //	And try to extract AttributList value from database
      if(!dev_prop[i].is_empty()) {
        dev_prop[i] >> attributList;
      }

      //	Try to initialize DMapFilePath from class property
      cl_prop = ds_class->get_class_property(dev_prop[++i].name);
      if(!cl_prop.is_empty()) {
        cl_prop >> dMapFilePath;
      }
      else {
        //	Try to initialize DMapFilePath from default device value
        def_prop = ds_class->get_default_device_property(dev_prop[i].name);
        if(!def_prop.is_empty()) {
          def_prop >> dMapFilePath;
        }
      }
      //	And try to extract DMapFilePath value from database
      if(!dev_prop[i].is_empty()) {
        dev_prop[i] >> dMapFilePath;
      }
    }

    /*----- PROTECTED REGION ID(AdapterDeviceImpl::get_device_property_after) ENABLED START -----*/

    //	Check device property data members init

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::get_device_property_after
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::always_executed_hook()
   *	Description : method always executed before any command is executed
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::always_executed_hook() {
    // DEBUG_STREAM << "AdapterDeviceImpl::always_executed_hook()  " << device_name << std::endl;
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::always_executed_hook) ENABLED START -----*/

    //	code always executed before all requests

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::always_executed_hook
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::read_attr_hardware()
   *	Description : Hardware acquisition for attributes
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::read_attr_hardware(TANGO_UNUSED(std::vector<long>& attr_list)) {
    // DEBUG_STREAM << "AdapterDeviceImpl::read_attr_hardware(vector<long> &attr_list) entering... " << std::endl;
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::read_attr_hardware) ENABLED START -----*/

    //	Add your own code

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::read_attr_hardware
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::add_dynamic_attributes()
   *	Description : Create the dynamic attributes if any
   *                for specified device.
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::add_dynamic_attributes() {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::add_dynamic_attributes) ENABLED START -----*/

    //	Add your own code to create and add dynamic attributes if any

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::add_dynamic_attributes
  }

  //--------------------------------------------------------
  /**
   *	Method      : AdapterDeviceImpl::add_dynamic_commands()
   *	Description : Create the dynamic commands if any
   *                for specified device.
   */
  //--------------------------------------------------------
  void AdapterDeviceImpl::add_dynamic_commands() {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::add_dynamic_commands) ENABLED START -----*/

    //	Add your own code to create and add dynamic commands if any

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::add_dynamic_commands
  }

  /*----- PROTECTED REGION ID(AdapterDeviceImpl::namespace_ending) ENABLED START -----*/

  //	Additional Methods

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::namespace_ending
} // namespace TangoAdapter
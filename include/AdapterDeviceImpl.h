/*----- PROTECTED REGION ID(AdapterDeviceImpl.h) ENABLED START -----*/
//=============================================================================
//
// file :        AdapterDeviceImpl.h
//
// description : Include file for the AdapterDeviceImpl class
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

#pragma once

#include "TangoAdapter.h"
#include <tango/tango.h>

// Most of the non-conforming naming is a requierment from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

/*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl.h

/**
 *  AdapterDeviceImpl class description:
 *    Test of TangoAdapterfor ChimeraTK
 */

namespace TangoAdapter {
  /*----- PROTECTED REGION ID(AdapterDeviceImpl::Additional Class Declarations) ENABLED START -----*/

  //	Additional Class Declarations

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::Additional Class Declarations

  class AdapterDeviceImpl : public TANGO_BASE_CLASS {
    /*----- PROTECTED REGION ID(AdapterDeviceImpl::Data Members) ENABLED START -----*/

    //	Add your own data members
    std::shared_ptr<ChimeraTK::TangoAdapter> tangoAdapter{};
    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::Data Members

    //	Device property data members
   public:
    //	AttributList:	AttributList
    std::vector<std::string> attributList;
    //	DMapFilePath:	DMapFilePath
    std::string dMapFilePath;

    //	Constructors and destructors
    /**
     * Constructs a newly device object.
     *
     *	@param cl	Class.
     *	@param s 	Device Name
     */
    AdapterDeviceImpl(Tango::DeviceClass* cl, std::string& s);
    /**
     * Constructs a newly device object.
     *
     *	@param cl	Class.
     *	@param s 	Device Name
     */
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s);
    /**
     * Constructs a newly device object.
     *
     *	@param cl	Class.
     *	@param s 	Device name
     *	@param d	Device description.
     */
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s, const char* d);
    /**
     * The device object destructor.
     */
    ~AdapterDeviceImpl() override { delete_device(); };

    //	Miscellaneous methods
    /*
     *	will be called at device destruction or at init command.
     */
    void delete_device() override;
    /*
     *	Initialize the device
     */
    void init_device() override;
    /*
     *	Read the device properties from database
     */
    void get_device_property();
    /*
     *	Always executed method before execution command method.
     */
    void always_executed_hook() override;

    //	Attribute methods
    //--------------------------------------------------------
    /*
     *	Method      : AdapterDeviceImpl::read_attr_hardware()
     *	Description : Hardware acquisition for attributes.
     */
    //--------------------------------------------------------
    // Disabling because this is Tango code
    // NOLINTNEXTLINE(google-runtime-int)
    void read_attr_hardware(std::vector<long>& attr_list) override;

    //--------------------------------------------------------
    /**
     *	Method      : AdapterDeviceImpl::add_dynamic_attributes()
     *	Description : Add dynamic attributes if any.
     */
    //--------------------------------------------------------
    void add_dynamic_attributes();

    //	Command related methods
    //--------------------------------------------------------
    /**
     *	Method      : AdapterDeviceImpl::add_dynamic_commands()
     *	Description : Add dynamic commands if any.
     */
    //--------------------------------------------------------
    void add_dynamic_commands();

    /*----- PROTECTED REGION ID(AdapterDeviceImpl::Additional Method prototypes) ENABLED START -----*/

    //	Additional Method prototypes

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::Additional Method prototypes
  };

  /*----- PROTECTED REGION ID(AdapterDeviceImpl::Additional Classes Definitions) ENABLED START -----*/

  //	Additional Classes Definitions

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceImpl::Additional Classes Definitions

} // namespace TangoAdapter
  // NOLINTEND(readability-identifier-naming)
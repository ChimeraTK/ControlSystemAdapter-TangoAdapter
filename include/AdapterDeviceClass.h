/*----- PROTECTED REGION ID(AdapterDeviceClass.h) ENABLED START -----*/
//=============================================================================
//
// file :        AdapterDeviceClass.h
//
// description : Include for the AdapterDeviceImpl root class.
//               This class is the singleton class for
//                the AdapterDeviceImpl device class.
//               It contains all properties and methods which the
//               AdapterDeviceImpl requires only once e.g. the commands.
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

#include "AdapterDeviceImpl.h"
#include <tango/tango.h>

// Most of the non-conforming naming is a requierment from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

/*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass.h

namespace TangoAdapter {
  /*----- PROTECTED REGION ID(AdapterDeviceClass::classes for dynamic creation) ENABLED START -----*/

  /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::classes for dynamic creation

  /**
   *	The AdapterDeviceClass singleton definition
   */

#ifdef _TG_WINDOWS_
  class __declspec(dllexport) AdapterDeviceClass : public Tango::DeviceClass
#else
  class AdapterDeviceClass : public Tango::DeviceClass
#endif
  {
    ;
    /*----- PROTECTED REGION ID(AdapterDeviceClass::Additional DServer data members) ENABLED START -----*/

    /*----- PROTECTED REGION END -----*/ //	AdapterDeviceClass::Additional DServer data members

   public:
    //	write class properties data members
    Tango::DbData cl_prop;
    Tango::DbData cl_def_prop;
    Tango::DbData dev_def_prop;

    //	Method prototypes
    static std::string getClassName();
    static AdapterDeviceClass* init(const char*);
    static AdapterDeviceClass* instance();
    ~AdapterDeviceClass() override;
    Tango::DbDatum get_class_property(std::string&);
    Tango::DbDatum get_default_device_property(std::string&);
    Tango::DbDatum get_default_class_property(std::string&);

   protected:
    explicit AdapterDeviceClass(std::string&);
    static AdapterDeviceClass* _instance;
    void command_factory() override;
    void attribute_factory(std::vector<Tango::Attr*>&) override;
    void pipe_factory() override;
    void write_class_property();
    void set_default_property();
    void get_class_property();
    std::string get_cvstag();
    std::string get_cvsroot();

   private:
    void device_factory(const Tango::DevVarStringArray*) override;
    void create_static_attribute_list(std::vector<Tango::Attr*>&);
    void erase_dynamic_attributes(const Tango::DevVarStringArray*, std::vector<Tango::Attr*>&);
    std::vector<std::string> defaultAttList;
    Tango::Attr* get_attr_object_by_name(std::vector<Tango::Attr*>& att_list, const std::string& attname);

    Tango::DbDatum getPropertyWithDefault(const Tango::DbData& list, const std::string& name);
  };

} // namespace TangoAdapter
  // NOLINTEND(readability-identifier-naming)
// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <tango/tango.h>

// Most of the non-conforming naming is a requierment from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

namespace TangoAdapter {
  /**
   *	The AdapterDeviceClass singleton definition
   */

  class AdapterDeviceClass : public Tango::DeviceClass {
   public:
    //	write class properties data members
    Tango::DbData cl_prop;
    Tango::DbData cl_def_prop;
    Tango::DbData dev_def_prop;

    //	Method prototypes
    static std::string getClassName();
    explicit AdapterDeviceClass(std::string&);
    ~AdapterDeviceClass() override = default;
    Tango::DbDatum get_class_property(std::string&);
    Tango::DbDatum get_default_device_property(std::string&);
    Tango::DbDatum get_default_class_property(std::string&);

   protected:
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
    void erase_dynamic_attributes(std::vector<Tango::Attr*>&);
    std::vector<std::string> defaultAttList;

    Tango::DbDatum getPropertyWithDefault(const Tango::DbData& list, const std::string& propertyName);
  };

} // namespace TangoAdapter
  // NOLINTEND(readability-identifier-naming)

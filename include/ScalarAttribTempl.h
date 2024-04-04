#pragma once

#include "AttributProperty.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
  template<typename TangoType, typename AdapterType>
  class ScalarAttribTempl : public Tango::Attr, Tango::LogAdapter {
   public:
    ScalarAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> pv,
        std::shared_ptr<AttributProperty> attProperty)
    : Tango::Attr(attProperty->name.c_str(), attProperty->dataType, attProperty->writeType),
      processScalar(std::move(pv)), Tango::LogAdapter(tangoDevice) {
      // memory the written value and write at initialization
      if(attProperty->writeType == Tango::READ_WRITE || attProperty->writeType == Tango::WRITE) {
        set_memorized();
        set_memorized_init(true);
      }

      Tango::UserDefaultAttrProp att_prop;
      att_prop.set_label(attProperty->name.c_str());

      att_prop.set_description(attProperty->desc.c_str());

      if(!(attProperty->unit.empty())) {
        att_prop.set_unit(attProperty->unit.c_str());
      }

      set_default_properties(att_prop);
    }

    ~ScalarAttribTempl() override = default;

    void read([[maybe_unused]] Tango::DeviceImpl* dev, Tango::Attribute& att) override {
      DEBUG_STREAM << "ScalarAttribTempl::read " << get_name() << std::endl;

      if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
        auto* value = new Tango::DevString;
        *value = Tango::string_dup(processScalar->accessData(0).c_str());
        att.set_value(value, 1, 0, true);
      }
      else if constexpr(std::is_same_v<TangoType, Tango::DevBoolean>) {
        auto pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processScalar);

        auto* value = new Tango::DevBoolean;
        if constexpr(std::is_same_v<AdapterType, Void>) {
          *value = true;
        }
        else {
          *value = processScalar->accessData(0);
        }
        att.set_value(value, 1, 0, true);
      }
      else {
        TangoType* value = new TangoType;
        *value = processScalar->accessData(0);
        att.set_value(value, 1, 0, true);
      }

      if(processScalar->dataValidity() != ChimeraTK::DataValidity::ok) {
        att.set_quality(Tango::AttrQuality::ATTR_INVALID);
        ERROR_STREAM << "ScalarAttribTempl::read " << get_name() << " is not valid" << std::endl;
      }
      else {
        att.set_quality(Tango::AttrQuality::ATTR_VALID);
      }
    }

    void write([[maybe_unused]] Tango::DeviceImpl* dev, Tango::WAttribute& att) override {
      DEBUG_STREAM << "ScalarAttribTempl::write " << get_name() << std::endl;

      if constexpr(std::is_same_v<AdapterType, std::string>) {
        Tango::DevString st_value;
        att.get_write_value(st_value);
        processScalar->accessData(0) = std::string(st_value);
      }
      else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Boolean>) {
        auto pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processScalar);
        Tango::DevBoolean b_value;
        att.get_write_value(b_value);
        pv->accessData(0) = b_value;
      }
      else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
        // do nothing, just write the accessor
      }
      else {
        TangoType value;
        att.get_write_value(value);
        processScalar->accessData(0) = value;
      }

      if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID) {
        processScalar->setDataValidity(DataValidity::faulty);
      }
      else {
        processScalar->setDataValidity(DataValidity::ok);
      }
      processScalar->write();
    }

    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    };

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> processScalar;
  };

} // namespace ChimeraTK

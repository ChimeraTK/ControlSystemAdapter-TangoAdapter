#pragma once

#include "AttributProperty.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
  template<typename T>
  class ScalarAttribTempl : public Tango::Attr, Tango::LogAdapter {
   public:
    ScalarAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv,
        std::shared_ptr<AttributProperty> attProperty)
    : Tango::Attr(attProperty->name.c_str(), attProperty->dataType, attProperty->writeType),
      processScalar(std::move(pv)), dataType(attProperty->dataType), Tango::LogAdapter(tangoDevice) {
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

      if constexpr(std::is_same<T, std::string>::value) {
        // Necessary because Tango API wants a plain char* - it will copy the data internally
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        auto* stringAttrHelper = const_cast<char*>(processScalar->accessData(0).c_str());
        att.set_value(&stringAttrHelper);
      }
      else if(dataType == Tango::DEV_BOOLEAN) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<Boolean>> pv =
            boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processScalar);

        Tango::DevBoolean boolAttrHelper = processScalar->accessData(0);
        att.set_value(&boolAttrHelper);
      }
      else {
        att.set_value(&processScalar->accessData(0));
      }

      if(processScalar->dataValidity() != ChimeraTK::DataValidity::ok) {
        ERROR_STREAM << "ScalarAttribTempl::read " << get_name() << " is not valid" << std::endl;
      }
    }

    void write([[maybe_unused]] Tango::DeviceImpl* dev, Tango::WAttribute& att) override {
      DEBUG_STREAM << "ScalarAttribTempl::write " << get_name() << std::endl;

      switch(dataType) {
        case Tango::DEV_UCHAR: {
          uint8_t c_value;
          att.get_write_value(c_value);
          processScalar->accessData(0) = c_value;
          break;
        }
        case Tango::DEV_USHORT: {
          uint16_t us_value;
          att.get_write_value(us_value);
          processScalar->accessData(0) = us_value;
          break;
        }
        case Tango::DEV_ULONG: {
          uint32_t ul_value;
          att.get_write_value(ul_value);
          processScalar->accessData(0) = ul_value;
          break;
        }
        case Tango::DEV_ULONG64: {
          uint64_t ul64_value;
          att.get_write_value(ul64_value);
          processScalar->accessData(0) = ul64_value;
          break;
        }
        case Tango::DEV_SHORT: {
          int16_t s_value;
          att.get_write_value(s_value);
          processScalar->accessData(0) = s_value;
          break;
        }
        case Tango::DEV_LONG: {
          int32_t l_value;
          att.get_write_value(l_value);
          processScalar->accessData(0) = l_value;
          break;
        }
        case Tango::DEV_LONG64: {
          int64_t l64_value;
          att.get_write_value(l64_value);
          processScalar->accessData(0) = l64_value;
          break;
        }
        case Tango::DEV_FLOAT: {
          float f_value;
          att.get_write_value(f_value);
          processScalar->accessData(0) = f_value;
          break;
        }
        case Tango::DEV_DOUBLE: {
          double d_value;
          att.get_write_value(d_value);
          processScalar->accessData(0) = d_value;
          break;
        }
        case Tango::DEV_BOOLEAN: {
          auto pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processScalar);
          bool b_value;
          att.get_write_value(b_value);
          pv->accessData(0) = b_value;
          break;
        }
        case Tango::DEV_STRING:
          if constexpr(std::is_same<T, std::string>::value) {
            Tango::DevString st_value;
            att.get_write_value(st_value);
            processScalar->accessData(0) = std::string(st_value);
          }

          break;
        default:

          break;
      }
      processScalar->write();
    }

    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    };

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> processScalar;
    Tango::CmdArgType dataType;
  };

} // namespace ChimeraTK

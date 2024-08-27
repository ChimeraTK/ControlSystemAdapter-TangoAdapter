#pragma once

#include <tango/tango.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <ChimeraTK/Exception.h>

namespace ChimeraTK {
    inline Tango::CmdArgType deriveDataType(Tango::CmdArgType& inType) {
      switch(inType) {
        case Tango::DEV_ENUM:
          // FIXME: HACK. There is no CHAR type in Tango, we use ENUM and map to SHORT
          return Tango::DEV_SHORT;
        case Tango::DEV_VOID:
          // DEV_VOID is not a type. It is usually used to signify that a Command does not have a parameter
          // or a return value. Hence we need to map it to something else, let's choose boolean for that
          return Tango::DEV_BOOLEAN;
        default:
          return inType;
      }
    }
  enum AttrDataFormat { SCALAR, SPECTRUM, IMAGE };

  struct AttributeProperty {
    // Speed;Board/Reg;SCALAR;DEVShort
    AttributeProperty(std::string attributeName, AttrDataFormat dataFormat,
        Tango::CmdArgType attrDataType, std::string attrDesc, std::string attrUnit)
    : unit(std::move(attrUnit)), desc(std::move(attrDesc)), name(std::move(attributeName)),
      attrDataFormat(dataFormat), dataType(attrDataType) {}

    ~AttributeProperty() = default;

    void operator=(AttributeProperty const&) = delete;

    std::unique_ptr<Tango::Attr> toTangoAttribute();

    std::string unit;
    std::string desc;
    std::string name;
    size_t length{0};

    ChimeraTK::AttrDataFormat attrDataFormat{};
    Tango::CmdArgType dataType{Tango::DATA_TYPE_UNKNOWN};
    Tango::AttrWriteType writeType{Tango::AttrWriteType::WT_UNKNOWN};
  };
} // namespace ChimeraTK

namespace std {
  inline std::ostream& operator<<(std::ostream& os, const ChimeraTK::AttributeProperty& prop) {
    os << "Dumping AttributeProperty " << prop.name << std::endl;
    os << "   unit: " << prop.unit << "\n"
       << "   desc: " << prop.desc << "\n"
       << "   length: " << prop.length << "\n"
       << "   format: " << prop.attrDataFormat << "\n"
       << "   type: " << prop.dataType << "\n"
       << "   writeType: " << prop.writeType << std::endl;

    return os;
  }
} // namespace std

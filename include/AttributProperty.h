#pragma once

#include <tango/tango.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <map>
#define TOKEN ";"

namespace ChimeraTK {
  enum AttrDataFormat { SCALAR, SPECTRUM, IMAGE };

  struct AttributProperty {
    // Speed;Board/Reg;SCALAR;DEVShort
    AttributProperty(std::string name, std::string path, AttrDataFormat dataFormat, Tango::CmdArgType dataType,
        std::string desc, std::string unit)
    : name(std::move(name)), path(std::move(path)), attrDataFormat(dataFormat), dataType(dataType),
      desc(std::move(desc)), unit(std::move(unit)) {}

    explicit AttributProperty(std::string desc) {
      std::vector<std::string> splitDesc;
      boost::algorithm::split(splitDesc, desc, boost::is_any_of(TOKEN));
      if(splitDesc.size() != 6) {
        std::cout << "error AttributProperty" << std::endl;
      }
      name = splitDesc[0];
      path = splitDesc[1];
      attrDataFormat = regTypeMap[splitDesc[2]];

      if(splitDesc[3] == "DevUChar") {
        dataType = Tango::DEV_UCHAR;
      }
      if(splitDesc[3] == "DevUShort") {
        dataType = Tango::DEV_USHORT;
      }
      else if(splitDesc[3] == "DevULong") {
        dataType = Tango::DEV_ULONG;
      }
      else if(splitDesc[3] == "DevULong64") {
        dataType = Tango::DEV_ULONG64;
      }
      else if(splitDesc[3] == "DevShort") {
        dataType = Tango::DEV_SHORT;
      }
      else if(splitDesc[3] == "DevLong") {
        dataType = Tango::DEV_LONG;
      }
      else if(splitDesc[3] == "DevLong64") {
        dataType = Tango::DEV_LONG64;
      }
      else if(splitDesc[3] == "DevFloat") {
        dataType = Tango::DEV_FLOAT;
      }
      else if(splitDesc[3] == "DevDouble") {
        dataType = Tango::DEV_DOUBLE;
      }
      else if(splitDesc[3] == "DevBoolean") {
        dataType = Tango::DEV_BOOLEAN;
      }
      else if(splitDesc[3] == "DevString") {
        dataType = Tango::DEV_STRING;
      }

      desc = splitDesc[4];
      unit = splitDesc[5];
    }

    ~AttributProperty() = default;

    void operator=(AttributProperty const&) = delete;

    std::map<std::string, ChimeraTK::AttrDataFormat> regTypeMap = {{"SCALAR", ChimeraTK::AttrDataFormat::SCALAR},
        {"SPECTRUM", ChimeraTK::AttrDataFormat::SPECTRUM}, {"IMAGE", ChimeraTK::AttrDataFormat::IMAGE}};

    std::string unit;
    std::string desc;
    std::string name;
    std::string path;

    ChimeraTK::AttrDataFormat attrDataFormat;
    Tango::CmdArgType dataType;
    Tango::AttrWriteType writeType{Tango::AttrWriteType::WT_UNKNOWN};
  };

} // namespace ChimeraTK

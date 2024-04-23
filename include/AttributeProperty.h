#pragma once

#include <tango/tango.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <ChimeraTK/Exception.h>

#include <map>
#define TOKEN ";"

namespace ChimeraTK {
  enum AttrDataFormat { SCALAR, SPECTRUM, IMAGE };

  struct AttributeProperty {
    // Speed;Board/Reg;SCALAR;DEVShort
    AttributeProperty(std::string attributeName, std::string variablePath, AttrDataFormat dataFormat,
        Tango::CmdArgType attrDataType, std::string attrDesc, std::string attrUnit)
    : unit(std::move(attrUnit)), desc(std::move(attrDesc)), name(std::move(attributeName)),
      path(std::move(variablePath)), attrDataFormat(dataFormat), dataType(attrDataType) {}

    explicit AttributeProperty(std::string attrDesc) {
      std::vector<std::string> splitDesc;
      boost::algorithm::split(splitDesc, attrDesc, boost::is_any_of(TOKEN));
      if(splitDesc.size() != 6) {
        throw ChimeraTK::runtime_error("Error parsing AttributeProperty: " + attrDesc);
      }
      name = splitDesc[0];
      path = splitDesc[1];
      try {
        attrDataFormat = regTypeMap.at(splitDesc[2]);
      } catch (std::out_of_range&) {
        throw ChimeraTK::runtime_error("Invalid accessor format: " + splitDesc[2]);
      }

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
      else if(splitDesc[3] == "DevVoid") {
        dataType = Tango::DEV_VOID;
      }

      if (dataType == Tango::DATA_TYPE_UNKNOWN) {
        throw ChimeraTK::runtime_error("Could not parse type name: " + splitDesc[3]);
      }

      desc = splitDesc[4];
      unit = splitDesc[5];
    }

    ~AttributeProperty() = default;

    void operator=(AttributeProperty const&) = delete;

    std::map<std::string, ChimeraTK::AttrDataFormat> regTypeMap = {{"SCALAR", ChimeraTK::AttrDataFormat::SCALAR},
        {"SPECTRUM", ChimeraTK::AttrDataFormat::SPECTRUM}, {"IMAGE", ChimeraTK::AttrDataFormat::IMAGE}};

    std::string unit{};
    std::string desc{};
    std::string name{};
    std::string path{};

    ChimeraTK::AttrDataFormat attrDataFormat{};
    Tango::CmdArgType dataType{Tango::DATA_TYPE_UNKNOWN};
    Tango::AttrWriteType writeType{Tango::AttrWriteType::WT_UNKNOWN};
  };

} // namespace ChimeraTK

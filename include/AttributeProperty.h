// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <tango/tango.h>

#include <ChimeraTK/Exception.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace TangoAdapter {

  enum class AttrDataFormat { SCALAR, SPECTRUM, IMAGE };

  struct AttributeProperty {
    enum class NotificationType { none, change, data_ready };

    AttributeProperty(std::string attributeName, AttrDataFormat dataFormat, Tango::CmdArgType attrDataType,
        std::string attrDesc, std::string attrUnit)
    : unit(std::move(attrUnit)), desc(std::move(attrDesc)), name(std::move(attributeName)), attrDataFormat(dataFormat),
      dataType(attrDataType) {}

    ~AttributeProperty() = default;

    void operator=(AttributeProperty const&) = delete;

    std::unique_ptr<Tango::Attr> toTangoAttribute();
    [[nodiscard]] Tango::CmdArgType getDataType() const;

    std::string unit;
    std::string desc;
    std::string name;
    size_t length{0};
    NotificationType notificationType{NotificationType::none};

    AttrDataFormat attrDataFormat{};
    Tango::CmdArgType dataType{Tango::DATA_TYPE_UNKNOWN};
    Tango::AttrWriteType writeType{Tango::AttrWriteType::WT_UNKNOWN};
  };
} // namespace TangoAdapter

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

namespace std {
  inline std::ostream& operator<<(std::ostream& os, const TangoAdapter::AttrDataFormat& f) {
    switch(f) {
      case TangoAdapter::AttrDataFormat::SCALAR:
        os << "SCALAR";
        break;
      case TangoAdapter::AttrDataFormat::SPECTRUM:
        os << "SCALAR";
        break;
      case TangoAdapter::AttrDataFormat::IMAGE:
        os << "SCALAR";
        break;
      default:
        os << "UNKNOWN";
        assert(false);
        break;
    }

    return os;
  }

  /********************************************************************************************************************/

  inline std::ostream& operator<<(std::ostream& os, const TangoAdapter::AttributeProperty& prop) {
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

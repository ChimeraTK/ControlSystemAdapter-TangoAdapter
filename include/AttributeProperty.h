// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/Exception.h>

#include <tango/tango.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <AttributeMappingDescription.h>

namespace TangoAdapter {

  struct AttributeProperty {
    // Speed;Board/Reg;SCALAR;DEVShort
    explicit AttributeProperty(AttributeMappingDescription attrDescription) : description(std::move(attrDescription)) {
      TANGO_LOG_DEBUG << "Hello World" << std::endl;
    }

    ~AttributeProperty() = default;

    void operator=(AttributeProperty const&) = delete;

    std::unique_ptr<Tango::Attr> toTangoAttribute();
    [[nodiscard]] Tango::CmdArgType getDataType() const;

    AttributeMappingDescription description;
    size_t length{0};

    Tango::CmdArgType dataType{Tango::DATA_TYPE_UNKNOWN};
    Tango::AttrWriteType writeType{Tango::AttrWriteType::WT_UNKNOWN};
  };
} // namespace TangoAdapter

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

/********************************************************************************************************************/

namespace std {
  inline std::ostream& operator<<(std::ostream& os, const TangoAdapter::AttributeProperty& prop) {
    os << "Dumping AttributeProperty " << prop.description.name << std::endl;
    os << "   unit: " << prop.description.unit.value_or("unset") << "\n"
       << "   desc: " << prop.description.description.value_or("unset") << "\n"
       << "   length: " << prop.length << "\n"
       << "   format: " << prop.description.dataLayout << "\n"
       << "   type: " << prop.dataType << "\n"
       << "   writeType: " << prop.writeType << std::endl;

    return os;
  }
} // namespace std

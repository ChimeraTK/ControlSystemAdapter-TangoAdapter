// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// The Tango command and value types below all come from the umbrella header, which clang-tidy cannot attribute.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tango/tango.h>

#include <ostream>
#include <string>

namespace TangoAdapter {

  /**
   * The Tango-facing description of one command: its name, its input and return type and the two descriptions shown
   * to a client. A command without an argument or without a return value carries DEV_VOID in that slot.
   */
  struct CommandProperty {
    CommandProperty() = default;

    CommandProperty(std::string commandName, Tango::CmdArgType inputType, Tango::CmdArgType outputType,
        std::string inputDescription, std::string outputDescription)
    : name(std::move(commandName)), inType(inputType), outType(outputType), inDesc(std::move(inputDescription)),
      outDesc(std::move(outputDescription)) {}

    std::string name;
    Tango::CmdArgType inType{Tango::DATA_TYPE_UNKNOWN};
    Tango::CmdArgType outType{Tango::DATA_TYPE_UNKNOWN};
    std::string inDesc;
    std::string outDesc;
  };

} // namespace TangoAdapter

/**********************************************************************************************************************/

namespace std {
  inline std::ostream& operator<<(std::ostream& os, const TangoAdapter::CommandProperty& prop) {
    os << "Dumping CommandProperty " << prop.name << std::endl;
    os << "   inType: " << prop.inType << "\n"
       << "   outType: " << prop.outType << "\n"
       << "   inDesc: " << prop.inDesc << "\n"
       << "   outDesc: " << prop.outDesc << std::endl;

    return os;
  }
} // namespace std

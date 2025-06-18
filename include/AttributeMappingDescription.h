// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/RegisterPath.h>

#include <tango/tango.h>

#include <cassert>
#include <optional>
#include <ostream>
#include <string>

namespace TangoAdapter {
  enum class AttributeEventing { NONE, DATA_READY, DATA };
  enum class AttributeDataLayout { SCALAR, SPECTRUM, IMAGE };

  struct AttributeMappingDescription {
    std::string name;
    ChimeraTK::RegisterPath source;
    std::optional<std::string> description;
    std::optional<std::string> unit;
    AttributeEventing attributeEventing{AttributeEventing::NONE};
    AttributeDataLayout dataLayout{AttributeDataLayout::SCALAR};
  };

} // namespace TangoAdapter

namespace std {
  inline std::ostream& operator<<(std::ostream& os, const TangoAdapter::AttributeDataLayout& f) {
    switch(f) {
      case TangoAdapter::AttributeDataLayout::SCALAR:
        os << "SCALAR";
        break;
      case TangoAdapter::AttributeDataLayout::SPECTRUM:
        os << "SPECTRUM";
        break;
      case TangoAdapter::AttributeDataLayout::IMAGE:
        os << "IMAGE";
        break;
      default:
        os << "UNKNOWN";
        assert(false);
        break;
    }

    return os;
  }
} // namespace std

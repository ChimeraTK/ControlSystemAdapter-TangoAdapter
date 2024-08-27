// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AttributeProperty.h"

#include "SpectrumAttribTempl.h"
#include "ScalarAttribTempl.h"

namespace util {
  std::unique_ptr<Tango::Attr> toScalarTangoAttribute(ChimeraTK::AttributeProperty& attProp) {

    switch(attProp.dataType) {
      case Tango::DEV_UCHAR: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevUChar, uint8_t>>(attProp);
      }
      case Tango::DEV_ENUM: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevShort, int8_t>>(attProp);
      }
      case Tango::DEV_USHORT: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevUShort, uint16_t>>(attProp);
      }
      case Tango::DEV_SHORT: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevShort, int16_t>>(attProp);
      }
      case Tango::DEV_DOUBLE: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevDouble, double>>(attProp);
      }
      case Tango::DEV_FLOAT: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevFloat, float>>(attProp);
      }
      case Tango::DEV_ULONG64: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevULong64, uint64_t>>(attProp);
      }
      case Tango::DEV_LONG64: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevLong64, int64_t>>(attProp);
      }
      case Tango::DEV_ULONG: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevULong, uint32_t>>(attProp);
      }
      case Tango::DEV_LONG: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevLong, int32_t>>(attProp);
      }
      case Tango::DEV_STRING: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevString, std::string>>(attProp);
      }
      case Tango::DEV_BOOLEAN: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>>(attProp);
      }
      case Tango::DEV_VOID: {
        return std::make_unique<ChimeraTK::ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Void>>(attProp);
      }
      default:
        throw ChimeraTK::logic_error("Unexpected data type id in scalar attribute");
    }
  }

  std::unique_ptr<Tango::Attr> toSpectrumTangoAttribute(ChimeraTK::AttributeProperty& attProp) {
    switch(attProp.dataType) {
      case Tango::DEV_UCHAR: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevUChar, uint8_t>>(attProp);
      }
      // FIXME: HACK. There is no CHAR type in tango
      case Tango::DEV_ENUM: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevShort, int8_t>>(attProp);
      }
      case Tango::DEV_USHORT: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevUShort, uint16_t>>(attProp);
      }
      case Tango::DEV_SHORT: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevShort, int16_t>>(attProp);
      }
      case Tango::DEV_ULONG: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevULong, uint32_t>>(attProp);
      }
      case Tango::DEV_LONG: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevLong, int32_t>>(attProp);
      }
      case Tango::DEV_DOUBLE: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevDouble, double>>(attProp);
      }
      case Tango::DEV_FLOAT: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevFloat, float>>(attProp);
      }
      case Tango::DEV_ULONG64: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevULong64, uint64_t>>(attProp);
      }
      case Tango::DEV_LONG64: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevLong64, int64_t>>(attProp);
      }
      case Tango::DEV_STRING: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevString, std::string, Tango::ConstDevString>>(attProp);
      }
      case Tango::DEV_BOOLEAN: {
        return std::make_unique<ChimeraTK::SpectrumAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>>(attProp);
      }
      default:
        throw ChimeraTK::logic_error("Unexpected data type id in spectrum attribute");
    }
  }
} // namespace util

namespace ChimeraTK {
  std::unique_ptr<Tango::Attr> AttributeProperty::toTangoAttribute() {
    switch(attrDataFormat) {
      case ChimeraTK::SCALAR:
        return util::toScalarTangoAttribute(*this);
      case ChimeraTK::SPECTRUM:
        return util::toSpectrumTangoAttribute(*this);
      default:
        throw ChimeraTK::logic_error("Unsupported attribute type in mapper");
    }

  }

} // namespace ChimeraTK

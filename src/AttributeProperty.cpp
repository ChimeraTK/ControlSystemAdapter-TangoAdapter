// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AttributeProperty.h"

#include "ScalarAttribTempl.h"
#include "SpectrumAttribTempl.h"

namespace TangoAdapter {
  namespace util {
    std::unique_ptr<Tango::Attr> toScalarTangoAttribute(TangoAdapter::AttributeProperty& attProp) {
      switch(attProp.dataType) {
        case Tango::DEV_UCHAR: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevUChar, uint8_t>>(attProp);
        }
        case Tango::DEV_ENUM: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevShort, int8_t>>(attProp);
        }
        case Tango::DEV_USHORT: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevUShort, uint16_t>>(attProp);
        }
        case Tango::DEV_SHORT: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevShort, int16_t>>(attProp);
        }
        case Tango::DEV_DOUBLE: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevDouble, double>>(attProp);
        }
        case Tango::DEV_FLOAT: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevFloat, float>>(attProp);
        }
        case Tango::DEV_ULONG64: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevULong64, uint64_t>>(attProp);
        }
        case Tango::DEV_LONG64: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevLong64, int64_t>>(attProp);
        }
        case Tango::DEV_ULONG: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevULong, uint32_t>>(attProp);
        }
        case Tango::DEV_LONG: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevLong, int32_t>>(attProp);
        }
        case Tango::DEV_STRING: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevString, std::string>>(attProp);
        }
        case Tango::DEV_BOOLEAN: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>>(attProp);
        }
        case Tango::DEV_VOID: {
          return std::make_unique<TangoAdapter::ScalarAttribTempl<Tango::DevBoolean, ChimeraTK::Void>>(attProp);
        }
        default:
          throw ChimeraTK::logic_error("Unexpected data type id in scalar attribute");
      }
    }

    /******************************************************************************************************************/

    std::unique_ptr<Tango::Attr> toSpectrumTangoAttribute(TangoAdapter::AttributeProperty& attProp) {
      switch(attProp.dataType) {
        case Tango::DEV_UCHAR: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevUChar, uint8_t>>(attProp);
        }
        // FIXME: HACK. There is no CHAR type in tango
        case Tango::DEV_ENUM: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevShort, int8_t>>(attProp);
        }
        case Tango::DEV_USHORT: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevUShort, uint16_t>>(attProp);
        }
        case Tango::DEV_SHORT: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevShort, int16_t>>(attProp);
        }
        case Tango::DEV_ULONG: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevULong, uint32_t>>(attProp);
        }
        case Tango::DEV_LONG: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevLong, int32_t>>(attProp);
        }
        case Tango::DEV_DOUBLE: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevDouble, double>>(attProp);
        }
        case Tango::DEV_FLOAT: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevFloat, float>>(attProp);
        }
        case Tango::DEV_ULONG64: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevULong64, uint64_t>>(attProp);
        }
        case Tango::DEV_LONG64: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevLong64, int64_t>>(attProp);
        }
        case Tango::DEV_STRING: {
          return std::make_unique<
              TangoAdapter::SpectrumAttribTempl<Tango::DevString, std::string, Tango::ConstDevString>>(attProp);
        }
        case Tango::DEV_BOOLEAN: {
          return std::make_unique<TangoAdapter::SpectrumAttribTempl<Tango::DevBoolean, ChimeraTK::Boolean>>(attProp);
        }
        default:
          throw ChimeraTK::logic_error("Unexpected data type id in spectrum attribute");
      }
    }
  } // namespace util

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  Tango::CmdArgType AttributeProperty::getDataType() const {
    switch(dataType) {
      case Tango::DEV_ENUM:
        // FIXME: HACK. There is no CHAR type in Tango, we use ENUM and map to SHORT
        return Tango::DEV_SHORT;
      case Tango::DEV_VOID:
        // DEV_VOID is not a type. It is usually used to signify that a Command does not have a parameter
        // or a return value. Hence we need to map it to something else, let's choose boolean for that
        return Tango::DEV_BOOLEAN;
      default:
        return dataType;
    }
  }

  /********************************************************************************************************************/

  std::unique_ptr<Tango::Attr> AttributeProperty::toTangoAttribute() {
    switch(attrDataFormat) {
      case AttrDataFormat::SCALAR:
        return util::toScalarTangoAttribute(*this);
      case AttrDataFormat::SPECTRUM:
        return util::toSpectrumTangoAttribute(*this);
      default:
        throw ChimeraTK::logic_error("Unsupported attribute type in mapper");
    }
  }

} // namespace TangoAdapter

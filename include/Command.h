// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ChimeraTK/TransferElementAbstractor.h"
#include "CommandProperty.h"

// The Tango command and value types this header uses all come from the umbrella header; clang-tidy cannot attribute.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tango/tango.h>

#include <string>

// Most of the non-conforming naming is a requirement from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

namespace TangoAdapter {

  class AdapterDeviceImpl;

  /**
   * The Tango scalar and 1D-array types a supported adapter value type travels in. Primary template is intentionally
   * incomplete: the mapper rejects every value type without a Tango command type, so only the supported ones are
   * instantiated.
   */
  template<typename AdapterType>
  struct TangoCommandType;

  template<>
  struct TangoCommandType<ChimeraTK::Boolean> {
    using Scalar = Tango::DevBoolean;
    using Array = Tango::DevVarBooleanArray;
  };
  template<>
  struct TangoCommandType<uint8_t> {
    using Scalar = Tango::DevUChar;
    using Array = Tango::DevVarCharArray;
  };
  template<>
  struct TangoCommandType<int8_t> {
    using Scalar = Tango::DevShort;
    using Array = Tango::DevVarShortArray;
  };
  template<>
  struct TangoCommandType<uint16_t> {
    using Scalar = Tango::DevUShort;
    using Array = Tango::DevVarUShortArray;
  };
  template<>
  struct TangoCommandType<int16_t> {
    using Scalar = Tango::DevShort;
    using Array = Tango::DevVarShortArray;
  };
  template<>
  struct TangoCommandType<uint32_t> {
    using Scalar = Tango::DevULong;
    using Array = Tango::DevVarULongArray;
  };
  template<>
  struct TangoCommandType<int32_t> {
    using Scalar = Tango::DevLong;
    using Array = Tango::DevVarLongArray;
  };
  template<>
  struct TangoCommandType<uint64_t> {
    using Scalar = Tango::DevULong64;
    using Array = Tango::DevVarULong64Array;
  };
  template<>
  struct TangoCommandType<int64_t> {
    using Scalar = Tango::DevLong64;
    using Array = Tango::DevVarLong64Array;
  };
  template<>
  struct TangoCommandType<float> {
    using Scalar = Tango::DevFloat;
    using Array = Tango::DevVarFloatArray;
  };
  template<>
  struct TangoCommandType<double> {
    using Scalar = Tango::DevDouble;
    using Array = Tango::DevVarDoubleArray;
  };
  template<>
  struct TangoCommandType<std::string> {
    using Scalar = Tango::DevString;
    using Array = Tango::DevVarStringArray;
  };

  /**
   * One Tango command of a device class. It is stateless: the process variables and the wait state live in the device
   * instance it is called on, so the same command object serves every device instance of the class.
   */
  class Command : public Tango::Command {
   public:
    explicit Command(const CommandProperty& commandProperty);

    CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) override;
    bool is_allowed(Tango::DeviceImpl* dev, const CORBA::Any& in_any) override;

   private:
    CommandProperty _commandProperty;

    void writeInputArgument(AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any,
        ChimeraTK::TransferElementAbstractor& inputAccessor);
    CORBA::Any* buildReturnValue(
        AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor);

    // Copy one scalar or one 1D array of the given adapter type from the incoming Any into the input accessor.
    template<typename AdapterType>
    void writeScalar(AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any,
        ChimeraTK::TransferElementAbstractor& inputAccessor);
    template<typename AdapterType>
    void writeArray(AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any,
        ChimeraTK::TransferElementAbstractor& inputAccessor);

    // Build the Tango answer from the return accessor as a scalar or a 1D array of the given adapter type.
    template<typename AdapterType>
    CORBA::Any* insertScalar(AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor);
    template<typename AdapterType>
    CORBA::Any* insertArray(AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor);
  };
} // namespace TangoAdapter

// NOLINTEND(readability-identifier-naming)

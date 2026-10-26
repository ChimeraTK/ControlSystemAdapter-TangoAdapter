// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "Command.h"

#include "AdapterDeviceImpl.h"
#include "TangoPropertyHelper.h"

#include <format>
#include <string>
#include <vector>

// Most of the non-conforming naming is a requirement from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

namespace TangoAdapter {

  /********************************************************************************************************************/

  Command::Command(const CommandProperty& commandProperty)
  : Tango::Command(commandProperty.name, commandProperty.inType, commandProperty.outType, commandProperty.inDesc,
        commandProperty.outDesc),
    _commandProperty(commandProperty) {}

  /********************************************************************************************************************/

  bool Command::is_allowed(Tango::DeviceImpl* dev, const CORBA::Any& /*in_any*/) {
    auto* adapterDevice = dynamic_cast<AdapterDeviceImpl*>(dev);
    if(adapterDevice == nullptr) {
      return false;
    }

    // A device instance which does not declare this command rejects the call as not allowed, so a call on another
    // device instance of the class fails with API_CommandNotAllowed.
    return adapterDevice->hasCommand(get_name());
  }

  /********************************************************************************************************************/

  CORBA::Any* Command::execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) {
    auto* adapterDevice = dynamic_cast<AdapterDeviceImpl*>(dev);
    if(adapterDevice == nullptr) {
      Tango::Except::throw_exception("API_CommandNotAllowed",
          std::format("Command {} is not allowed on this device", get_name()).c_str(), "Command::execute()");
    }

    auto state = adapterDevice->getCommandState(_commandProperty.name);
    if(!state) {
      Tango::Except::throw_exception("API_CommandNotAllowed",
          std::format("Command {} is not allowed on device {}", get_name(), dev->get_name()).c_str(),
          "Command::execute()");
    }

    // The newest version number the command's own writes received. The wait requires the return value to post-date it,
    // so an unrelated update of the return-value process variable which pre-dates the write does not end the wait.
    ChimeraTK::VersionNumber writeVersion(nullptr);

    // The argument is written first, so the application sees it before the trigger.
    if(state->inputArgument.isInitialised()) {
      writeInputArgument(*adapterDevice, in_any, state->inputArgument);
      writeVersion = state->inputArgument.getVersionNumber();
    }

    if(state->trigger.isInitialised()) {
      // A ChimeraTK::Void accessor carries no data; writing it advances its version number only, exactly as the void
      // attribute write does.
      auto voidAccessor = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<ChimeraTK::Void>>(
          state->trigger.getHighLevelImplElement());
      if(!voidAccessor) {
        Tango::Except::throw_exception("API_CommandNotAllowed",
            std::format("Command {}: trigger is not a ChimeraTK::Void process variable", _commandProperty.name).c_str(),
            "Command::execute()");
      }
      voidAccessor->setDataValidity(ChimeraTK::DataValidity::ok);
      voidAccessor->write();
      // A trigger-only command takes the version of its void trigger write, which is newer than the argument write.
      writeVersion = voidAccessor->getVersionNumber();
    }

    if(!state->returnValue.isInitialised()) {
      // Tango takes ownership of the returned Any.
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      return new CORBA::Any();
    }

    AdapterDeviceImpl::waitForCommandReturnValue(*state, writeVersion);

    if(state->returnValue.dataValidity() != ChimeraTK::DataValidity::ok) {
      // A Tango command has no quality channel, so an invalid return value has to fail the call. The reason names the
      // faulty return value, so a client can tell it apart from a rejection of the command itself (which carries
      // API_CommandNotAllowed).
      Tango::Except::throw_exception("API_DataNotValid",
          std::format("Command {}: process variable {} carries no valid return value", _commandProperty.name,
              state->returnValue.getName())
              .c_str(),
          "Command::execute()");
    }

    return buildReturnValue(*adapterDevice, state->returnValue);
  }

  /********************************************************************************************************************/

  void Command::writeInputArgument(
      AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any, ChimeraTK::TransferElementAbstractor& inputAccessor) {
    // Dispatch on the Tango input type. The mapper derived this type from the process variable, so the extracted
    // value's adapter type matches the accessor's value type.
    switch(_commandProperty.inType) {
      case Tango::DEV_BOOLEAN:
        writeScalar<ChimeraTK::Boolean>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_USHORT:
        writeScalar<uint16_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_SHORT:
        writeScalar<int16_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_ULONG:
        writeScalar<uint32_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_LONG:
        writeScalar<int32_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_ULONG64:
        writeScalar<uint64_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_LONG64:
        writeScalar<int64_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_FLOAT:
        writeScalar<float>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_DOUBLE:
        writeScalar<double>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEV_STRING:
        writeScalar<std::string>(deviceAdapter, in_any, inputAccessor);
        return;

      case Tango::DEVVAR_CHARARRAY:
        writeArray<uint8_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_BOOLEANARRAY:
        writeArray<ChimeraTK::Boolean>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_USHORTARRAY:
        writeArray<uint16_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_SHORTARRAY:
        writeArray<int16_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_ULONGARRAY:
        writeArray<uint32_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_LONGARRAY:
        writeArray<int32_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_ULONG64ARRAY:
        writeArray<uint64_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_LONG64ARRAY:
        writeArray<int64_t>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_FLOATARRAY:
        writeArray<float>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_DOUBLEARRAY:
        writeArray<double>(deviceAdapter, in_any, inputAccessor);
        return;
      case Tango::DEVVAR_STRINGARRAY:
        writeArray<std::string>(deviceAdapter, in_any, inputAccessor);
        return;

      default:
        Tango::Except::throw_exception("API_CommandNotAllowed",
            std::format("Command {}: unsupported input type", _commandProperty.name).c_str(), "Command::execute()");
    }
  }

  /********************************************************************************************************************/

  template<typename AdapterType>
  void Command::writeScalar(
      AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any, ChimeraTK::TransferElementAbstractor& inputAccessor) {
    using Scalar = TangoCommandType<AdapterType>::Scalar;

    Scalar value{};
    extract(in_any, value);
    deviceAdapter.writeCommandInputValues(
        inputAccessor, std::vector<AdapterType>{convertFromTangoType<AdapterType>(value)});
  }

  /********************************************************************************************************************/

  template<typename AdapterType>
  void Command::writeArray(
      AdapterDeviceImpl& deviceAdapter, const CORBA::Any& in_any, ChimeraTK::TransferElementAbstractor& inputAccessor) {
    using Array = TangoCommandType<AdapterType>::Array;

    const Array* data = nullptr;
    extract(in_any, data);

    std::vector<AdapterType> values(data->length());
    for(unsigned i = 0; i < data->length(); ++i) {
      values[i] = convertFromTangoType<AdapterType>((*data)[i]);
    }

    deviceAdapter.writeCommandInputValues(inputAccessor, values);
  }

  /********************************************************************************************************************/

  CORBA::Any* Command::buildReturnValue(
      AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor) {
    switch(_commandProperty.outType) {
      case Tango::DEV_BOOLEAN:
        return insertScalar<ChimeraTK::Boolean>(deviceAdapter, returnAccessor);
      case Tango::DEV_UCHAR:
        return insertScalar<uint8_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_USHORT:
        return insertScalar<uint16_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_SHORT:
        return insertScalar<int16_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_ULONG:
        return insertScalar<uint32_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_LONG:
        return insertScalar<int32_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_ULONG64:
        return insertScalar<uint64_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_LONG64:
        return insertScalar<int64_t>(deviceAdapter, returnAccessor);
      case Tango::DEV_FLOAT:
        return insertScalar<float>(deviceAdapter, returnAccessor);
      case Tango::DEV_DOUBLE:
        return insertScalar<double>(deviceAdapter, returnAccessor);
      case Tango::DEV_STRING:
        return insertScalar<std::string>(deviceAdapter, returnAccessor);

      case Tango::DEVVAR_CHARARRAY:
        return insertArray<uint8_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_BOOLEANARRAY:
        return insertArray<ChimeraTK::Boolean>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_USHORTARRAY:
        return insertArray<uint16_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_SHORTARRAY:
        return insertArray<int16_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_ULONGARRAY:
        return insertArray<uint32_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_LONGARRAY:
        return insertArray<int32_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_ULONG64ARRAY:
        return insertArray<uint64_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_LONG64ARRAY:
        return insertArray<int64_t>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_FLOATARRAY:
        return insertArray<float>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_DOUBLEARRAY:
        return insertArray<double>(deviceAdapter, returnAccessor);
      case Tango::DEVVAR_STRINGARRAY:
        return insertArray<std::string>(deviceAdapter, returnAccessor);

      default:
        Tango::Except::throw_exception("API_CommandNotAllowed",
            std::format("Command {}: unsupported return type", _commandProperty.name).c_str(), "Command::execute()");
    }
  }

  /********************************************************************************************************************/

  template<typename AdapterType>
  CORBA::Any* Command::insertScalar(
      AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor) {
    using Scalar = TangoCommandType<AdapterType>::Scalar;
    auto values = deviceAdapter.readCommandReturnValues<AdapterType>(returnAccessor);
    return Tango::Command::insert(convertToTangoType<Scalar, AdapterType>(values.front()));
  }

  /********************************************************************************************************************/

  template<typename AdapterType>
  CORBA::Any* Command::insertArray(
      AdapterDeviceImpl& deviceAdapter, ChimeraTK::TransferElementAbstractor& returnAccessor) {
    using Array = TangoCommandType<AdapterType>::Array;
    using Scalar = TangoCommandType<AdapterType>::Scalar;

    auto values = deviceAdapter.readCommandReturnValues<AdapterType>(returnAccessor);

    const auto size = static_cast<unsigned>(values.size());

    // Ownership of the buffer passes to the sequence below, and of the sequence to insert().
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* buffer = Array::allocbuf(size);
    for(unsigned i = 0; i < size; ++i) {
      buffer[i] = convertToTangoType<Scalar, AdapterType>(values[i]);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto* data = new Array(size, size, buffer, 1);

    // The pointer form of insert() takes ownership of the sequence, so it must not be freed here.
    return Tango::Command::insert(data);
  }
} // namespace TangoAdapter

// NOLINTEND(readability-identifier-naming)

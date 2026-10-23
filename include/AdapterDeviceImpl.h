// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AttributeMapper.h"
#include "AttributeProperty.h"
#include "ChimeraTK/NDRegisterAccessor.h"
#include "ChimeraTK/TransferElementAbstractor.h"

#include <tango/tango.h>

#include <condition_variable>
#include <format>
#include <mutex>
#include <vector>

// Most of the non-conforming naming is a requierment from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

/**
 *  AdapterDeviceImpl class description:
 *    Test of TangoAdapterfor ChimeraTK
 */

namespace TangoAdapter {
  class AdapterDeviceImpl : public TANGO_BASE_CLASS {
   public:
    AdapterDeviceImpl(Tango::DeviceClass* cl, std::string& s);
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s);
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s, const char* d);
    ~AdapterDeviceImpl() override { delete_device(); }

    void delete_device() override;
    void init_device() override;
    void get_device_property();
    void always_executed_hook() override;

    void attachToClassAttributes(const std::shared_ptr<AttributeMapper::DeviceClass>& deviceClass);
    ChimeraTK::TransferElementAbstractor getPvForAttribute(const std::string& attributeName);

    void attachToClassCommands(const std::shared_ptr<AttributeMapper::DeviceClass>& deviceClass);
    bool hasCommand(const std::string& commandName);

    // The three accessors and the wait state of one command of this device instance. Shared with the command object
    // through a shared_ptr, so the command itself stays stateless.
    struct CommandState {
      ChimeraTK::TransferElementAbstractor trigger;
      ChimeraTK::TransferElementAbstractor inputArgument;
      ChimeraTK::TransferElementAbstractor returnValue;
      std::mutex mutex;
      std::condition_variable condition;
    };

    // The wait state and accessors of one command, looked up by name. Public because the stateless
    // TangoAdapter::Command drives the execution through them.
    std::shared_ptr<CommandState> getCommandState(const std::string& commandName);

    // Waits until the return accessor carries a version number other than the given one.
    static void waitForCommandReturnValue(CommandState& state, const ChimeraTK::VersionNumber& previousVersion);

    // Stores the incoming values in the input accessor (scalar or spectrum) and writes it with a valid data validity.
    // The Command does the CORBA::Any conversion, this does the accessor I/O.
    template<typename AdapterType>
    void writeCommandInputValues(ChimeraTK::TransferElementAbstractor& pv, const std::vector<AdapterType>& values);

    // Reads the whole return accessor into a vector without changing anything (the validity is checked by the caller).
    template<typename AdapterType>
    std::vector<AdapterType> readCommandReturnValues(ChimeraTK::TransferElementAbstractor& pv);

    // Lint: Disabling because this is Tango code
    // NOLINTNEXTLINE(google-runtime-int)
    void read_attr_hardware(std::vector<long>& attr_list) override;
    void add_dynamic_attributes();
    void add_dynamic_commands();

   private:
    std::map<std::string, ChimeraTK::TransferElementAbstractor> _attributeToPvMap;
    std::map<std::string, AttributeProperty> _attributeProperties;
    std::map<std::string, Tango::DevLong> _dataReadyCounters;
    std::map<std::string, std::shared_ptr<CommandState>> _commandStates;
    void pushChangeEvent(const std::string& attrName);

    void pushDataReadyEvent(const std::string& attrName);
    void restoreMemoriedSpectra(const std::list<AttributeProperty>&);
    void addMonitoredDeviceProperty(const std::optional<std::string>& source, const std::string& purpose,
        const std::string& updaterName, const std::function<bool(const std::type_info&)>& isSupportedType,
        std::function<void(ChimeraTK::TransferElementAbstractor&)> map);
  };

  /********************************************************************************************************************/

  template<typename AdapterType>
  void AdapterDeviceImpl::writeCommandInputValues(
      ChimeraTK::TransferElementAbstractor& pv, const std::vector<AdapterType>& values) {
    auto typedAccessor =
        boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<AdapterType>>(pv.getHighLevelImplElement());
    if(!typedAccessor) {
      throw ChimeraTK::logic_error(
          "Command input argument: process variable value type does not match the command input type");
    }

    if(typedAccessor->getNumberOfSamples() != values.size()) {
      // Tango arrays have a runtime length, so the incoming length has to match the process variable's.
      throw ChimeraTK::logic_error(
          std::format("Command input argument has {} elements, but the process variable has {}", values.size(),
              typedAccessor->getNumberOfSamples()));
    }

    for(size_t i = 0; i < values.size(); ++i) {
      typedAccessor->accessData(i) = values[i];
    }

    typedAccessor->setDataValidity(ChimeraTK::DataValidity::ok);
    typedAccessor->write();
  }

  /********************************************************************************************************************/

  template<typename AdapterType>
  std::vector<AdapterType> AdapterDeviceImpl::readCommandReturnValues(ChimeraTK::TransferElementAbstractor& pv) {
    auto typedAccessor =
        boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<AdapterType>>(pv.getHighLevelImplElement());
    if(!typedAccessor) {
      throw ChimeraTK::logic_error(
          "Command return value: process variable value type does not match the command return type");
    }

    std::vector<AdapterType> values(typedAccessor->getNumberOfSamples());
    for(size_t i = 0; i < values.size(); ++i) {
      values[i] = typedAccessor->accessData(i);
    }

    return values;
  }
} // namespace TangoAdapter
  // NOLINTEND(readability-identifier-naming)

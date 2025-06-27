// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"

#include <tango/tango.h>

namespace TangoAdapter {

  struct CommandDescription {
    std::string name;
  };

  class Command;

  class ProxyCommand : public Tango::Command {
   public:
    ~ProxyCommand() override;

    CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) override {
      return _owner->execute(dev, in_any);
    }

   private:
    std::shared_ptr<Command> _owner;
  };

  class Command {
   public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() { return _proxy != nullptr; }
    void notifyDeleted() { _proxy = nullptr; }

    CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in) {
      auto* adapterDevice = dynamic_cast<AdapterDeviceImpl*>(dev);
      assert(adapterDevice != nullptr);

      return std::make_unique<CORBA::Any>().release();
    }

   private:
    ProxyCommand* _proxy;
    ChimeraTK::TransferElementAbstractor _trigger;
    ChimeraTK::TransferElementAbstractor _argument;
    cppext::future_queue<void> _waitForResult;
  };

}; // namespace TangoAdapter

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"

#include <tango/tango.h>

namespace TangoAdapter {

  struct CommandDescription {
    std::string name;
  };

  class ProxyCommand;

  class CommandBase {
   public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() { return _proxy != nullptr; }
    void notifyDeleted() { _proxy = nullptr; }

    virtual CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in) = 0;

   protected:
    ProxyCommand* _proxy;
    cppext::future_queue<void> _waitForResult;
  };

  class ProxyCommand : public Tango::Command {
   public:
    ~ProxyCommand() override;

    CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) override {
      return _owner->execute(dev, in_any);
    }

   private:
    std::shared_ptr<CommandBase> _owner;
  };

  template<typename ArgumentUserType, typename ResultUserType>
  class Command : CommandBase {
   public:
    CORBA::Any* execute(Tango::DeviceImpl* dev, [[maybe_unused]] const CORBA::Any& in) override {
      auto* adapterDevice = dynamic_cast<AdapterDeviceImpl*>(dev);
      assert(adapterDevice != nullptr);
      return std::make_unique<CORBA::Any>().release();
    };
  };

}; // namespace TangoAdapter

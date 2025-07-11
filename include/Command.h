// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/VoidRegisterAccessor.h>

#include <tango/tango.h>

#include <utility>

namespace TangoAdapter {

  struct CommandDescription {
    std::string name;
  };

  class ProxyCommand;

  class CommandBase : public std::enable_shared_from_this<CommandBase> {
   public:
    CommandBase(std::string name, std::string triggerSourceName, ProxyCommand* proxy)
    : _proxy(proxy), _name(std::move(name)), _triggerSourceName(std::move(triggerSourceName)) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator bool() { return _proxy != nullptr; }
    void notifyDeleted() { _proxy = nullptr; }
    ProxyCommand* getTangoProxy();

    void setTrigger(const ChimeraTK::TransferElementAbstractor& triggerAccessor) {
      _triggerAccessor.replace(triggerAccessor);
    }

    [[nodiscard]] const std::string& getName() const { return _name; }
    [[nodiscard]] const std::string& getTriggerSourceName() const { return _triggerSourceName; }

   protected:
    ProxyCommand* _proxy;
    std::string _name;
    std::string _triggerSourceName;
    cppext::future_queue<void> _waitForResult;
    ChimeraTK::TransferElementAbstractor _triggerAccessor;

    friend class ProxyCommand;
    virtual CORBA::Any* execute(Tango::DeviceImpl*, const CORBA::Any&) { return nullptr; };
  };

  class ProxyCommand : public Tango::Command {
   public:
    using Tango::Command::Command;
    ~ProxyCommand() override;

    void setOwner(const std::shared_ptr<CommandBase>& owner) {
      assert(not _owner);
      _owner = owner;
    }

    [[nodiscard]] bool hasOwner() const { return bool(_owner); }

    CORBA::Any* execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) override {
      return _owner->execute(dev, in_any);
    }

   private:
    std::shared_ptr<CommandBase> _owner;
  };

  inline ProxyCommand* CommandBase::getTangoProxy() {
    if(!_proxy->hasOwner()) {
      _proxy->setOwner(shared_from_this());
    }

    return _proxy;
  }

  class Command : public CommandBase {
   public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    Command(const std::string& name, const std::string& triggerSourceName)
    : CommandBase(name, triggerSourceName,
          std::make_unique<ProxyCommand>(name, Tango::DEV_VOID, Tango::DEV_VOID, Tango::OPERATOR).release()) {}

    CORBA::Any* execute([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] const CORBA::Any& in) override {
      auto v = ChimeraTK::VersionNumber();
      _triggerAccessor.write(v);
      return std::make_unique<CORBA::Any>().release();
    }

   private:
  };

}; // namespace TangoAdapter

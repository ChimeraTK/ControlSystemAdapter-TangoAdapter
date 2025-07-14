#include "Command.h"

namespace TangoAdapter {

  /******************************************************************************************************************/
  /******************************************************************************************************************/
  /******************************************************************************************************************/

  void CommandBase::setTrigger(const ChimeraTK::TransferElementAbstractor& triggerAccessor) {
    _triggerAccessor.replace(triggerAccessor);
  }

  /******************************************************************************************************************/

  ProxyCommand* CommandBase::getTangoProxy() {
    if(!_proxy->hasOwner()) {
      _proxy->setOwner(shared_from_this());
    }

    return _proxy;
  }

  /******************************************************************************************************************/
  /******************************************************************************************************************/
  /******************************************************************************************************************/

  CORBA::Any* ProxyCommand::execute(Tango::DeviceImpl* dev, const CORBA::Any& in_any) {
    assert(_owner);

    return _owner->execute(dev, in_any);
  }

  /******************************************************************************************************************/

  void ProxyCommand::setOwner(const std::shared_ptr<CommandBase>& owner) {
    assert(not _owner);
    _owner = owner;
  }

  /******************************************************************************************************************/
  /******************************************************************************************************************/
  /******************************************************************************************************************/

  Command::Command(const std::string& name, const std::string& triggerSourceName)
  : CommandBase(name, triggerSourceName,
        std::make_unique<ProxyCommand>(name, Tango::DEV_VOID, Tango::DEV_VOID, Tango::OPERATOR).release()) {}

  /******************************************************************************************************************/

  CORBA::Any* Command::execute([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] const CORBA::Any& in) {
    auto v = ChimeraTK::VersionNumber();
    _triggerAccessor.write(v);
    return std::make_unique<CORBA::Any>().release();
  }

} // namespace TangoAdapter

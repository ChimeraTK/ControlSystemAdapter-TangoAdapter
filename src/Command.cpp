#include "Command.h"

namespace TangoAdapter {

  ProxyCommand::~ProxyCommand() {
    if(_owner) {
      _owner->notifyDeleted();
    }
  }
} // namespace TangoAdapter

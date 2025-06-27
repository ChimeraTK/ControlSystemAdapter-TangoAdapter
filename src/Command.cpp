#include "Command.h"

namespace TangoAdapter {

  ProxyCommand::~ProxyCommand() {
    _owner->notifyDeleted();
  }
} // namespace TangoAdapter

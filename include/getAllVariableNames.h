#pragma once

#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <set>
#include <string>

namespace ChimeraTK {

  /// convenience function to get all variable names from the CS adapter as a
  /// std::set (needed for instance for the variable mapper)
  std::set<std::string> getAllVariableNames(const boost::shared_ptr<ControlSystemPVManager>& csManager);

} //  namespace ChimeraTK

#include "getAllVariableNames.h"

namespace ChimeraTK {

  std::set<std::string> getAllVariableNames(boost::shared_ptr<ControlSystemPVManager> csManager) {
    std::set<std::string> output;
    for(auto& pv : csManager->getAllProcessVariables()) {
      output.insert(pv->getName());
    }

    return output;
  }

} //  namespace ChimeraTK

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AttributeMapper.h"
#include "TangoUpdater.h"
#include "ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h"

#include <boost/smart_ptr/shared_ptr.hpp>
namespace ChimeraTK {
  class TangoAdapter {
   public:
    static TangoAdapter& getInstance() {
      static TangoAdapter instance;

      return instance;
    }

    void run(int argc, char* argv[]);
    void prepareApplicationStartup();
    void finalizeApplicationStartup();
    void shutdown();
    [[nodiscard]] std::string getError() { return _error; }
    [[nodiscard]] boost::shared_ptr<ControlSystemPVManager> getCsPvManager() { return _controlSystemPVManager; }
    [[nodiscard]] boost::shared_ptr<DevicePVManager> getDevicePvManager() { return _devicePVManager; }

    [[nodiscard]] std::set<std::string> getCsVariableNames();
    [[nodiscard]] AttributeMapper& getMapper() { return _attributeMapper; }
    [[nodiscard]] TangoUpdater& getUpdater() { return _updater; }

   private:
    TangoAdapter();
    boost::shared_ptr<ControlSystemPVManager> _controlSystemPVManager;
    boost::shared_ptr<DevicePVManager> _devicePVManager;
    AttributeMapper _attributeMapper;
    TangoUpdater _updater;
    ChimeraTK::ApplicationBase* _appInstance{nullptr};
    std::string _error;
  };
} // namespace ChimeraTK

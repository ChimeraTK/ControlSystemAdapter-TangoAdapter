// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AttributeMapper.h"
#include "ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h"
#include "TangoUpdater.h"

#include <boost/smart_ptr/shared_ptr.hpp>
namespace TangoAdapter {
  class TangoAdapter {
   public:
    static constexpr std::string_view PLAIN_IMPORT_DUMMY_DEVICE{"__CHIMERATK_TEMPLATE_DEVICE_RAW_IMPORT"};
    static TangoAdapter& getInstance() {
      static TangoAdapter instance;

      return instance;
    }

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    void run(int argc, char* argv[], std::optional<std::function<void()>> postInitHook = {});
    void prepareApplicationStartup();
    void finalizeApplicationStartup();
    void shutdown();
    [[nodiscard]] std::string getError() { return _error; }
    [[nodiscard]] boost::shared_ptr<ChimeraTK::ControlSystemPVManager> getCsPvManager() {
      return _controlSystemPVManager;
    }
    [[nodiscard]] boost::shared_ptr<ChimeraTK::DevicePVManager> getDevicePvManager() { return _devicePVManager; }

    [[nodiscard]] std::set<std::string> getCsVariableNames();
    [[nodiscard]] AttributeMapper& getMapper() { return _attributeMapper; }
    [[nodiscard]] TangoUpdater& getUpdater() { return _updater; }

   private:
    TangoAdapter();
    boost::shared_ptr<ChimeraTK::ControlSystemPVManager> _controlSystemPVManager;
    boost::shared_ptr<ChimeraTK::DevicePVManager> _devicePVManager;
    AttributeMapper _attributeMapper;
    TangoUpdater _updater;
    ChimeraTK::ApplicationBase* _appInstance{nullptr};
    std::string _error;
  };
} // namespace TangoAdapter

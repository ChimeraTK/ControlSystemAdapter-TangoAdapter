// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoAdapter.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>

#include <tango/tango.h>

#include <filesystem>

// Compatibility with TANGO < 9.4 - cout may also be a macro there.
#ifndef TANGO_LOG
#  define TANGO_LOG cout
#endif

namespace ChimeraTK {
  TangoAdapter::TangoAdapter() {
    std::tie(_controlSystemPVManager, _devicePVManager) = createPVManager();
    _attributeMapper.setCSPVManager(_controlSystemPVManager);
  }

  std::set<std::string> TangoAdapter::getCsVariableNames() {
    std::set<std::string> output;
    for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
      output.insert(pv->getName());
    }

    return output;
  }

  void TangoAdapter::run(int argc, char* argv[]) {
    prepareApplicationStartup();
    try {
      // Initialise the device server
      //----------------------------------------
      Tango::Util* tg = Tango::Util::init(argc, argv);

      // Create the device server singleton
      //	which will create everything
      //----------------------------------------
      tg->server_init(false);

      // Run the endless loop
      //----------------------------------------
      TANGO_LOG << "Ready to accept request" << std::endl;
      tg->server_run();
    }
    catch(std::bad_alloc&) {
      TANGO_LOG << "Can't allocate memory to store device object !!!" << std::endl;
      TANGO_LOG << "Exiting" << std::endl;
    }
    catch(CORBA::Exception& e) {
      Tango::Except::print_exception(e);

      TANGO_LOG << "Received a CORBA_Exception" << std::endl;
      TANGO_LOG << "Exiting" << std::endl;
    }
    Tango::Util::instance()->server_cleanup();
    shutdown();
  }

  void TangoAdapter::prepareApplicationStartup() {
    // We do not get ownership of the application here. A plain pointer is used because the reference returned
    // by getApplicationInstance cannot be stored to a variable that has been created outside of the try block,
    // and we want to limit the scope of the try/catch to that single line.
    std::cout << "TangoAdapter starting application from " << std::filesystem::current_path() << std::endl;

    try {
      _appInstance = &ChimeraTK::ApplicationFactoryBase::getApplicationInstance();
    }
    catch(ChimeraTK::logic_error& e) {
      std::cerr << "*************************************************************"
                   "***************************************"
                << std::endl;
      std::cerr << "Logic error when getting the application instance. The TangoAdapter requires the use of the "
                   "ChimeraTK::ApplicationFactory instead of a static application instance."
                << std::endl;

      std::cerr << "Replace `static MyApplication theApp` with `static ChimeraTK::ApplicationFactory<MyApplication> "
                   "theAppFactory`."
                << std::endl;
      std::cerr << "*************************************************************"
                   "***************************************"
                << std::endl;

      std::cerr << e.what() << std::endl;
      _error = e.what();
      return;
    }
    _appInstance->setPVManager(_devicePVManager);
    _appInstance->initialise();
  }

  void TangoAdapter::finalizeApplicationStartup() {
    if(!_error.empty()) {
      return;
    }
    // start the application
    _appInstance->run();
    _updater.run();
  }

  void TangoAdapter::shutdown() {
    _updater.stop();
  }

} // namespace ChimeraTK

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoAdapter.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>

#include <tango/tango.h>

#include <filesystem>

namespace TangoAdapter {
  TangoAdapter::TangoAdapter() {
    std::tie(_controlSystemPVManager, _devicePVManager) = ChimeraTK::createPVManager();
    _attributeMapper.setCSPVManager(_controlSystemPVManager);
  }

  std::set<std::string> TangoAdapter::getCsVariableNames() {
    static std::set<std::string> variables = _attributeMapper.getCsVariableNames();

    return variables;
  }

  // This gets passed in commandline arguments
  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
  void TangoAdapter::run(int argc, char* argv[], std::optional<std::function<void()>> postInitHook) {
    prepareApplicationStartup();
    try {
      // Initialise the device server

      Tango::Util* tg = Tango::Util::init(argc, argv);

      // This has to be done after Util::init since it needs some information from Tango
      // if the mapper file is missing some information (e.g. Class name)
      _attributeMapper.readMapperFile();

      // Create the device server singleton
      //	which will create everything
      tg->server_init(false);

      // check for variables not yet initialised - we must guarantee that all to-application variables are written
      // exactly once at server start.
      for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
        if(!pv->isWriteable()) {
          continue;
        }

        if(pv->getVersionNumber() == ChimeraTK::VersionNumber(nullptr)) {
          // The variable has not yet been written. Do it now, even if we just send a 0.
          pv->write();
        }
      }

      // Start application and updater here after all classes and devices are created in server_init
      finalizeApplicationStartup();

      // Run anything else that needs to be done after server start - This is mainly used in the tests
      if(postInitHook) {
        postInitHook.value()();
      }

      // Run the endless loop
      /*--------------------------------------------------------------------------------------------------------------*/
      std::cout << "Ready to accept request" << std::endl;
      tg->server_run();
    }
    catch(std::bad_alloc&) {
      std::cerr << "Can't allocate memory to store device object !!!" << std::endl;
      std::cerr << "Exiting" << std::endl;
    }
    catch(CORBA::Exception& e) {
      Tango::Except::print_exception(e);

      std::cerr << "Received a CORBA_Exception" << std::endl;
      std::cerr << "Exiting" << std::endl;
    }
    shutdown();
  }

  /********************************************************************************************************************/

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

  /********************************************************************************************************************/

  void TangoAdapter::finalizeApplicationStartup() {
    if(!_error.empty()) {
      return;
    }

    _appInstance->optimiseUnmappedVariables(_attributeMapper.getUnusedVariables());

    // start the application
    _appInstance->run();
    _updater.run();
  }

  void TangoAdapter::shutdown() {
    _updater.stop();
  }

} // namespace TangoAdapter

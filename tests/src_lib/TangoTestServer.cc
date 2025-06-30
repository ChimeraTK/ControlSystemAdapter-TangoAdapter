// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoTestServer.h"

#include "TangoAdapter.h"

#include <chrono>
#include <filesystem>
#include <random>

ThreadedTangoServer::ThreadedTangoServer(std::string setTestName, bool setVerbose)
: testName(std::move(setTestName)), verbose(setVerbose) {}

/**********************************************************************************************************************/

ThreadedTangoServer::~ThreadedTangoServer() {
  if(tangoServerThread.joinable()) {
    stop();
  }

  if(!keepOfflineDatabase) {
    try {
      std::filesystem::remove(offlineDatabase);
    }
    catch(std::runtime_error&) {
      // ignore
    }
  }
}

/**********************************************************************************************************************/

void ThreadedTangoServer::start() {
  std::mutex in_mtx;
  std::unique_lock<std::mutex> in(in_mtx);
  std::condition_variable cv;
  bool threadRunning{false};

  tangoServerThread = std::thread([&]() {
    auto& adapter = TangoAdapter::TangoAdapter::getInstance();
    argv.emplace_back(testName + "_ds");
    argv.emplace_back("Test" + testName);
    if(verbose || std::getenv("TANGO_TESTS_VERBOSE") != nullptr) {
      argv.emplace_back("-v5");
    }
    deviceString = std::string("tango/test/") + testName;

    if(offlineDatabase.empty()) {
      argv.emplace_back("-nodb");
      argv.emplace_back("-dlist");
    }
    else {
      argv.emplace_back("-file=" + offlineDatabase);
    }
    argv.emplace_back(deviceString);
    argv.emplace_back("-ORBendPoint");
    argv.emplace_back("giop:tcp::" + port());

    std::vector<const char*> args;
    args.resize(argv.size());
    std::transform(argv.begin(), argv.end(), args.begin(), [&](auto& s) { return s.c_str(); });

    auto postInitHook = [&, this]() {
      tg = Tango::Util::instance();

      auto callback = []() -> bool {
        auto shutdown = ThreadedTangoServer::shutdownRequested.load();
        if(shutdown) {
          return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return false;
      };

      tg->server_set_event_loop(callback);
      {
        std::lock_guard<std::mutex> lg(in_mtx);
        threadRunning = true;
        cv.notify_one();
      }
    };

    // Need to pass it down to something that usually takes argc, argv directly from main
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    adapter.run(int(args.size()), const_cast<char**>(args.data()), std::make_optional(postInitHook));
  });
  cv.wait(in, [&] { return threadRunning; });
}

/**********************************************************************************************************************/

void ThreadedTangoServer::stop() {
  ThreadedTangoServer::shutdownRequested.store(true);
  tangoServerThread.join();
}

/**********************************************************************************************************************/

std::string ThreadedTangoServer::port() {
  static std::string corbaPort;
  if(corbaPort.empty()) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(10000, 50000);
    corbaPort = std::to_string(dist(rd));
  }

  return corbaPort;
}

/**********************************************************************************************************************/

std::string ThreadedTangoServer::getClientUrl() const {
  return "tango://localhost:" + port() + "/" + device() + "#dbase=no";
}

/**********************************************************************************************************************/

const std::string& ThreadedTangoServer::device() const {
  return deviceString;
}

/**********************************************************************************************************************/

ThreadedTangoServer& ThreadedTangoServer::setCreateOfflineDatabase(bool create) {
  createOfflineDatabase = create;

  return *this;
}

/**********************************************************************************************************************/

ThreadedTangoServer& ThreadedTangoServer::setKeepOfflineDatabase(bool keep) {
  keepOfflineDatabase = keep;

  return *this;
}

/**********************************************************************************************************************/

ThreadedTangoServer& ThreadedTangoServer::setOfflineDatabase(const std::string& basePath) {
  offlineDatabase = basePath + ".db";
  if(createOfflineDatabase) {
    auto sourceTemplate = basePath + "_template.db";
    try {
      std::filesystem::copy_file(sourceTemplate, offlineDatabase, std::filesystem::copy_options::overwrite_existing);
    }
    catch(std::runtime_error& err) {
      std::cerr << err.what() << std::endl;
      throw;
    }
  }

  return *this;
}

/**********************************************************************************************************************/

ThreadedTangoServer& ThreadedTangoServer::overrideNames(const std::string& newNames) {
  testName = newNames;

  return *this;
}

/**********************************************************************************************************************/

std::atomic<bool> ThreadedTangoServer::shutdownRequested{false};

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

TangoTestFixtureImpl::TangoTestFixtureImpl()
: theFactory(boost::unit_test::framework::master_test_suite().p_name.value),
  theServer(boost::unit_test::framework::master_test_suite().p_name.value) {
  f = this;
}

/**********************************************************************************************************************/

void TangoTestFixtureImpl::startup() {
  theServer.start();

  using clock = std::chrono::steady_clock;
  auto start = clock::now();

  auto url = theServer.getClientUrl();
  while(true) {
    try {
      std::cout << "Trying to connect to Tango server " << theServer.getClientUrl() << std::endl;
      proxy = std::make_unique<Tango::DeviceProxy>(url);
      proxy->read_attribute("State");
      break;
    }
    catch(CORBA::Exception& e) {
      if(auto now = clock::now(); now > start + std::chrono::seconds(10)) {
        Tango::Except::print_exception(e);
        assert(false);
      }
    }
  }

  // Cannot call any of the BOOST_ tests here, otherwise it will mark the setup as failed, regardless of the test outcome
  assert(proxy->is_connected());

  start = clock::now();
  while(proxy->state() != Tango::ON) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() > 5) {
      std::cerr << "Timeout while waiting for Tango server to go to state \"ON\"" << std::endl;
      break;
    }
  }

  // Cannot call any of the BOOST_ tests here, otherwise it will mark the setup as failed, regardless of the test outcome
  assert(proxy->state() == Tango::ON);

  // Wait for the server to become ON before getting the application from the factory
  // so that the server is definitely the one that creates the application, not the test
  theApp = dynamic_cast<ExtendedReferenceTestApplication*>(
      &ChimeraTK::ApplicationFactory<ExtendedReferenceTestApplication>::getApplicationInstance());
  assert(theApp != nullptr);
  if(manualLoopControl) {
    theApp->initialiseManualLoopControl();
  }
}

/**********************************************************************************************************************/

int TangoTestFixtureImpl::name2TypeId(const std::string& name) {
  static std::map<std::string, int> name2Type{{"DOUBLE", Tango::DEV_DOUBLE}, {"FLOAT", Tango::DEV_FLOAT},
      {"LONG", Tango::DEV_LONG64}, {"INT", Tango::DEV_LONG}, {"SHORT", Tango::DEV_SHORT}, {"UCHAR", Tango::DEV_UCHAR},
      {"ULONG", Tango::DEV_ULONG64}, {"UINT", Tango::DEV_ULONG}, {"USHORT", Tango::DEV_USHORT},
      {"CHAR", Tango::DEV_SHORT}, {"VOID", Tango::DEV_BOOLEAN}, {"BOOLEAN", Tango::DEV_BOOLEAN},
      {"STRING", Tango::DEV_STRING}};

  return name2Type.at(name);
}

/**********************************************************************************************************************/

TangoTestFixtureImpl::~TangoTestFixtureImpl() {
  // Shutdown proxy first, then the server, otherwise we will get a CORBA exception
  if(manualLoopControl) {
    theApp->releaseManualLoopControl();
  }
  proxy.reset(nullptr);
  theServer.stop();
}

/**********************************************************************************************************************/

void TangoTestFixtureImpl::setManualLoopControl(bool manualControl) {
  manualLoopControl = manualControl;
}

/**********************************************************************************************************************/

TangoTestFixtureImpl* TangoTestFixtureImpl::f;

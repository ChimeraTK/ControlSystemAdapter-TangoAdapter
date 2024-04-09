// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include <boost/test/included/unit_test.hpp>

#include <tango/tango.h>

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

#include <random>
#include <thread>
#include <chrono>

using namespace boost::unit_test;
using namespace boost::unit_test_framework;

struct ThreadedTangoServer {
  explicit ThreadedTangoServer(const std::string& testName, bool verbose = false) {
    argv.emplace_back(strdup((testName + "_ds").c_str()));
    argv.emplace_back(strdup(("Test" + testName).c_str()));
    if (verbose) {
      argv.emplace_back(strdup("-v4"));
    }
    argv.emplace_back(strdup("-nodb"));
    argv.emplace_back(strdup("-dlist"));
    deviceString = std::string("tango/test/") + testName;
    argv.emplace_back(strdup(deviceString.c_str()));
    argv.emplace_back(strdup("-ORBendPoint"));
    argv.emplace_back(strdup(("giop:tcp::" + port()).c_str()));
  }

  ~ThreadedTangoServer() {
    for(auto* c : argv) {
      free(c);
    }

    if (tangoServerThread.joinable()) {
      stop();
    }
  }

  void start() {
    std::mutex in_mtx;
    std::unique_lock<std::mutex> in(in_mtx);
    std::condition_variable cv;
    tangoServerThread = std::thread([&]() {
      try {
        tg = Tango::Util::init(argv.size(), argv.data());
        tg->server_init(false);
        cv.notify_all();
        tg->server_set_event_loop([]() {
            auto shutdown = ThreadedTangoServer::shutdownRequested.load();
            if (shutdown) {
              return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return false;
        });
        tg->server_run();
      } catch (CORBA::Exception& e) {
        Tango::Except::print_exception(e);
        std::rethrow_exception(std::current_exception());
      }
    });
    cv.wait(in);
  }

  void stop() {
    ThreadedTangoServer::shutdownRequested.store(true);
    tangoServerThread.join();
  }

  std::string port() {
    if(corbaPort.empty()) {
      std::random_device rd;
      std::uniform_int_distribution<int> dist(10000, 50000);
      corbaPort = std::to_string(dist(rd));
    }

    return corbaPort;
  }

  std::string getClientUrl() { return "tango://localhost:" + port() + "/" + device() + "#dbase=no"; }

  [[nodiscard]] const std::string& device() const { return deviceString; }

  std::vector<char*> argv;
  std::thread tangoServerThread;
  std::string deviceString{};
  Tango::Util *tg{nullptr};
  static std::string corbaPort;
  static std::atomic<bool> shutdownRequested;
};

std::string ThreadedTangoServer::corbaPort;
std::atomic<bool> ThreadedTangoServer::shutdownRequested{false};

struct TangoTestFixture {
  TangoTestFixture()  : theFactory(boost::unit_test::framework::master_test_suite().p_name.value), theServer(boost::unit_test::framework::master_test_suite().p_name.value) {
    f = this;
    theServer.start();
    using clock = std::chrono::steady_clock;
    auto start = clock::now();

    proxy = std::make_unique<Tango::DeviceProxy>(theServer.getClientUrl());
    while (!proxy->is_connected()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      if (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() > 5) {
        std::cerr << "Timeout while waiting for Tango server to become available" << std::endl;
        break;
      }
      proxy->connect(theServer.getClientUrl());
    }

    // Cannot all any of the BOOST_ tests here, otherwise it will mark the setup as failed, regardless of the test outcome
    assert(proxy->is_connected());

    start = clock::now();
    while (proxy->state() != Tango::ON) {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      if (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() > 5) {
        std::cerr << "Timeout while waiting for Tango server to become available" << std::endl;
        break;
      }
    }
    // Cannot all any of the BOOST_ tests here, otherwise it will mark the setup as failed, regardless of the test outcome
    assert(proxy->state() == Tango::ON);
  }

  ~TangoTestFixture() {
    proxy.reset(nullptr);
    theServer.stop();
  }

  ChimeraTK::ApplicationFactory<ReferenceTestApplication> theFactory;
  ThreadedTangoServer theServer;
  std::unique_ptr<Tango::DeviceProxy> proxy{nullptr};
  static TangoTestFixture* f;
};

TangoTestFixture* TangoTestFixture::f;

BOOST_GLOBAL_FIXTURE(TangoTestFixture);

BOOST_AUTO_TEST_CASE(testVariableExistence) {
  for(const auto* const directory : {"DOUBLE", "FLOAT", "INT", "SHORT", "UCHAR", "UINT", "USHORT", "CHAR"}) {
    for(const auto* const variable : {"CONSTANT_ARRAY", "FROM_DEVICE_ARRAY", "TO_DEVICE_ARRAY"}) {
      // if this throws the property does not exist. we should always be able to
      // read"
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      BOOST_CHECK_NO_THROW(TangoTestFixture::f->proxy->attribute_query(tangoName));
      std::cout << "test done " << std::endl;
    }

    for(const auto *const variable : {"DATA_TYPE_CONSTANT", "FROM_DEVICE_SCALAR", "TO_DEVICE_SCALAR"}) {
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      // if this throws the property does not exist. we should always be able to
      // read"
      BOOST_CHECK_NO_THROW(TangoTestFixture::f->proxy->attribute_query(tangoName));
      std::cout << "test done " << std::endl;
    }
  }
}

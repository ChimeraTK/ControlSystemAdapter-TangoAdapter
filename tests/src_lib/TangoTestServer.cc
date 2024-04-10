// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoTestServer.h"

#include <chrono>
#include <random>

ThreadedTangoServer::ThreadedTangoServer(const std::string& testName, bool verbose) {
  argv.emplace_back(testName + "_ds");
  argv.emplace_back("Test" + testName);
  if(verbose) {
    argv.emplace_back("-v4");
  }
  argv.emplace_back("-nodb");
  argv.emplace_back("-dlist");
  deviceString = std::string("tango/test/") + testName;
  argv.emplace_back(deviceString);
  argv.emplace_back("-ORBendPoint");
  argv.emplace_back("giop:tcp::" + port());
}

/*********************************************************************************************************************/

ThreadedTangoServer::~ThreadedTangoServer() {
  if(tangoServerThread.joinable()) {
    stop();
  }
}

/*********************************************************************************************************************/

void ThreadedTangoServer::start() {
  std::mutex in_mtx;
  std::unique_lock<std::mutex> in(in_mtx);
  std::condition_variable cv;
  tangoServerThread = std::thread([&]() {
    try {
      std::vector<const char*> args;
      args.resize(argv.size());
      std::transform(argv.begin(), argv.end(), args.begin(), [&](auto& s) { return s.c_str(); });
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      tg = Tango::Util::init(int(args.size()), const_cast<char**>(args.data()));
      tg->server_init(false);
      cv.notify_all();
      tg->server_set_event_loop([]() {
        auto shutdown = ThreadedTangoServer::shutdownRequested.load();
        if(shutdown) {
          return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return false;
      });
      tg->server_run();
    }
    catch(CORBA::Exception& e) {
      Tango::Except::print_exception(e);
      std::rethrow_exception(std::current_exception());
    }
  });
  cv.wait(in);
}

/*********************************************************************************************************************/

void ThreadedTangoServer::stop() {
  ThreadedTangoServer::shutdownRequested.store(true);
  tangoServerThread.join();
}

/*********************************************************************************************************************/

std::string ThreadedTangoServer::port() {
  static std::string corbaPort;
  if(corbaPort.empty()) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(10000, 50000);
    corbaPort = std::to_string(dist(rd));
  }

  return corbaPort;
}

/*********************************************************************************************************************/

std::string ThreadedTangoServer::getClientUrl() const {
  return "tango://localhost:" + port() + "/" + device() + "#dbase=no";
}

/*********************************************************************************************************************/

std::atomic<bool> ThreadedTangoServer::shutdownRequested{false};

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/

TangoTestFixture::TangoTestFixture()
: theFactory(boost::unit_test::framework::master_test_suite().p_name.value),
  theServer(boost::unit_test::framework::master_test_suite().p_name.value) {
  f = this;
  theServer.start();
  using clock = std::chrono::steady_clock;
  auto start = clock::now();

  proxy = std::make_unique<Tango::DeviceProxy>(theServer.getClientUrl());
  while(!proxy->is_connected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() > 5) {
      std::cerr << "Timeout while waiting for Tango server to become available" << std::endl;
      break;
    }
    proxy->connect(theServer.getClientUrl());
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
}

/*********************************************************************************************************************/

TangoTestFixture::~TangoTestFixture() {
  // Shutdown proxy first, then the server, otherwise we will get a CORBA exception
  proxy.reset(nullptr);
  theServer.stop();
}

/*********************************************************************************************************************/

TangoTestFixture* TangoTestFixture::f;

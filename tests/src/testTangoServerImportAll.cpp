// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include <tango/tango.h>

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

#include <boost/test/included/unit_test.hpp>

#include <random>
#include <thread>

struct ThreadedTangoServer {
  explicit ThreadedTangoServer(const std::string& testName) {
    argv.emplace_back(strdup((testName + "_ds").c_str()));
    argv.emplace_back(strdup(("Test" + testName).c_str()));
    argv.emplace_back(strdup("-v4"));
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
  }

  void start() {
    tangoServerThread = std::thread([&]() {
      auto* tg = Tango::Util::init(argv.size(), argv.data());
      tg->server_init(false);
      tg->server_run();
    });
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
  static std::string corbaPort;
};

std::string ThreadedTangoServer::corbaPort;

BOOST_AUTO_TEST_CASE(testVariableExistence) {
  auto factory = ChimeraTK::ApplicationFactory<ReferenceTestApplication>("testVariableExistence");
  auto t = ThreadedTangoServer("testVariableExistence");
  t.start();
  sleep(1);
  auto proxy = std::make_unique<Tango::DeviceProxy>(t.getClientUrl());
  while(proxy->state() != Tango::INIT) {
    std::cout << proxy->status() << std::endl;
    sleep(1);
  }
  for(const auto* const directory : {"DOUBLE", "FLOAT", "INT", "SHORT", "UCHAR", "UINT", "USHORT", "CHAR"}) {
    for(const auto* const variable : {"CONSTANT_ARRAY", "FROM_DEVICE_ARRAY", "TO_DEVICE_ARRAY "}) {
      // if this throws the property does not exist. we should always be able to
      // read"
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << std::endl;
      auto attr = proxy->read_attribute(std::string(directory) + "_" + variable);
      BOOST_CHECK(!attr.has_failed());
    }
    for(auto const variable : {"DATA_TYPE_CONSTANT", "FROM_DEVICE_SCALAR", "TO_DEVICE_SCALAR"}) {
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << std::endl;
      // if this throws the property does not exist. we should always be able to
      // read"
      std::cout << "test done " << std::endl;
    }
  }
}
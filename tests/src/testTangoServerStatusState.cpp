// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestStatusState

#include "TangoTestServer.h"

#include <ChimeraTK/cppext/finally.hpp>

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setOfflineDatabase("testServerStatusState");
  }
};
using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

bool checkStateWithTimeout(Tango::DeviceProxy& proxy, Tango::DevState expected) {
  const auto TIMEOUT = std::chrono::seconds(5);
  using clock = std::chrono::steady_clock;
  auto now = clock::now();

  Tango::DevState currentState;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {
      currentState = proxy.state();
    }
    catch(Tango::CommunicationFailed& ex) {
      Tango::Except::print_exception(ex);
    }
  } while(currentState != expected && (clock::now() - now) < TIMEOUT);

  return currentState == expected;
}

/**********************************************************************************************************************/

bool checkStatusWithTimeout(Tango::DeviceProxy& proxy, const std::string& expected) {
  const auto TIMEOUT = std::chrono::seconds(5);
  using clock = std::chrono::steady_clock;
  auto now = clock::now();

  std::string currentState;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {
      currentState = proxy.status();
    }
    catch(Tango::CommunicationFailed& ex) {
      Tango::Except::print_exception(ex);
    }
  } while(currentState != expected && (clock::now() - now) < TIMEOUT);

  std::cerr << ">> STATUS: " << currentState << std::endl;

  return currentState == expected;
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testState) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // FIXME: This is not working since the reference application writes
  // the output twice on startup
#if 0
  // When coming up, the device state should be ON
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::ON));
#endif

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 12);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::UNKNOWN));

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 0);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::ON));

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 1);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::FAULT));

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 2);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::OFF));

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 3);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::ALARM));
}

BOOST_AUTO_TEST_CASE(testStatus) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // FIXME: This is not working since the reference application writes
  // the output twice on startup
#if 0
  // When coming up, the device status should be ON
  BOOST_CHECK(checkStatusWithTimeout(proxy, "Application is running."));
#endif

  constexpr auto TEST_STRING_1 = "TEST_STRING_1";
  tf.write("STRING_TO_DEVICE_SCALAR", TEST_STRING_1);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStatusWithTimeout(proxy, TEST_STRING_1));
}

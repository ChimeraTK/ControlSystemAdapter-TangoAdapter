// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestStatusState

#include "TangoAdapter.h"
#include "TangoTestServer.h"

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <string>
#include <thread>

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

  std::string currentStatus;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {
      currentStatus = proxy.status();
    }
    catch(Tango::CommunicationFailed& ex) {
      Tango::Except::print_exception(ex);
    }
  } while(currentStatus != expected && (clock::now() - now) < TIMEOUT);

  return currentStatus == expected;
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testState) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // The reference application publishes 0 for INT/FROM_DEVICE_SCALAR on startup. That value is
  // already available when the device is created and has to be applied, so the device starts up
  // in state ON
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::ON));

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

  // The reference application publishes an empty string for STRING/FROM_DEVICE_SCALAR on startup.
  // That value is already available when the device is created and has to be applied, so the
  // default status of the device is replaced on startup
  BOOST_CHECK(checkStatusWithTimeout(proxy, std::string{}));

  constexpr auto TEST_STRING_1 = "TEST_STRING_1";
  tf.write("STRING_TO_DEVICE_SCALAR", TEST_STRING_1);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStatusWithTimeout(proxy, TEST_STRING_1));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFaultyValidity) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // Bring the device to a known state and status which the application has published and marked as valid
  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 2);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::OFF));

  const std::string statusString{"VALID_STATUS"};
  tf.write("STRING_TO_DEVICE_SCALAR", statusString);
  app.runMainLoopOnce();
  BOOST_CHECK(checkStatusWithTimeout(proxy, statusString));

  // With faulty validity neither the state nor the status must reach the client, although new values
  // are waiting in the to-device variables
  app.dataValidity = ChimeraTK::DataValidity::faulty;
  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 1);
  tf.write("STRING_TO_DEVICE_SCALAR", std::string("FAULTY_STATUS"));
  app.runMainLoopOnce();

  std::this_thread::sleep_for(std::chrono::seconds(1));
  BOOST_TEST(proxy.state() == Tango::DevState::OFF);
  BOOST_TEST(proxy.status() == statusString);

  // Restoring the validity applies the values which the application publishes from now on
  app.dataValidity = ChimeraTK::DataValidity::ok;
  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 1);
  tf.write("STRING_TO_DEVICE_SCALAR", std::string("VALID_AGAIN"));
  app.runMainLoopOnce();
  BOOST_CHECK(checkStateWithTimeout(proxy, Tango::DevState::FAULT));
  BOOST_CHECK(checkStatusWithTimeout(proxy, "VALID_AGAIN"));
}

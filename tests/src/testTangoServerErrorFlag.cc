// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <ChimeraTK/cppext/finally.hpp>

#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
  }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

bool checkQualityWithTimeout(Tango::DeviceProxy& proxy, std::string readName, Tango::AttrQuality quality)
{
  const auto TIMEOUT = std::chrono::seconds(5);
  using clock = std::chrono::steady_clock;
  auto now = clock::now();

  Tango::DeviceAttribute attr;
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    attr = proxy.read_attribute(readName);
  } while (attr.get_quality() != quality && (clock::now() - now) < TIMEOUT);

  return attr.get_quality() == quality;
}

BOOST_AUTO_TEST_CASE(testScalar) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  app.dataValidity = ChimeraTK::DataValidity::ok;
  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 12);
  app.runMainLoopOnce();
  BOOST_CHECK(checkQualityWithTimeout(proxy, std::string("INT_FROM_DEVICE_SCALAR"), Tango::AttrQuality::ATTR_VALID));

  app.dataValidity = ChimeraTK::DataValidity::faulty;
  tf.write<int>(std::string("INT_TO_DEVICE_SCALAR"), 24);
  app.runMainLoopOnce();

  BOOST_CHECK(checkQualityWithTimeout(proxy, std::string("INT_FROM_DEVICE_SCALAR"), Tango::AttrQuality::ATTR_INVALID));
}

BOOST_AUTO_TEST_CASE(testArray) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  app.dataValidity = ChimeraTK::DataValidity::ok;
  tf.write("INT_TO_DEVICE_ARRAY", std::vector<int>{12, 12, 12, 12, 12, 12, 12, 12, 12, 12});
  app.runMainLoopOnce();
  BOOST_CHECK(checkQualityWithTimeout(proxy, "INT_FROM_DEVICE_ARRAY", Tango::AttrQuality::ATTR_VALID));

  app.dataValidity = ChimeraTK::DataValidity::faulty;
  tf.write("INT_TO_DEVICE_ARRAY", std::vector<int>{24, 24, 24, 24, 24, 24, 24, 24, 24, 24});
  app.runMainLoopOnce();
  BOOST_CHECK(checkQualityWithTimeout(proxy, "INT_FROM_DEVICE_ARRAY", Tango::AttrQuality::ATTR_INVALID));
}

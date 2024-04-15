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

BOOST_AUTO_TEST_CASE(testScalar) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  app.dataValidity = ChimeraTK::DataValidity::ok;
  tf.write("INT_TO_DEVICE_SCALAR", 12);
  app.runMainLoopOnce();
  auto attr = proxy.read_attribute("INT_FROM_DEVICE_SCALAR");
  BOOST_TEST(attr.get_quality() == Tango::AttrQuality::ATTR_VALID);

  app.dataValidity = ChimeraTK::DataValidity::faulty;
  tf.write<int>("INT_TO_DEVICE_SCALAR", 24);
  app.runMainLoopOnce();
  attr = proxy.read_attribute("INT_FROM_DEVICE_SCALAR");
  BOOST_TEST(attr.get_quality() == Tango::AttrQuality::ATTR_INVALID);
}

BOOST_AUTO_TEST_CASE(testArray) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  app.dataValidity = ChimeraTK::DataValidity::ok;
  tf.write("INT_TO_DEVICE_ARRAY", std::vector<int>{12, 12, 12, 12, 12, 12, 12, 12, 12, 12});
  app.runMainLoopOnce();
  auto attr = proxy.read_attribute("INT_FROM_DEVICE_ARRAY");
  BOOST_TEST(attr.get_quality() == Tango::AttrQuality::ATTR_VALID);

  app.dataValidity = ChimeraTK::DataValidity::faulty;
  tf.write("INT_TO_DEVICE_ARRAY", std::vector<int>{24, 24, 24, 24, 24, 24, 24, 24, 24, 24});
  app.runMainLoopOnce();
  attr = proxy.read_attribute("INT_FROM_DEVICE_ARRAY");
  BOOST_TEST(attr.get_quality() == Tango::AttrQuality::ATTR_INVALID);
}

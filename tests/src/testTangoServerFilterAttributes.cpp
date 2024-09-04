// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestFilterAttributes

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setOfflineDatabase("testFilterAttributes");
  }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testPropertyFiltering) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());

  // We expect the 4 mapped properties and the built-in "State" and "Status"
  BOOST_TEST(attributes->size() == 6);

  BOOST_CHECK(std::find_if(attributes->begin(), attributes->end(), [](auto& attr) { return attr == "PlainRename"; }) !=
      attributes->end());

  auto info = proxy.get_attribute_config("PlainRename");
  BOOST_TEST(info.unit == "");
  // This is the default that Tango generates internally
  BOOST_TEST(info.description == "No description");

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), 42);
  app.runMainLoopOnce();

  BOOST_CHECK(tf.checkWithTimeout(std::string("PlainRename"), Tango::DevLong(42)));

  BOOST_CHECK(std::find_if(attributes->begin(), attributes->end(),
                  [](auto& attr) { return attr == "ChangeAttributes"; }) != attributes->end());

  info = proxy.get_attribute_config("ChangeAttributes");
  BOOST_TEST(info.unit == "mA");
  BOOST_TEST(info.description == "ChangedDescription");

  tf.write(std::string("SHORT_TO_DEVICE_SCALAR"), Tango::DevShort(23));
  app.runMainLoopOnce();

  BOOST_CHECK(tf.checkWithTimeout(std::string("ChangeAttributes"), Tango::DevShort(23)));
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestMapTwice

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setOfflineDatabase("testMapTwice");
  }
};
using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDoublePropertyMapping) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());

  // We expect the 4 mapped properties and the built-in "State" and "Status"
  BOOST_TEST(attributes->size() == 5);

  tf.write(std::string("INT_TO_DEVICE_SCALAR"), Tango::DevLong(43));

  app.runMainLoopOnce();

  BOOST_CHECK(tf.checkWithTimeout("PlainRename1", Tango::DevLong(43)));
  BOOST_CHECK(tf.checkWithTimeout("PlainRename2", Tango::DevLong(43)));
}

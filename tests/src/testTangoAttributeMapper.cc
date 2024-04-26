// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <ChimeraTK/cppext/finally.hpp>

#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl&) {
    ExtendedReferenceTestApplication::additionalVariables.emplace_back(ChimeraTK::SynchronizationDirection::deviceToControlSystem, "plainVariableInRoot", 1);
  }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

BOOST_AUTO_TEST_CASE(testPropertyMapping) {
  // Test that we can also properly handle variables that do not have any hierarchy level
  // This can happen if the Application uses a ConfigReader that has a toplevel variable
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());
  BOOST_TEST(attributes->size() == 77);

  BOOST_CHECK(std::find_if(attributes->begin(), attributes->end(), [](auto& attr) {
    return attr == "plainVariableInRoot";
  }) != attributes->end());
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestCommandOptionRoot

#include "TangoTestServer.h"

// The Tango umbrella header is the repository's test-file convention; clang-tidy cannot attribute the declarations.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tango/tango.h>

// The Boost.Test umbrella header is the repository's test-file convention; clang-tidy cannot attribute its macros.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <boost/test/included/unit_test.hpp>

namespace {

  /// The optionRoot module sets autoMapCommandsFromVoid="false" on the deviceServer root of its mapper, and neither its
  /// device class nor its device instance overrides it. The in-process fixture provides the single device instance.
  struct TestFixtureConfig {
    static void apply(TangoTestFixtureImpl& fixture) { fixture.setManualLoopControl(false); }
  };

} // namespace

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

/// The deviceServer root sets autoMapCommandsFromVoid="false", while its device class and its device instance carry no
/// override, so the value of the root level is the one which applies and the write-only ChimeraTK::Void process
/// variable stays the boolean attribute of today. The mapper declares no command.
BOOST_AUTO_TEST_CASE(testAutoMapCommandsFromVoidFalseOnServer) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto attributeInfo = proxy.attribute_query("VOID_TO_DEVICE_SCALAR");
  BOOST_TEST(attributeInfo.data_type == Tango::DEV_BOOLEAN);
}

/**********************************************************************************************************************/

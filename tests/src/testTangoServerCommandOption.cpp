// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestCommandOption

#include "TangoTestServer.h"

// The Tango umbrella header is the repository's test-file convention; clang-tidy cannot attribute the declarations.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tango/tango.h>

// The Boost.Test umbrella header is the repository's test-file convention; clang-tidy cannot attribute its macros.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <boost/test/included/unit_test.hpp>

#include <format>
#include <string>

namespace {

  /// The option module runs against the offline database tests/testCommandOption_template.db, which declares the two
  /// device instances tango/test/servertestcommandoption and tango/test/servertestcommandoptionother of two device
  /// classes, so the mapper has to reflect that layout.
  struct TestFixtureConfig {
    static void apply(TangoTestFixtureImpl& fixture) {
      fixture.setManualLoopControl(true);
      fixture.theServer.setOfflineDatabase("testCommandOption");
    }
  };

} // namespace

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

/// Whether the write-only ChimeraTK::Void process variable VOID/TO_DEVICE_SCALAR of the given device is still mapped as
/// the boolean attribute of today: the option autoMapCommandsFromVoid="false" keeps it an attribute instead of a
/// command.
static bool hasVoidBooleanAttribute(const std::string& deviceName) {
  Tango::DeviceProxy proxy(std::format("tango://localhost:{}/{}#dbase=no", ThreadedTangoServer::port(), deviceName));

  auto attributeInfo = proxy.attribute_query("VOID_TO_DEVICE_SCALAR");

  return attributeInfo.data_type == Tango::DEV_BOOLEAN;
}

/**********************************************************************************************************************/

/// autoMapCommandsFromVoid="false" keeps the write-only ChimeraTK::Void process variable the boolean attribute of
/// today instead of a command. The device class of tango/test/servertestcommandoption takes the false of its own level
/// over the true of the deviceServer root, and tango/test/servertestcommandoptionother takes its own false over the
/// true of its device class.
BOOST_AUTO_TEST_CASE(testAutoMapCommandsFromVoidFalse) {
  BOOST_TEST(hasVoidBooleanAttribute("tango/test/servertestcommandoption"));
  BOOST_TEST(hasVoidBooleanAttribute("tango/test/servertestcommandoptionother"));
}

/**********************************************************************************************************************/

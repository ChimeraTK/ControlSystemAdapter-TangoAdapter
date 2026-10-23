// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestCommandOptionServer

#include "TangoTestServer.h"

#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>
#include <ChimeraTK/VersionNumber.h>
#include <ChimeraTK/Void.h>

// The Tango umbrella header is the repository's test-file convention; clang-tidy cannot attribute the declarations.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <tango/tango.h>

// The Boost.Test umbrella header is the repository's test-file convention; clang-tidy cannot attribute its macros.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

  /// The optionServer module sets autoMapCommandsFromVoid="true" on the deviceServer root of its mapper, and neither
  /// its device class nor its device instance overrides it. The in-process fixture provides the single device
  /// instance, so no manual loop control and no offline database are needed.
  struct TestFixtureConfig {
    static void apply(TangoTestFixtureImpl& fixture) { fixture.setManualLoopControl(false); }
  };

} // namespace

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

/// The deviceServer root sets autoMapCommandsFromVoid="true", so the write-only ChimeraTK::Void process variable
/// VOID/TO_DEVICE_SCALAR becomes an argument-less command without a return value. The mapper imports with
/// <import>/</import>, the only import form whose derived command name is VOID_TO_DEVICE_SCALAR.
BOOST_AUTO_TEST_CASE(testAutoMappedVoid) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto voidTrigger = app.getPVManager()->getProcessArray<ChimeraTK::Void>("/VOID/TO_DEVICE_SCALAR");
  const auto voidTriggerVersion = voidTrigger->getVersionNumber();

  Tango::DeviceData out = proxy.command_inout("VOID_TO_DEVICE_SCALAR");

  // A command without a return value answers with an empty Tango::DeviceData. The isempty_flag must be reset first:
  // without it is_empty() throws API_EmptyDeviceData in this cppTango build.
  out.reset_exceptions(Tango::DeviceData::isempty_flag);
  BOOST_TEST(out.is_empty());

  // The application has consumed the trigger write: the process array carries a new version number.
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  while(voidTrigger->getVersionNumber() == voidTriggerVersion && clock::now() - start < std::chrono::seconds(5)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  BOOST_TEST(voidTrigger->getVersionNumber() != voidTriggerVersion);

  // The auto-mapped source is a command, not an attribute of the device.
  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());
  for(const auto& attrName : *attributes) {
    BOOST_TEST(attrName != "VOID_TO_DEVICE_SCALAR");
  }
}

/**********************************************************************************************************************/

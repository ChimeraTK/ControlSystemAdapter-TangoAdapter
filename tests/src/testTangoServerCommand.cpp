// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestCommand

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

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// The descriptions of a command default to the description of the process variable of its argument and of its return
// value, so the module registers one described process variable which the mapper uses as the source of the
// description-default command.
struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl&) {
    ExtendedReferenceTestApplication::additionalVariables.emplace_back(
        ChimeraTK::SynchronizationDirection::bidirectional, "/INT/DESCRIBED", 1, "the described value");
  }
};

// The calls block until the application has answered, so the module runs with the automatic main loop of the reference
// application.
using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

/// The process array of a to-device process variable of the reference application, for the version-number assertions.
template<typename T>
static ChimeraTK::ProcessArray<T>::SharedPtr toDeviceProcessArray(
    ExtendedReferenceTestApplication& app, const std::string& name) {
  return app.getPVManager()->getProcessArray<T>(name);
}

/**********************************************************************************************************************/

/**
 * @brief Poll the given to-device process variable until its version number differs from the given one.
 *
 * Through the version number a test shows that the application consumed the command's write, not only that the call
 * did not throw. The write of a command is consumed by the main loop when it copies the to-device variable, so polling
 * is needed for the calls which return before that copy.
 */
template<typename T>
static bool pollToDeviceWriteVersionChanged(
    ExtendedReferenceTestApplication& app, const std::string& name, const ChimeraTK::VersionNumber& before) {
  auto processArray = toDeviceProcessArray<T>(app, name);
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  while(clock::now() - start < std::chrono::seconds(5)) {
    if(processArray->getVersionNumber() != before) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return false;
}

/**********************************************************************************************************************/

/// A command without a trigger: its argument write is the trigger, and the call returns the return value.
BOOST_AUTO_TEST_CASE(testArgumentAndReturnValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto intArgument = toDeviceProcessArray<int32_t>(app, "/INT/TO_DEVICE_SCALAR");
  const auto intArgumentVersion = intArgument->getVersionNumber();

  Tango::DeviceData in;
  in << static_cast<Tango::DevLong>(17);

  Tango::DeviceData out = proxy.command_inout("ArgumentAndReturnValue", in);

  Tango::DevLong returned = 0;
  out >> returned;
  BOOST_TEST(returned == 17);

  // The process array of the application carries the argument: its version number changed. The return-value source is
  // not read with a Tango attribute, because INT/FROM_DEVICE_SCALAR is a command source of the file and the import
  // skips it.
  BOOST_TEST(pollToDeviceWriteVersionChanged<int32_t>(app, "/INT/TO_DEVICE_SCALAR", intArgumentVersion));
}

/**********************************************************************************************************************/

/// The command descriptions come from the inputDescription and outputDescription children, or default to the
/// description of the process variable of the argument and of the return value.
BOOST_AUTO_TEST_CASE(testCommandDescriptions) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // The command carries explicit descriptions, so command_query returns them.
  Tango::CommandInfo explicitInfo = proxy.command_query("ArgumentAndReturnValue");
  BOOST_TEST(explicitInfo.in_type_desc == std::string("the argument"));
  BOOST_TEST(explicitInfo.out_type_desc == std::string("the answer"));

  // A command without the two children takes the description of the process variable of its inputArgument and of its
  // returnValue.
  Tango::CommandInfo defaultInfo = proxy.command_query("DescribedArgumentAndReturnValue");
  BOOST_TEST(defaultInfo.in_type_desc == std::string("the described value"));
  BOOST_TEST(defaultInfo.out_type_desc == std::string("the described value"));
}

/**********************************************************************************************************************/

/// A command declared without a name answers to the name derived from its trigger source, or from its inputArgument
/// source when there is no trigger.
BOOST_AUTO_TEST_CASE(testDefaultCommandName) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // The trigger-only command without a name answers to the name derived from its trigger source.
  Tango::CommandInfo triggerOnlyInfo = proxy.command_query("VOID_TO_DEVICE_SCALAR");
  BOOST_TEST(triggerOnlyInfo.cmd_name == std::string("VOID_TO_DEVICE_SCALAR"));

  // The command without a name and without a trigger answers to the name derived from its inputArgument source.
  Tango::CommandInfo argumentOnlyInfo = proxy.command_query("INT_TO_DEVICE_SCALAR");
  BOOST_TEST(argumentOnlyInfo.cmd_name == std::string("INT_TO_DEVICE_SCALAR"));
}

/**********************************************************************************************************************/

/// A command with a ChimeraTK::Void trigger and an argument: the application sees both writes.
BOOST_AUTO_TEST_CASE(testTriggerAndArgument) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto intArgument = toDeviceProcessArray<int32_t>(app, "/INT/TO_DEVICE_SCALAR");
  auto voidTrigger = toDeviceProcessArray<ChimeraTK::Void>(app, "/VOID/TO_DEVICE_SCALAR");

  const auto intArgumentVersion = intArgument->getVersionNumber();
  const auto voidTriggerVersion = voidTrigger->getVersionNumber();

  Tango::DeviceData in;
  in << static_cast<Tango::DevLong>(23);

  Tango::DeviceData out = proxy.command_inout("TriggerAndArgument", in);

  Tango::DevLong returned = 0;
  out >> returned;
  BOOST_TEST(returned == 23);

  // The application has consumed both writes: the argument write and the trigger write. Both process arrays carry a
  // new version number.
  BOOST_TEST(pollToDeviceWriteVersionChanged<int32_t>(app, "/INT/TO_DEVICE_SCALAR", intArgumentVersion));
  BOOST_TEST(pollToDeviceWriteVersionChanged<ChimeraTK::Void>(app, "/VOID/TO_DEVICE_SCALAR", voidTriggerVersion));

  // The argument was written before the trigger.
  BOOST_TEST(intArgument->getVersionNumber() < voidTrigger->getVersionNumber());
}

/**********************************************************************************************************************/

/// A command with a write-only ChimeraTK::Void trigger only: the call returns without a value.
BOOST_AUTO_TEST_CASE(testTriggerOnly) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto voidTrigger = toDeviceProcessArray<ChimeraTK::Void>(app, "/VOID/TO_DEVICE_SCALAR");
  const auto voidTriggerVersion = voidTrigger->getVersionNumber();

  // The returned Tango::DeviceData must not be inspected: both is_empty() and get_type() throw
  // API_EmptyDeviceData in this cppTango build for a command without a return value.
  BOOST_CHECK_NO_THROW(proxy.command_inout("TriggerOnly"));

  // The application has consumed the trigger write: the process array carries a new version number.
  BOOST_TEST(pollToDeviceWriteVersionChanged<ChimeraTK::Void>(app, "/VOID/TO_DEVICE_SCALAR", voidTriggerVersion));
}

/**********************************************************************************************************************/

/// The <import> element before the <command> elements leaves a command source unmapped as an attribute.
BOOST_AUTO_TEST_CASE(testImportSkipsCommandSource) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // The import covers VOID/TO_DEVICE_SCALAR, but the source is a command source of the file, so it stays unmapped.
  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());
  for(const auto& attrName : *attributes) {
    BOOST_TEST(attrName != "VOID_TO_DEVICE_SCALAR");
  }
  // The import covers every process variable of the household except the eight command sources of the file: the
  // source stays unmapped although the import covers it. The 88 process variables less the eight command sources plus
  // the built-in State and Status give 82 attributes.
  BOOST_TEST(attributes->size() == 82);

  // The command itself is registered on the device instance.
  Tango::CommandInfo cmdInfo = proxy.command_query("TriggerOnly");
  BOOST_TEST(cmdInfo.cmd_name == std::string("TriggerOnly"));
}

/**********************************************************************************************************************/

/// An array argument and an array return value, asserted element by element.
BOOST_AUTO_TEST_CASE(testArrayArgumentAndReturnValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  const std::vector<Tango::DevLong> written{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  Tango::DeviceData in;
  in << written;

  Tango::DeviceData out = proxy.command_inout("ArrayArgumentAndReturnValue", in);

  std::vector<Tango::DevLong> returned;
  out >> returned;
  BOOST_TEST(returned == written, boost::test_tools::per_element());
}

/**********************************************************************************************************************/

/// A Boolean array argument and return value, asserted element by element.
BOOST_AUTO_TEST_CASE(testBooleanArrayArgumentAndReturnValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  const std::vector<Tango::DevBoolean> written{true, false, true, true, false, false, true, false, true, true};

  Tango::DeviceData in;
  in << written;

  Tango::DeviceData out = proxy.command_inout("BooleanArrayArgumentAndReturnValue", in);

  std::vector<Tango::DevBoolean> returned;
  out >> returned;
  BOOST_TEST(returned == written, boost::test_tools::per_element());
}

/**********************************************************************************************************************/

/// A command without a return value returns without waiting, after the argument has been written.
BOOST_AUTO_TEST_CASE(testNoReturnValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  auto intArgument = toDeviceProcessArray<int32_t>(app, "/INT/TO_DEVICE_SCALAR");
  const auto intArgumentVersion = intArgument->getVersionNumber();

  Tango::DeviceData in;
  in << static_cast<Tango::DevLong>(31);

  // The returned Tango::DeviceData must not be inspected: both is_empty() and get_type() throw
  // API_EmptyDeviceData in this cppTango build for a command without a return value.
  BOOST_CHECK_NO_THROW(proxy.command_inout("NoReturnValue", in));

  // The argument reached the application although the command has no return value. The argument is a command source
  // of the file, so it is not a Tango attribute and the test reads it through ApplicationBase::getPVManager().
  BOOST_TEST(pollToDeviceWriteVersionChanged<int32_t>(app, "/INT/TO_DEVICE_SCALAR", intArgumentVersion));
}

/**********************************************************************************************************************/

/// A return value whose process variable carries faulty data fails the call.
BOOST_AUTO_TEST_CASE(testInvalidReturnValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  app.dataValidity = ChimeraTK::DataValidity::faulty;

  Tango::DeviceData in;
  in << static_cast<Tango::DevLong>(41);

  try {
    proxy.command_inout("InvalidReturnValue", in);
    BOOST_TEST(false);
  }
  catch(const Tango::DevFailed& error) {
    BOOST_TEST(error.errors.length() > 0);
    // The faulty return value is not a rejection of the command itself, so the reason must not be the one is_allowed()
    // relies on. The description names the return value process variable.
    BOOST_TEST(std::string(error.errors[0].reason.in()) != Tango::API_CommandNotAllowed);
    BOOST_TEST(std::string(error.errors[0].reason.in()) == "API_DataNotValid");
    BOOST_TEST(std::string(error.errors[0].desc.in()).find("INT/FROM_DEVICE_SCALAR") != std::string::npos);
  }

  app.dataValidity = ChimeraTK::DataValidity::ok;
}

/**********************************************************************************************************************/

/**
 * @brief Assert that the mapper of the given rejection directory aborts the start-up of testTangoServer.
 *
 * The device name is derived from the directory name by lowering it, as every reject fixture uses lower-case device
 * names.
 */
static void assertRejectedAtMapperRead(const std::string& rejectDirectory) {
  auto deviceName = rejectDirectory;
  std::transform(deviceName.begin(), deviceName.end(), deviceName.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  const auto status = runTestTangoServer(
      std::filesystem::current_path() / rejectDirectory, "CommandReject", "tango/test/" + deviceName);
  BOOST_TEST(status != 0);
}

/**********************************************************************************************************************/

/// A command without trigger and without inputArgument makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testCommandWithoutSourceRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandNoSource");
}

/// An unsupported value type makes the mapper read abort the start-up: a uint8_t scalar, an int8_t scalar and an
/// int8_t array have no Tango command type.
BOOST_AUTO_TEST_CASE(testUnsupportedValueTypeRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectUint8ScalarArgument");
  assertRejectedAtMapperRead("rejectInt8ScalarArgument");
  assertRejectedAtMapperRead("rejectInt8ArrayArgument");
}

/// An explicit <attribute> naming a write-only ChimeraTK::Void process variable makes the mapper read abort the
/// start-up.
BOOST_AUTO_TEST_CASE(testVoidAttributeRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandVoidAttribute");
}

/// A returnValue naming a ChimeraTK::Void process variable makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testVoidReturnValueRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectVoidReturnValue");
}

/// An inputArgument naming a ChimeraTK::Void process variable makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testVoidInputArgumentRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectVoidInputArgument");
}

/// A trigger naming a process variable which is not a write-only ChimeraTK::Void makes the mapper read abort the
/// start-up.
BOOST_AUTO_TEST_CASE(testNonVoidTriggerRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectNonVoidTrigger");
}

/// A source which is not in the variable household makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testUnknownSourceRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectUnknownSource");
}

/// An inputArgument naming a process variable which is not writeable makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testNonWriteableArgumentRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectNonWriteableArgument");
}

/// A returnValue naming a process variable which is not readable makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testNonReadableReturnValueRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectNonReadableReturnValue");
}

/// A process variable which a command source and an attribute share, with the <attribute> element before the <command>
/// element, makes the mapper read abort the start-up: the attribute maps the source first, so the <command> which
/// names the source second is rejected.
BOOST_AUTO_TEST_CASE(testCommandSourceAsAttributeRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandSourceAsAttribute");
}

/// A process variable which a command source and an attribute share, with the <command> element before the <attribute>
/// element, makes the mapper read abort the start-up: the command declares the source first, so the <attribute> which
/// names the source second is rejected.
BOOST_AUTO_TEST_CASE(testCommandSourceAsCommandRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandSourceAsCommand");
}

/// The same command name with different types in two device instances makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testCommandTypeClashInClassRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandTypeClashInClass");
}

/// A command named State, Status or Init makes the mapper read abort the start-up.
BOOST_AUTO_TEST_CASE(testReservedCommandNameRejectedAtMapperRead) {
  assertRejectedAtMapperRead("rejectCommandNameState");
  assertRejectedAtMapperRead("rejectCommandNameStatus");
  assertRejectedAtMapperRead("rejectCommandNameInit");
}

/**********************************************************************************************************************/

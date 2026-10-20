// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoAdapter.h"
#include "TangoTestServer.h"

#include <ChimeraTK/cppext/finally.hpp>

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <string>
#include <vector>

/**********************************************************************************************************************/

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl&) {
    ExtendedReferenceTestApplication::additionalVariables.emplace_back(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, "plainVariableInRoot", 1);
  }
};

/**********************************************************************************************************************/

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testPropertyMapping) {
  // Test that we can also properly handle variables that do not have any hierarchy level
  // This can happen if the Application uses a ConfigReader that has a toplevel variable
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());
  BOOST_TEST(attributes->size() == 90);

  BOOST_CHECK(std::find_if(attributes->begin(), attributes->end(),
                  [](auto& attr) { return attr == "plainVariableInRoot"; }) != attributes->end());
}

/**********************************************************************************************************************/

namespace {
  /// A mapper file with the given `<attribute>` children, written under the executable name of the running server.
  constexpr std::string_view mapperFilePrefix = R"(<?xml version="1.0" encoding="UTF-8" ?>
<deviceServer>
    <deviceClass name="serverTestImportAll">
        <deviceInstance name="tango/test/serverTestimportall">
)";

  /// Run the given action with the current directory set to a fresh per-case directory holding a mapper file.
  template<typename Action>
  void inCaseDirectory(const std::string& caseName, const std::string& attributeXml, Action action) {
    const auto previousDirectory = std::filesystem::current_path();
    const auto caseDirectory = previousDirectory / ("mapperTest_" + caseName);
    std::filesystem::create_directories(caseDirectory);
    // The mapper derives the file name it reads from the executable name of the running in-process server, so the
    // file has to carry exactly that name.
    {
      const auto mapperFileName = std::string(Tango::Util::instance()->get_ds_exec_name()) + "-AttributeMapper.xml";
      std::ofstream out(caseDirectory / mapperFileName);
      out << mapperFilePrefix << attributeXml << "    </deviceInstance>\n  </deviceClass>\n</deviceServer>\n";
    }

    const auto guard = cppext::finally([&previousDirectory] { std::filesystem::current_path(previousDirectory); });
    std::filesystem::current_path(caseDirectory);
    action();
  }

  /// Build a mapper reading the mapper file written for the given attribute XML.
  void readMapperFor(
      const std::string& caseName, const std::string& attributeXml, TangoAdapter::AttributeMapper& mapper) {
    // The process variable manager is the running server's control-system manager, since the application only
    // exposes a DevicePVManager.
    mapper.setCSPVManager(TangoAdapter::TangoAdapter::getInstance().getCsPvManager());
    inCaseDirectory(caseName, attributeXml, [&mapper] { mapper.readMapperFile(); });
  }

  /// A single `<attribute>` element with the given eventing element, empty for an absent `<eventing>`.
  std::string attributeWithEventing(const std::string& name, const std::string& source, const std::string& eventing) {
    return "      <attribute name=\"" + name + "\" source=\"" + source + "\">\n" + eventing + "      </attribute>\n";
  }

  /// The parsed eventing mode of the single attribute of the mapper file written for the given attribute XML.
  TangoAdapter::EventingMode parsedEventingOf(const std::string& caseName, const std::string& attributeXml) {
    TangoAdapter::AttributeMapper mapper;
    readMapperFor(caseName, attributeXml, mapper);

    std::string className = "serverTestImportAll";
    const auto deviceClass = mapper.getClass(className);
    BOOST_REQUIRE(deviceClass != nullptr);
    BOOST_REQUIRE_EQUAL(deviceClass->attributes.size(), 1U);
    return deviceClass->attributes.front().eventing;
  }

  /// Assert that the mapper file written for the given attribute XML is rejected with a logic_error.
  void expectMapperRejects(const std::string& caseName, const std::string& attributeXml) {
    TangoAdapter::AttributeMapper mapper;
    BOOST_CHECK_THROW(readMapperFor(caseName, attributeXml, mapper), ChimeraTK::logic_error);
  }
} // namespace

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testMapperParsesEventingModes) {
  const auto scalar = [](const std::string& eventing) {
    return attributeWithEventing("INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR", eventing);
  };

  BOOST_CHECK(parsedEventingOf("dataReady", scalar("        <eventing>dataReady</eventing>\n")) ==
      TangoAdapter::EventingMode::DataReady);
  BOOST_CHECK(parsedEventingOf("change", scalar("        <eventing>change</eventing>\n")) ==
      TangoAdapter::EventingMode::Change);
  BOOST_CHECK(parsedEventingOf("off", scalar("        <eventing>off</eventing>\n")) == TangoAdapter::EventingMode::Off);
  BOOST_CHECK(parsedEventingOf("absent", scalar("")) == TangoAdapter::EventingMode::Off);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInvalidEventingValueRejectedAtMapperRead) {
  expectMapperRejects("invalidValue",
      attributeWithEventing(
          "INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR", "        <eventing>invalidValue</eventing>\n"));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteOnlyEventingRejectedAtMapperRead) {
  expectMapperRejects("writeOnly",
      attributeWithEventing("INT_TO_DEVICE_SCALAR", "INT/TO_DEVICE_SCALAR", "        <eventing>change</eventing>\n"));
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testVoidChangeEventingRejectedAtMapperRead) {
  expectMapperRejects("voidChange",
      attributeWithEventing(
          "VOID_FROM_DEVICE_SCALAR", "VOID/FROM_DEVICE_SCALAR", "        <eventing>change</eventing>\n"));
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoAdapter.h"
#include "TangoTestServer.h"

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**********************************************************************************************************************/

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl&) {
    ExtendedReferenceTestApplication::additionalVariables.emplace_back(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, "plainVariableInRoot", 1, std::string{});
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
  BOOST_TEST(attributes->size() == 89);

  BOOST_CHECK(std::find_if(attributes->begin(), attributes->end(),
                  [](auto& attr) { return attr == "plainVariableInRoot"; }) != attributes->end());
}

/**********************************************************************************************************************/

constexpr std::string_view className = "serverTestImportAll";
constexpr std::string_view deviceName = "tango/test/serverTestimportall";

/**
 * @brief A single `<attribute>` element with the given children (empty for none).
 */
static std::string attribute(const std::string& name, const std::string& source, const std::string& children = {}) {
  return "            <attribute name=\"" + name + "\" source=\"" + source + "\">\n" + children +
      "            </attribute>\n";
}

/**
 * @brief An `<attribute>` element with an `<eventing>` element carrying the given text (empty for an absent element).
 */
static std::string attributeWithEventing(
    const std::string& name, const std::string& source, const std::string& eventingText) {
  const auto eventing =
      eventingText.empty() ? std::string{} : "                <eventing>" + eventingText + "</eventing>\n";
  return attribute(name, source, eventing);
}

/**
 * @brief Build a mapper that reads the mapper file written for the given attribute XML.
 */
static void readMapperFor(
    const std::string& caseName, const std::string& attributesXml, TangoAdapter::AttributeMapper& mapper) {
  // The process variable manager is the running server's control-system manager, since the application only
  // exposes a DevicePVManager.
  mapper.setCSPVManager(TangoAdapter::TangoAdapter::getInstance().getCsPvManager());
  withMapperFileInCaseDirectory(
      caseName, std::string(className), std::string(deviceName), attributesXml, [&mapper] { mapper.readMapperFile(); });
}

/**
 * @brief The attributes the mapper parsed from the mapper file written for the given attribute XML.
 */
static std::list<TangoAdapter::AttributeProperty> parseAttributesOf(
    const std::string& caseName, const std::string& attributesXml) {
  TangoAdapter::AttributeMapper mapper;
  readMapperFor(caseName, attributesXml, mapper);

  std::string name = std::string(className);
  const auto deviceClass = mapper.getClass(name);
  BOOST_REQUIRE(deviceClass != nullptr);
  return deviceClass->attributes;
}

/**
 * @brief Assert that the mapper file written for the given attribute XML is rejected with a logic_error.
 */
static void expectMapperRejects(const std::string& caseName, const std::string& attributesXml) {
  TangoAdapter::AttributeMapper mapper;
  BOOST_CHECK_THROW(readMapperFor(caseName, attributesXml, mapper), ChimeraTK::logic_error);
}

/**********************************************************************************************************************/

/**
 * @brief The `<eventing>` text is mapped to the eventing mode, and an absent element means Off.
 */
BOOST_AUTO_TEST_CASE(testMapperParsesEventing) {
  const std::vector<std::pair<std::string, TangoAdapter::EventingMode>> cases{
      {"dataReady", TangoAdapter::EventingMode::DataReady},
      {"change", TangoAdapter::EventingMode::Change},
      {"off", TangoAdapter::EventingMode::Off},
      {"", TangoAdapter::EventingMode::Off},
  };

  for(const auto& [text, expected] : cases) {
    const auto attributes = parseAttributesOf(text.empty() ? "absent" : text,
        attributeWithEventing("INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR", text));

    BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
    BOOST_TEST(attributes.front().eventing == expected);
  }
}

/**********************************************************************************************************************/

/**
 * @brief DataReady is parsed for a Void source, which accepts all three values.
 */
BOOST_AUTO_TEST_CASE(testMapperParsesDataReadyForVoid) {
  const auto attributes = parseAttributesOf(
      "voidDataReady", attributeWithEventing("VOID_FROM_DEVICE_SCALAR", "VOID/FROM_DEVICE_SCALAR", "dataReady"));

  BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
  BOOST_TEST(attributes.front().eventing == TangoAdapter::EventingMode::DataReady);
}

/**********************************************************************************************************************/

/**
 * @brief A scalar attribute with `<eventing>change</eventing>` carries the mapped values, with eventing on.
 */
BOOST_AUTO_TEST_CASE(testMapperMapsScalarAttributeWithEventing) {
  const auto attributes = parseAttributesOf("scalarMapping",
      attribute("INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR",
          "                <description>An integer</description>\n                <egu>mm</egu>\n"
          "                <eventing>change</eventing>\n"));

  BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
  const auto& property = attributes.front();
  BOOST_TEST(property.name == "INT_FROM_DEVICE_SCALAR");
  BOOST_TEST(property.desc == "An integer");
  BOOST_TEST(property.unit == "mm");
  BOOST_TEST(property.attrDataFormat == TangoAdapter::AttrDataFormat::SCALAR);
  BOOST_TEST(property.dataType == Tango::DEV_LONG);
  BOOST_TEST(property.writeType == Tango::AttrWriteType::READ);
  BOOST_TEST(property.eventing == TangoAdapter::EventingMode::Change);
}

/**********************************************************************************************************************/

/**
 * @brief A spectrum attribute with `<eventing>dataReady</eventing>` carries the mapped values, with eventing on.
 */
BOOST_AUTO_TEST_CASE(testMapperMapsSpectrumAttributeWithEventing) {
  const auto attributes = parseAttributesOf("spectrumMapping",
      attribute("INT_FROM_DEVICE_ARRAY", "INT/FROM_DEVICE_ARRAY",
          "                <description>An array</description>\n                <egu>n/a</egu>\n"
          "                <eventing>dataReady</eventing>\n"));

  BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
  const auto& property = attributes.front();
  BOOST_TEST(property.name == "INT_FROM_DEVICE_ARRAY");
  BOOST_TEST(property.unit == "n/a");
  BOOST_TEST(property.length == 10U);
  BOOST_TEST(property.attrDataFormat == TangoAdapter::AttrDataFormat::SPECTRUM);
  BOOST_TEST(property.dataType == Tango::DEV_LONG);
  BOOST_TEST(property.writeType == Tango::AttrWriteType::READ);
  BOOST_TEST(property.eventing == TangoAdapter::EventingMode::DataReady);
}

/**********************************************************************************************************************/

/**
 * @brief A write-only attribute without `<eventing>` carries the mapped values, with eventing off.
 */
BOOST_AUTO_TEST_CASE(testMapperMapsWriteOnlyAttributeWithoutEventing) {
  const auto attributes =
      parseAttributesOf("writeOnlyMapping", attribute("INT_TO_DEVICE_SCALAR", "INT/TO_DEVICE_SCALAR"));

  BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
  const auto& property = attributes.front();
  BOOST_TEST(property.name == "INT_TO_DEVICE_SCALAR");
  BOOST_TEST(property.attrDataFormat == TangoAdapter::AttrDataFormat::SCALAR);
  BOOST_TEST(property.dataType == Tango::DEV_LONG);
  BOOST_TEST(property.writeType == Tango::AttrWriteType::WRITE);
  BOOST_TEST(property.eventing == TangoAdapter::EventingMode::Off);
}

/**********************************************************************************************************************/

/**
 * @brief An unknown `<eventing>` value is rejected when the mapper file is read.
 */
BOOST_AUTO_TEST_CASE(testInvalidEventingValueRejectedAtMapperRead) {
  expectMapperRejects(
      "invalidValue", attributeWithEventing("INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR", "invalidValue"));
}

/**********************************************************************************************************************/

/**
 * @brief An empty `<eventing>` element is rejected when the mapper file is read.
 */
BOOST_AUTO_TEST_CASE(testEmptyEventingValueRejectedAtMapperRead) {
  expectMapperRejects("emptyValue",
      attribute("INT_FROM_DEVICE_SCALAR", "INT/FROM_DEVICE_SCALAR", "                <eventing></eventing>\n"));
}

/**********************************************************************************************************************/

/**
 * @brief Eventing on a write-only attribute is rejected when the mapper file is read.
 */
BOOST_AUTO_TEST_CASE(testWriteOnlyEventingRejectedAtMapperRead) {
  expectMapperRejects("writeOnly", attributeWithEventing("INT_TO_DEVICE_SCALAR", "INT/TO_DEVICE_SCALAR", "change"));
}

/**********************************************************************************************************************/

/**
 * @brief Change is parsed for a Void source, whose CHANGE event carries the readout time stamp and the quality.
 */
BOOST_AUTO_TEST_CASE(testMapperParsesChangeForVoid) {
  const auto attributes = parseAttributesOf(
      "voidChange", attributeWithEventing("VOID_FROM_DEVICE_SCALAR", "VOID/FROM_DEVICE_SCALAR", "change"));

  BOOST_REQUIRE_EQUAL(attributes.size(), 1U);
  BOOST_TEST(attributes.front().eventing == TangoAdapter::EventingMode::Change);
}

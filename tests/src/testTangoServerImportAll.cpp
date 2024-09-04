// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <boost/mpl/list.hpp>
#include <boost/mpl/map.hpp>
#include <boost/test/included/unit_test.hpp>

using Fixture_t = TangoTestFixture<>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testVariableExistence) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  std::unique_ptr<std::vector<std::string>> attributes(proxy.get_attribute_list());
  // All of our attributes and the built-in "State" and "Status"
  BOOST_TEST(attributes->size() == 76);

  for(const auto* const directory : {"DOUBLE", "FLOAT", "LONG", "INT", "SHORT", "ULONG", "UCHAR", "UINT", "USHORT",
          "CHAR", "BOOLEAN", "VOID", "STRING"}) {
    for(const auto* const variable : {"CONSTANT_ARRAY", "FROM_DEVICE_ARRAY", "TO_DEVICE_ARRAY"}) {
      if(std::string(directory) == "VOID") {
        continue;
      }
      // if this throws the property does not exist. we should always be able to
      // read"
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable
                << " with tango name " << tangoName << std::endl;
      Tango::AttributeInfoEx attrInfo;
      BOOST_CHECK_NO_THROW(attrInfo = proxy.attribute_query(tangoName));
      BOOST_TEST(attrInfo.data_format == Tango::AttrDataFormat::SPECTRUM);
      BOOST_TEST(attrInfo.data_type == tf.name2TypeId(directory));
      std::cout << "test done " << std::endl;
    }

    for(const auto* const variable : {"DATA_TYPE_CONSTANT", "FROM_DEVICE_SCALAR", "TO_DEVICE_SCALAR"}) {
      if(std::string(directory) == "VOID" && std::string(variable) == "DATA_TYPE_CONSTANT") {
        continue;
      }
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable
                << " with tango name " << tangoName << std::endl;
      // if this throws the property does not exist. we should always be able to
      // read"
      Tango::AttributeInfoEx attrInfo;
      BOOST_CHECK_NO_THROW(attrInfo = proxy.attribute_query(tangoName));
      BOOST_TEST(attrInfo.data_format == Tango::AttrDataFormat::SCALAR);
      BOOST_TEST(attrInfo.data_type == tf.name2TypeId(directory));
      std::cout << "test done " << std::endl;
    }
  }
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE_TEMPLATE(testReadConstants, T, TestTypesNoVoid) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // Read contants
  {
    // Scalar
    auto name = tf.adapterType2Name<T>();
    auto tangoName = name + "_DATA_TYPE_CONSTANT";
    auto attr = proxy.read_attribute(tangoName);
    using TargetType = typename boost::mpl::at<Type2Tango, T>::type;

    auto expectedValue = tf.generateDefaultValueForType<TargetType, T>();

    TargetType value;
    attr >> value;
    BOOST_TEST(expectedValue == value);
  }
  {
    // Arrays
    auto name = tf.adapterType2Name<T>();
    auto tangoName = name + "_CONSTANT_ARRAY";
    auto attr = proxy.read_attribute(tangoName);
    using TargetType = typename boost::mpl::at<Type2Tango, T>::type;
    auto expectedValue = tf.generateDefaultValueForType<TargetType, T>();

    std::vector<TargetType> expectedValues;
    for(size_t i = 0; i < 10; i++) {
      if constexpr(std::is_same_v<ChimeraTK::Boolean, T>) {
        expectedValues.push_back(i == 0 ? false : expectedValue);
      }
      else if constexpr(std::is_same_v<std::string, T>) {
        expectedValues.push_back(std::to_string(double(i) * double(i) * 42.0));
      }
      else {
        expectedValues.push_back(TargetType(i) * TargetType(i) * expectedValue);
      }
    }

    std::vector<TargetType> values;
    attr >> values;
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedValues.begin(), expectedValues.end(), values.begin(), values.end());
  }
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE testAttributeProperty

#include <boost/test/included/unit_test.hpp>

#include "AttributeProperty.h"

#include <ChimeraTK/Exception.h>

#include <tango/tango.h>

BOOST_AUTO_TEST_CASE(testMapperParsing)
{
  constexpr std::string_view GOOD_PROPERTY{"INT_TO_DEVICE_SCALAR;INT/TO_DEVICE_SCALAR;SCALAR;DevLong;;"};

  BOOST_CHECK_NO_THROW(ChimeraTK::AttributeProperty(GOOD_PROPERTY.data()));
  ChimeraTK::AttributeProperty property(GOOD_PROPERTY.data());

  BOOST_TEST(property.name == "INT_TO_DEVICE_SCALAR");
  BOOST_TEST(property.path == "INT/TO_DEVICE_SCALAR");
  BOOST_TEST(property.attrDataFormat == ChimeraTK::AttrDataFormat::SCALAR);
  BOOST_TEST(property.dataType == Tango::DEV_LONG);
  BOOST_TEST(property.desc == "");
  BOOST_TEST(property.unit == "");


  constexpr std::string_view GOOD_PROPERTY_WITH_UNIT_AND_DESCRIPTION{"INT_TO_DEVICE_SCALAR;INT/TO_DEVICE_SCALAR;SCALAR;DevLong;Some description;SomeUnit"};
  BOOST_CHECK_NO_THROW(ChimeraTK::AttributeProperty(GOOD_PROPERTY_WITH_UNIT_AND_DESCRIPTION.data()));
  ChimeraTK::AttributeProperty property2(GOOD_PROPERTY_WITH_UNIT_AND_DESCRIPTION.data());
  BOOST_TEST(property2.desc == "Some description");
  BOOST_TEST(property2.unit == "SomeUnit");

  constexpr std::string_view PROPERTY_TOO_SHORT{"INT_TO_DEVICE_SCALAR;INT/TO_DEVICE_SCALAR;SCALAR;DevLong;"};
  BOOST_CHECK_THROW(ChimeraTK::AttributeProperty(PROPERTY_TOO_SHORT.data()), ChimeraTK::runtime_error);

  constexpr std::string_view INVALID_DATA_FORMAT{"INT_TO_DEVICE_SCALAR;INT/TO_DEVICE_SCALAR;CIRCULAR;DevLong;;"};
  BOOST_CHECK_THROW(ChimeraTK::AttributeProperty(INVALID_DATA_FORMAT.data()), ChimeraTK::runtime_error);

  constexpr std::string_view INVALID_TYPE{"INT_TO_DEVICE_SCALAR;INT/TO_DEVICE_SCALAR;SCALAR;DevCoffee;;"};
  BOOST_CHECK_THROW(ChimeraTK::AttributeProperty(INVALID_TYPE.data()), ChimeraTK::runtime_error);
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoTestServer.h"

#include <boost/test/included/unit_test.hpp>

#include <tango/tango.h>

using Fixture_t = TangoTestFixture<>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

static int textToTangoType(const std::string& name) {
  static std::map<std::string, int> name2Type{
      {"DOUBLE", Tango::DEV_DOUBLE},
      {"FLOAT", Tango::DEV_FLOAT},
      {"INT", Tango::DEV_LONG},
      {"SHORT", Tango::DEV_SHORT},
      {"UCHAR", Tango::DEV_UCHAR},
      {"UINT", Tango::DEV_ULONG},
      {"USHORT", Tango::DEV_USHORT},
      {"CHAR", Tango::DEV_SHORT},
      {"VOID", Tango::DEV_BOOLEAN},
      {"BOOLEAN", Tango::DEV_BOOLEAN},
      };

  return name2Type.at(name);
}

BOOST_AUTO_TEST_CASE(testVariableExistence) {
  for(const auto* const directory : {"DOUBLE", "FLOAT", "INT", "SHORT", "UCHAR", "UINT", "USHORT", "CHAR", "BOOLEAN", "VOID"}) {
    for(const auto* const variable : {"CONSTANT_ARRAY", "FROM_DEVICE_ARRAY", "TO_DEVICE_ARRAY"}) {
      if (std::string(directory) == "VOID") {
        continue;
      }
      // if this throws the property does not exist. we should always be able to
      // read"
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      Tango::AttributeInfoEx attrInfo;
      BOOST_CHECK_NO_THROW(attrInfo = TangoTestFixtureImpl::f->proxy->attribute_query(tangoName));
      BOOST_TEST(attrInfo.data_format == Tango::AttrDataFormat::SPECTRUM);
      BOOST_TEST(attrInfo.data_type == textToTangoType(directory));
      std::cout << "test done " << std::endl;
    }

    for(const auto *const variable : {"DATA_TYPE_CONSTANT", "FROM_DEVICE_SCALAR", "TO_DEVICE_SCALAR"}) {
      if (std::string(directory) == "VOID" && std::string(variable) == "DATA_TYPE_CONSTANT") {
        continue;
      }
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      // if this throws the property does not exist. we should always be able to
      // read"
      Tango::AttributeInfoEx attrInfo;
      BOOST_CHECK_NO_THROW(attrInfo = TangoTestFixtureImpl::f->proxy->attribute_query(tangoName));
      BOOST_TEST(attrInfo.data_format == Tango::AttrDataFormat::SCALAR);
      BOOST_TEST(attrInfo.data_type == textToTangoType(directory));
      std::cout << "test done " << std::endl;
    }
  }
}

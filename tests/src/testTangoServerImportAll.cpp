// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestImportAll

#include "TangoTestServer.h"

#include <boost/test/included/unit_test.hpp>

#include <tango/tango.h>


BOOST_GLOBAL_FIXTURE(TangoTestFixture);

BOOST_AUTO_TEST_CASE(testVariableExistence) {
  for(const auto* const directory : {"DOUBLE", "FLOAT", "INT", "SHORT", "UCHAR", "UINT", "USHORT", "CHAR"}) {
    for(const auto* const variable : {"CONSTANT_ARRAY", "FROM_DEVICE_ARRAY", "TO_DEVICE_ARRAY"}) {
      // if this throws the property does not exist. we should always be able to
      // read"
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      BOOST_CHECK_NO_THROW(TangoTestFixture::f->proxy->attribute_query(tangoName));
      std::cout << "test done " << std::endl;
    }

    for(const auto *const variable : {"DATA_TYPE_CONSTANT", "FROM_DEVICE_SCALAR", "TO_DEVICE_SCALAR"}) {
      auto tangoName = std::string(directory) + "_" + variable;
      std::cout << "testing existence of " << std::string("//MASTER/") + directory + "." + variable << " with tango name " << tangoName << std::endl;
      // if this throws the property does not exist. we should always be able to
      // read"
      BOOST_CHECK_NO_THROW(TangoTestFixture::f->proxy->attribute_query(tangoName));
      std::cout << "test done " << std::endl;
    }
  }
}

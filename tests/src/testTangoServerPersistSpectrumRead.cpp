// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#define BOOST_TEST_MODULE serverTestPersistSpectrum

#include "TangoTestServer.h"
#include <tango/tango.h>

#include <boost/mpl/list.hpp>
#include <boost/mpl/map.hpp>
#include <boost/test/included/unit_test.hpp>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setCreateOfflineDatabase(false)
        .setOfflineDatabase("testPersistSpectrumDatabase")
        .overrideNames("serverTestPersistSpectrum");
  }
};
using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE_TEMPLATE(testReadPersistedArray, T, TestTypesNoVoid) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();
  auto tangoName = tf.adapterType2Name<T>() + "_TO_DEVICE_ARRAY";
  using TargetType = typename boost::mpl::at<Type2Tango, T>::type;

  auto value = tf.generateDefaultValueForType<TargetType, T>();

  auto expectedValues = std::vector<TargetType>(10, value);
  std::vector<TargetType> values;
  auto attr = proxy.read_attribute(tangoName);
  attr >> values;

  BOOST_CHECK_EQUAL_COLLECTIONS(values.begin(), values.end(), expectedValues.begin(), expectedValues.end());
}

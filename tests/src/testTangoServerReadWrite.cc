// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestReadWrite

#include "TangoTestServer.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include <tango/tango.h>


struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
  }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

BOOST_AUTO_TEST_CASE_TEMPLATE(testReadWriteScalar, T, TestTypes) {
  auto [tf, app, _] = TangoTestFixtureImpl::getContents();
  using TangoType = typename boost::mpl::at<Type2Tango, T>::type;

  auto tangoWriteName = tf.adapterType2Name<T>() + "_TO_DEVICE_SCALAR";
  auto value = tf.generateDefaultValueForType<TangoType, T>();
  if constexpr(!std::is_same_v<ChimeraTK::Boolean, T> && !std::is_same_v<ChimeraTK::Void, T> && !std::is_same_v<std::string, T>) {
    value *= 3;
  }

  tf.write(tangoWriteName, value);
  app.runMainLoopOnce();

  auto tangoReadName = tf.adapterType2Name<T>() + "_FROM_DEVICE_SCALAR";

  BOOST_TEST(tf.checkWithTimeout(tangoReadName, value));
}


BOOST_AUTO_TEST_CASE_TEMPLATE(testReadWriteSpectrum, T, TestTypesNoVoid) {
  auto [tf, app, _] = TangoTestFixtureImpl::getContents();
  using TangoType = typename boost::mpl::at<Type2Tango, T>::type;

  auto tangoWriteName = tf.adapterType2Name<T>() + "_TO_DEVICE_ARRAY";
  auto value = tf.generateDefaultValueForType<TangoType, T>();
  std::vector<TangoType> values;
  values.reserve(10);
  for (int i = 0; i < 10; i++) {
    if constexpr(std::is_same_v<ChimeraTK::Boolean, T> || std::is_same_v<std::string, T>) {
      values.push_back(value);
    } else {
      values.push_back(value * 9 * i);
    }
  }
  tf.write(tangoWriteName, values);
  app.runMainLoopOnce();

  auto tangoReadName = tf.adapterType2Name<T>() + "_FROM_DEVICE_ARRAY";

  BOOST_TEST(tf.checkWithTimeout(tangoReadName, values));
}

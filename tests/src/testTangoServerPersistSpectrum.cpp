// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#define BOOST_TEST_MODULE serverTestPersistSpectrum

#include "TangoTestServer.h"

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/map.hpp>

#include <tango/tango.h>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setOfflineDatabase("testPersistSpectrumDatabase")
        .setKeepOfflineDatabase(true);
  }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

using Type2Tango = boost::mpl::map<
    boost::mpl::pair<double, Tango::DevDouble>,
    boost::mpl::pair<float, Tango::DevFloat>,
    boost::mpl::pair<int32_t, Tango::DevLong>,
    boost::mpl::pair<int16_t, Tango::DevShort>,
    boost::mpl::pair<int8_t, Tango::DevShort>,
    boost::mpl::pair<uint32_t, Tango::DevULong>,
    boost::mpl::pair<uint16_t, Tango::DevUShort>,
    boost::mpl::pair<uint8_t, Tango::DevUChar>,
    boost::mpl::pair<ChimeraTK::Boolean, Tango::DevBoolean>
    >;

template <typename T>
std::string typeToName() {
  if constexpr(std::is_same_v<T, double>) {
    return "DOUBLE";
  }
  if constexpr(std::is_same_v<T, float>) {
    return "FLOAT";
  }

  if constexpr(std::is_same_v<T, int32_t>) {
    return "INT";
  }
  if constexpr(std::is_same_v<T, int16_t>) {
    return "SHORT";

  }
  if constexpr(std::is_same_v<T, int8_t>) {
    return "CHAR";
  }

  if constexpr(std::is_same_v<T, uint32_t>) {
    return "UINT";
  }
  if constexpr(std::is_same_v<T, uint16_t>) {
    return "USHORT";

  }
  if constexpr(std::is_same_v<T, uint8_t>) {
    return "UCHAR";
  }

  if constexpr(std::is_same_v<T, ChimeraTK::Boolean>) {
    return "BOOLEAN";
  }
}

using TestTypes = boost::mpl::list<double, float, int32_t, int16_t, int8_t, uint32_t, uint16_t, uint8_t, ChimeraTK::Boolean>;

BOOST_AUTO_TEST_CASE_TEMPLATE(testWriteArray, T, TestTypes) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();
  auto tangoName = typeToName<T>() + "_TO_DEVICE_ARRAY";
  using TargetType = typename boost::mpl::at<Type2Tango, T>::type;

  TargetType value;
  if constexpr(std::is_same_v<Tango::DevBoolean, TargetType>) {
    value = true;
  } else {
    value = (std::is_signed_v<T> ? -1 : 1) * TargetType(sizeof(T));
  }

  auto values = std::vector<TargetType>(10, value);
  tf.write(tangoName, values);
}

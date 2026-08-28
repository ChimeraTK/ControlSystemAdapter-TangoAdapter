// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestEventing

#include "TangoTestServer.h"

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**********************************************************************************************************************/

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) {
    f.setManualLoopControl(true);
    f.theServer.setOfflineDatabase("testEventing");
  }
};
using Fixture_t = TangoTestFixture<TestFixtureConfig>;

BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

namespace {
  /// Wait (bounded) for a predicate, mirroring the checkWithTimeout pattern.
  template<typename Predicate>
  bool waitWithTimeout(Predicate predicate, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    using clock = std::chrono::steady_clock;
    auto deadline = clock::now() + timeout;
    while(clock::now() < deadline) {
      if(predicate()) {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return predicate();
  }
} // namespace

/**********************************************************************************************************************/

/// Thread-safe holder for received data-ready events.
class DataReadyReceiver : public Tango::CallBack {
 public:
  void push_event(Tango::DataReadyEventData* dre) override {
    std::lock_guard<std::mutex> lock(_mtx);
    _attrName = dre->attr_name;
    _err = dre->err;
    ++_count;
    _cv.notify_all();
  }

  /// Wait until at least \p n events have arrived.
  bool waitForCount(int n, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    std::unique_lock<std::mutex> lock(_mtx);
    return _cv.wait_for(lock, timeout, [this, n] { return _count >= n; });
  }

  std::string attrName() {
    std::lock_guard<std::mutex> lock(_mtx);
    return _attrName;
  }

  bool err() {
    std::lock_guard<std::mutex> lock(_mtx);
    return _err;
  }

 private:
  std::mutex _mtx;
  std::condition_variable _cv;
  int _count{0};
  std::string _attrName;
  bool _err{false};
};

/**********************************************************************************************************************/

/// Thread-safe holder for received change (EventData) events.
class ChangeReceiver : public Tango::CallBack {
 public:
  void push_event(Tango::EventData* ed) override {
    std::lock_guard<std::mutex> lock(_mtx);
    _err = ed->err;
    if(!_err && ed->attr_value != nullptr && !ed->attr_value->is_empty()) {
      _hasValue = true;
      _devAttr = *ed->attr_value;
    }
  }

  bool err() {
    std::lock_guard<std::mutex> lock(_mtx);
    return _err;
  }

  bool hasValue() {
    std::lock_guard<std::mutex> lock(_mtx);
    return _hasValue;
  }

  Tango::DevLong scalar() {
    std::lock_guard<std::mutex> lock(_mtx);
    Tango::DevLong value{0};
    _devAttr >> value;
    return value;
  }

  std::vector<Tango::DevLong> spectrum() {
    std::lock_guard<std::mutex> lock(_mtx);
    std::vector<Tango::DevLong> values;
    _devAttr >> values;
    return values;
  }

  /// Wait until the last stored spectrum equals \p target.
  bool waitForSpectrum(
      const std::vector<Tango::DevLong>& target, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    return waitWithTimeout(
        [this, &target] {
          std::lock_guard<std::mutex> lock(_mtx);
          if(!_hasValue || _err) {
            return false;
          }
          std::vector<Tango::DevLong> values;
          _devAttr >> values;
          return values == target;
        },
        timeout);
  }

  /// Wait until the last stored scalar equals \p target.
  bool waitForScalar(Tango::DevLong target, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    return waitWithTimeout(
        [this, target] {
          std::lock_guard<std::mutex> lock(_mtx);
          if(!_hasValue || _err) {
            return false;
          }
          Tango::DevLong value{0};
          _devAttr >> value;
          return value == target;
        },
        timeout);
  }

 private:
  std::mutex _mtx;
  bool _err{false};
  bool _hasValue{false};
  Tango::DeviceAttribute _devAttr{};
};

/**********************************************************************************************************************/

static void driveScalarOnce(TangoTestFixtureImpl& tf, ExtendedReferenceTestApplication& app, Tango::DevLong value) {
  tf.write(std::string("INT_TO_DEVICE_SCALAR"), value);
  app.runMainLoopOnce();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataReadyScalar) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  DataReadyReceiver receiver;
  int eventId = proxy.subscribe_event("DR_SCALAR", Tango::DATA_READY_EVENT, &receiver);

  constexpr Tango::DevLong VALUE = 42;
  driveScalarOnce(tf, app, VALUE);

  BOOST_CHECK(receiver.waitForCount(1));
  BOOST_CHECK(!receiver.err());
  BOOST_CHECK(receiver.attrName().find(std::string("dr_scalar")) != std::string::npos);

  BOOST_CHECK(tf.checkWithTimeout("DR_SCALAR", VALUE));

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataReadySpectrum) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  DataReadyReceiver receiver;
  int eventId = proxy.subscribe_event("DR_SPECTRUM", Tango::DATA_READY_EVENT, &receiver);

  std::vector<Tango::DevLong> values(10, 7);
  tf.write(std::string("INT_TO_DEVICE_ARRAY"), values);
  app.runMainLoopOnce();

  BOOST_CHECK(receiver.waitForCount(1));
  BOOST_CHECK(!receiver.err());
  BOOST_CHECK(receiver.attrName().find(std::string("dr_spectrum")) != std::string::npos);

  auto read = proxy.read_attribute("DR_SPECTRUM");
  std::vector<Tango::DevLong> readValues;
  read >> readValues;
  BOOST_CHECK(readValues == values);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataChangeScalar) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  ChangeReceiver receiver;
  int eventId = proxy.subscribe_event("DATA_SCALAR", Tango::CHANGE_EVENT, &receiver);

  constexpr Tango::DevLong VALUE1 = 11;
  constexpr Tango::DevLong VALUE2 = 22;

  driveScalarOnce(tf, app, VALUE1);
  driveScalarOnce(tf, app, VALUE2);

  BOOST_CHECK(receiver.waitForScalar(VALUE2));
  BOOST_CHECK(!receiver.err());
  BOOST_CHECK(receiver.hasValue());
  BOOST_CHECK_EQUAL(receiver.scalar(), VALUE2);

  BOOST_CHECK(tf.checkWithTimeout("DATA_SCALAR", VALUE2));

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataChangeSpectrum) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  ChangeReceiver receiver;
  int eventId = proxy.subscribe_event("DATA_SPECTRUM", Tango::CHANGE_EVENT, &receiver);

  std::vector<Tango::DevLong> values1(10, 3);
  tf.write(std::string("INT_TO_DEVICE_ARRAY"), values1);
  app.runMainLoopOnce();

  std::vector<Tango::DevLong> values2(10, 5);
  tf.write(std::string("INT_TO_DEVICE_ARRAY"), values2);
  app.runMainLoopOnce();

  BOOST_CHECK(receiver.waitForSpectrum(values2));
  BOOST_CHECK(!receiver.err());
  BOOST_CHECK(receiver.hasValue());

  // The event payload must carry the spectrum contents with the updated values.
  auto received = receiver.spectrum();
  BOOST_CHECK(received == values2);

  auto read = proxy.read_attribute("DATA_SPECTRUM");
  std::vector<Tango::DevLong> readValues;
  read >> readValues;
  BOOST_CHECK(readValues == values2);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testEventingNone_NoEvents) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // A data-ready subscription on a non-enabled attribute must throw.
  DataReadyReceiver dataReadyReceiver;
  bool threw = false;
  try {
    proxy.subscribe_event("NONE_SCALAR", Tango::DATA_READY_EVENT, &dataReadyReceiver);
  }
  catch(Tango::DevFailed&) {
    threw = true;
  }
  BOOST_CHECK(threw);

  // A change-event subscription on a "none" attribute must throw as well (change events are not enabled).
  ChangeReceiver changeReceiver;
  bool changeThrew = false;
  try {
    proxy.subscribe_event("NONE_SCALAR", Tango::CHANGE_EVENT, &changeReceiver);
  }
  catch(Tango::DevFailed&) {
    changeThrew = true;
  }
  BOOST_CHECK(changeThrew);
}

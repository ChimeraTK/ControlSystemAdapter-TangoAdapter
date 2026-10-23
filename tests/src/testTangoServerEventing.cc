// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestEventing

#include "TangoTestServer.h"

#include <tango/tango.h>

#include <boost/test/included/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

struct TestFixtureConfig {
  static void apply(TangoTestFixtureImpl& f) { f.setManualLoopControl(true); }
};

using Fixture_t = TangoTestFixture<TestFixtureConfig>;
BOOST_GLOBAL_FIXTURE(Fixture_t);

/**********************************************************************************************************************/

/// Poll the given condition for up to the Tango event timeout.
template<typename Predicate>
static bool pollUntil(Predicate predicate, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  while(clock::now() - start < timeout) {
    if(predicate()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return false;
}

/**
 * @brief Collects Tango events delivered to a callback for later inspection.
 *
 * The Tango event machinery delivers events on its own thread, so all access is guarded by a mutex.
 */
class EventCollector : public Tango::CallBack {
 public:
  void push_event(Tango::EventData* ed) override {
    std::lock_guard<std::mutex> lg(_mutex);
    if(ed->err) {
      return;
    }
    if(ed->attr_value != nullptr) {
      _changeEvents.emplace_back(*ed->attr_value);
    }
  }

  void push_event(Tango::DataReadyEventData* dre) override {
    std::lock_guard<std::mutex> lg(_mutex);
    if(dre->err) {
      return;
    }
    _dataReadyCounters.push_back(dre->ctr);
  }

  /// Whether a change event satisfying the predicate has been received so far.
  template<typename Predicate>
  bool hasChangeEvent(Predicate predicate) {
    std::lock_guard<std::mutex> lg(_mutex);
    return std::any_of(_changeEvents.begin(), _changeEvents.end(),
        [&predicate](Tango::DeviceAttribute& attr) { return predicate(attr); });
  }

  /// The last change event satisfying the predicate, if any.
  template<typename Predicate>
  std::optional<Tango::DeviceAttribute> lastChangeEvent(Predicate predicate) {
    std::lock_guard<std::mutex> lg(_mutex);
    for(auto& event : std::views::reverse(_changeEvents)) {
      if(predicate(event)) {
        return event;
      }
    }
    return std::nullopt;
  }

  /// The last data ready counter received so far, if any.
  int lastDataReadyCounter() {
    std::lock_guard<std::mutex> lg(_mutex);
    assert(!_dataReadyCounters.empty());
    return _dataReadyCounters.back();
  }

  /// Number of change events received so far.
  [[nodiscard]] size_t changeEventCount() {
    std::lock_guard<std::mutex> lg(_mutex);
    return _changeEvents.size();
  }

  /// Number of data ready events received so far.
  [[nodiscard]] size_t dataReadyEventCount() {
    std::lock_guard<std::mutex> lg(_mutex);
    return _dataReadyCounters.size();
  }

 private:
  std::mutex _mutex;
  std::vector<Tango::DeviceAttribute> _changeEvents;
  std::vector<int> _dataReadyCounters;
};

/**********************************************************************************************************************/

/**
 * @brief Repeat a write and one main loop iteration until the given condition holds.
 *
 * A single main loop iteration only copies a to-device variable if it has received a new value since the
 * last iteration, so the write issued by the test client may not have arrived in time yet. Repeating the
 * pair makes the test independent of that timing without weakening the assertion.
 */
template<typename WriteFunction, typename Predicate>
static bool pumpUntilEvent(ExtendedReferenceTestApplication& app, WriteFunction write, Predicate condition) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  do {
    write();
    app.runMainLoopOnce();
    if(pollUntil(condition, std::chrono::milliseconds(100))) {
      return true;
    }
  } while(std::chrono::steady_clock::now() < deadline);

  return condition();
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testChangeEventCarriesWrittenValue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("INT_FROM_DEVICE_SCALAR", Tango::CHANGE_EVENT, &collector);

  app.dataValidity = ChimeraTK::DataValidity::ok;
  const auto hasWrittenValue = [&collector] {
    return collector.hasChangeEvent([](Tango::DeviceAttribute& attr) {
      Tango::DevLong value = 0;
      attr >> value;
      return value == 42;
    });
  };
  BOOST_REQUIRE(pumpUntilEvent(app, [&tf] { tf.write("INT_TO_DEVICE_SCALAR", Tango::DevLong(42)); }, hasWrittenValue));

  auto event = collector.lastChangeEvent([](Tango::DeviceAttribute& attr) {
    Tango::DevLong value = 0;
    attr >> value;
    return value == 42;
  });
  BOOST_REQUIRE(event.has_value());
  Tango::DevLong value = 0;
  (*event) >> value;
  BOOST_TEST(value == 42);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testChangeEventOfSpectrumCarriesAllElements) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("INT_FROM_DEVICE_ARRAY", Tango::CHANGE_EVENT, &collector);

  const std::vector<Tango::DevLong> written{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  app.dataValidity = ChimeraTK::DataValidity::ok;
  const auto hasWrittenValues = [&collector, &written] {
    return collector.hasChangeEvent([&written](Tango::DeviceAttribute& attr) {
      std::vector<Tango::DevLong> read;
      attr >> read;
      return read == written;
    });
  };
  BOOST_REQUIRE(pumpUntilEvent(app, [&tf, &written] { tf.write("INT_TO_DEVICE_ARRAY", written); }, hasWrittenValues));

  auto event = collector.lastChangeEvent([](Tango::DeviceAttribute&) { return true; });
  BOOST_REQUIRE(event.has_value());
  std::vector<Tango::DevLong> read;
  (*event) >> read;
  BOOST_TEST(read == written, boost::test_tools::per_element());

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataReadyEventCounterIncreases) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("SHORT_FROM_DEVICE_SCALAR", Tango::DATA_READY_EVENT, &collector);

  app.dataValidity = ChimeraTK::DataValidity::ok;
  const auto hasFirstEvent = [&collector] { return collector.dataReadyEventCount() >= 1; };
  BOOST_REQUIRE(pumpUntilEvent(app, [&tf] { tf.write("SHORT_TO_DEVICE_SCALAR", Tango::DevShort(1)); }, hasFirstEvent));
  const auto firstCounter = collector.lastDataReadyCounter();

  // A new write must produce a further data ready event with a strictly larger counter.
  const auto hasLaterCounter = [&collector, firstCounter] { return collector.lastDataReadyCounter() > firstCounter; };
  BOOST_REQUIRE(
      pumpUntilEvent(app, [&tf] { tf.write("SHORT_TO_DEVICE_SCALAR", Tango::DevShort(2)); }, hasLaterCounter));
  const auto secondCounter = collector.lastDataReadyCounter();

  BOOST_TEST(secondCounter > firstCounter);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testDataReadyEventOfSpectrum) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("SHORT_FROM_DEVICE_ARRAY", Tango::DATA_READY_EVENT, &collector);

  app.dataValidity = ChimeraTK::DataValidity::ok;
  const std::vector<Tango::DevShort> firstWritten{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  const auto hasFirstEvent = [&collector] { return collector.dataReadyEventCount() >= 1; };
  BOOST_REQUIRE(
      pumpUntilEvent(app, [&tf, &firstWritten] { tf.write("SHORT_TO_DEVICE_ARRAY", firstWritten); }, hasFirstEvent));
  const auto firstCounter = collector.lastDataReadyCounter();

  // A second write must produce a further data ready event with a strictly larger counter.
  const std::vector<Tango::DevShort> secondWritten{10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  const auto hasLaterCounter = [&collector, firstCounter] { return collector.lastDataReadyCounter() > firstCounter; };
  BOOST_REQUIRE(pumpUntilEvent(
      app, [&tf, &secondWritten] { tf.write("SHORT_TO_DEVICE_ARRAY", secondWritten); }, hasLaterCounter));
  const auto secondCounter = collector.lastDataReadyCounter();

  BOOST_TEST(secondCounter > firstCounter);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testOffAttributeProducesNoEvent) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  // An attribute without event generation must reject event subscriptions
  BOOST_CHECK_THROW(proxy.subscribe_event("DOUBLE_FROM_DEVICE_SCALAR", Tango::CHANGE_EVENT, nullptr), Tango::DevFailed);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testVoidChangeEventCarriesConstantTrue) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("VOID_FROM_DEVICE_SCALAR", Tango::CHANGE_EVENT, &collector);

  // Subscribing delivers an initial event, so wait for it before writing and only regard later events as
  // write-triggered.
  BOOST_REQUIRE(pollUntil([&collector] { return collector.changeEventCount() >= 1; }));
  const auto initialEventCount = collector.changeEventCount();

  app.dataValidity = ChimeraTK::DataValidity::ok;
  const auto hasNewEvent = [&collector, initialEventCount] { return collector.changeEventCount() > initialEventCount; };
  BOOST_REQUIRE(
      pumpUntilEvent(app, [&tf] { tf.write("VOID_TO_DEVICE_SCALAR", Tango::DevBoolean(true)); }, hasNewEvent));

  auto event = collector.lastChangeEvent([](Tango::DeviceAttribute&) { return true; });
  BOOST_REQUIRE(event.has_value());
  Tango::DevBoolean value = false;
  (*event) >> value;
  BOOST_TEST(value == true);
  BOOST_TEST((*event).get_quality() == Tango::AttrQuality::ATTR_VALID);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testChangeEventCarriesInvalidQualityForFaultyData) {
  auto [tf, app, proxy] = TangoTestFixtureImpl::getContents();

  EventCollector collector;
  const auto eventId = proxy.subscribe_event("FLOAT_FROM_DEVICE_SCALAR", Tango::CHANGE_EVENT, &collector);

  app.dataValidity = ChimeraTK::DataValidity::faulty;
  const auto hasInvalidEvent = [&collector] {
    return collector.hasChangeEvent(
        [](Tango::DeviceAttribute& attr) { return attr.get_quality() == Tango::AttrQuality::ATTR_INVALID; });
  };
  BOOST_REQUIRE(
      pumpUntilEvent(app, [&tf] { tf.write("FLOAT_TO_DEVICE_SCALAR", Tango::DevFloat(3.5F)); }, hasInvalidEvent));

  auto event = collector.lastChangeEvent(
      [](Tango::DeviceAttribute& attr) { return attr.get_quality() == Tango::AttrQuality::ATTR_INVALID; });
  BOOST_REQUIRE(event.has_value());
  BOOST_TEST((*event).get_quality() == Tango::AttrQuality::ATTR_INVALID);

  proxy.unsubscribe_event(eventId);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testInvalidEventingValueRejectedAtMapperRead) {
  const auto status = runTestTangoServer(std::filesystem::current_path() / "rejectInvalidEventingValue",
      "TestEventingReject", "tango/test/testtangoserverinvalideventing");

  BOOST_TEST(status != 0);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWriteOnlyEventingRejectedAtMapperRead) {
  const auto status = runTestTangoServer(std::filesystem::current_path() / "rejectWriteOnlyEventing",
      "TestEventingReject", "tango/test/testtangoserverwriteonlyeventing");

  BOOST_TEST(status != 0);
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ExtendedReferenceTestApplication.h"
#include <tango/tango.h>

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

#include <boost/mpl/list.hpp>
#include <boost/mpl/map.hpp>
#include <boost/test/unit_test.hpp>

#include <thread>

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

using TestTypesNoVoid = boost::mpl::list<double, float, int64_t, int32_t, int16_t, int8_t, uint64_t, uint32_t, uint16_t,
    uint8_t, ChimeraTK::Boolean, std::string>;
using TestTypes = boost::mpl::list<double, float, int64_t, int32_t, int16_t, int8_t, uint64_t, uint32_t, uint16_t,
    uint8_t, ChimeraTK::Boolean, ChimeraTK::Void, std::string>;

using Type2Tango = boost::mpl::map<boost::mpl::pair<double, Tango::DevDouble>, boost::mpl::pair<float, Tango::DevFloat>,
    boost::mpl::pair<int64_t, Tango::DevLong64>, boost::mpl::pair<int32_t, Tango::DevLong>,
    boost::mpl::pair<int16_t, Tango::DevShort>, boost::mpl::pair<int8_t, Tango::DevShort>,
    boost::mpl::pair<uint64_t, Tango::DevULong64>, boost::mpl::pair<uint32_t, Tango::DevULong>,
    boost::mpl::pair<uint16_t, Tango::DevUShort>, boost::mpl::pair<uint8_t, Tango::DevUChar>,
    boost::mpl::pair<ChimeraTK::Boolean, Tango::DevBoolean>, boost::mpl::pair<ChimeraTK::Void, Tango::DevBoolean>,
    boost::mpl::pair<std::string, std::string>>;

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

struct ThreadedTangoServer {
  explicit ThreadedTangoServer(std::string setTestName, bool setVerbose = false);
  ~ThreadedTangoServer();

  /*std::string t the Tango server
   *
   * The code makes sure that the server should be started enough that a proxy can connect to it
   * The fixture will then wait for the server to go into state "ON"
   */
  void start();

  /**
   * @brief Shut down the Tango server
   */
  void stop();

  /**
   * @brief Get the port used for CORBA commuication
   * @return The CORBA port of this server
   */
  static std::string port();

  /**
   * @brief An URL that can be used by the Tango::DeviceProxy to connect to this server
   * @return
   */
  [[nodiscard]] std::string getClientUrl() const;

  /**
   * @brief The Tango device name
   * @return The device name used for this server
   */
  [[nodiscard]] const std::string& device() const;

  /**
   * @brief Configure the Tango server to run against an offline database
   * @param basePath base name of the db file. A template database with the name basename + ".db"
   * has to exist in the working folder.
   */
  ThreadedTangoServer& setOfflineDatabase(const std::string& basePath);

  ThreadedTangoServer& setKeepOfflineDatabase(bool keep);

  ThreadedTangoServer& setCreateOfflineDatabase(bool create);

  ThreadedTangoServer& overrideNames(const std::string& newNames);

  /**
   * @brief Set property on the server
   * @param name The name of the property
   * @param value The value
   *
   * Back-door function to modify a property since it is not possible to
   * call put_property from the client in the test.
   */
  template<typename T>
  void setProperty(const std::string& name, T value);

  template<typename T>
  T getClassProperty(const std::string& name);

  std::string testName;
  std::string offlineDatabase;
  bool createOfflineDatabase{true};
  bool keepOfflineDatabase{false};
  bool verbose{false};
  std::vector<std::string> argv{};
  std::thread tangoServerThread;
  std::string deviceString{};
  Tango::Util* tg{nullptr};
  static std::atomic<bool> shutdownRequested;
};

/**********************************************************************************************************************/

template<typename T>
inline void ThreadedTangoServer::setProperty(const std::string& name, T value) {
  Tango::DbDatum dbDatum(name);
  dbDatum << value;
  std::vector<Tango::DbDatum> list{dbDatum};
  tg->get_device_by_name(device())->get_db_device()->put_property(list);
}

template<typename T>
inline T ThreadedTangoServer::getClassProperty(const std::string& name) {
  Tango::DbDatum dbDatum(name);
  std::vector<Tango::DbDatum> list{dbDatum};
  const auto& classList = tg->get_class_list();
  for(auto* c : *classList) {
    if(c->get_name() == testName) {
      classList->at(0)->get_db_class()->get_property(list);

      T value;
      dbDatum >> value;

      return value;
    }
  }

  assert(false);

  return {};
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

struct TangoTestFixtureImpl {
  TangoTestFixtureImpl();
  virtual ~TangoTestFixtureImpl();

  void setManualLoopControl(bool manualControl);
  void startup();

  template<typename T>
  void write(std::string attribute, T value);

  template<typename T>
  bool checkWithTimeout(std::string readName, T referenceValues);

  static int name2TypeId(const std::string& name);

  template<typename T>
  static std::string adapterType2Name();

  template<typename TargetType, typename T>
  static TargetType generateDefaultValueForType();

  bool manualLoopControl{false};
  std::list<ExtendedReferenceTestApplication::VariableHolder> additionalVarables{};

  ChimeraTK::ApplicationFactory<ExtendedReferenceTestApplication> theFactory;
  ThreadedTangoServer theServer;
  ExtendedReferenceTestApplication* theApp{nullptr};
  std::unique_ptr<Tango::DeviceProxy> proxy{nullptr};
  static TangoTestFixtureImpl* f;

  static std::tuple<TangoTestFixtureImpl&, ExtendedReferenceTestApplication&, Tango::DeviceProxy&> getContents() {
    auto& fixture = *f;
    auto& app = *f->theApp;
    auto& proxy = *f->proxy;
    return {fixture, app, proxy};
  }
};

/**********************************************************************************************************************/

template<typename T>
inline bool TangoTestFixtureImpl::checkWithTimeout(std::string readName, T referenceValues) {
  const auto TIMEOUT = std::chrono::seconds(5);
  using clock = std::chrono::steady_clock;
  auto now = clock::now();

  T readValues;
  do {
    readValues = {};
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    try {
      proxy->read_attribute(readName) >> readValues;
    }
    catch(CORBA::Exception& ex) {
      Tango::Except::print_exception(ex);
      std::rethrow_exception(std::current_exception());
    }

  } while(referenceValues != readValues && (clock::now() - now) < TIMEOUT);

  return referenceValues == readValues;
}

/**********************************************************************************************************************/

template<typename TargetType, typename T>
inline TargetType TangoTestFixtureImpl::generateDefaultValueForType() {
  TargetType expectedValue;
  if constexpr(std::is_same_v<std::string, TargetType>) {
    expectedValue = std::to_string(42.0);
  }
  else if constexpr(std::is_same_v<Tango::DevBoolean, TargetType>) {
    expectedValue = true;
  }
  else {
    expectedValue = TargetType(sizeof(T));
    if constexpr(std::is_floating_point_v<TargetType>) {
      expectedValue = TargetType(1.) / expectedValue;
    }
    else {
      expectedValue *= (std::is_signed_v<T> ? -1 : 1);
    }
  }
  return expectedValue;
}

/**********************************************************************************************************************/

template<typename T>
inline std::string TangoTestFixtureImpl::adapterType2Name() {
  if constexpr(std::is_same_v<T, double>) {
    return "DOUBLE";
  }
  if constexpr(std::is_same_v<T, float>) {
    return "FLOAT";
  }

  if constexpr(std::is_same_v<T, int64_t>) {
    return "LONG";
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

  if constexpr(std::is_same_v<T, uint64_t>) {
    return "ULONG";
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

  if constexpr(std::is_same_v<T, ChimeraTK::Void>) {
    return "VOID";
  }

  if constexpr(std::is_same_v<T, std::string>) {
    return "STRING";
  }
}

/**********************************************************************************************************************/

template<typename T>
inline void TangoTestFixtureImpl::write(std::string attribute, T value) {
  Tango::DeviceAttribute writeValue(attribute, value);
  try {
    proxy->write_attribute(writeValue);
  }
  catch(CORBA::Exception& e) {
    Tango::Except::print_exception(e);
    std::rethrow_exception(std::current_exception());
  }
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

struct NullConfiguration {
  static void apply(TangoTestFixtureImpl&) {}
};

/**********************************************************************************************************************/

/**
 * @brief Test fixture to use in all TangoAdapter tests
 *
 * The test fixture will create a reference application and a client. It will wait for the server to
 * start and be in state "ON".
 *
 * The fixture itself is available through TangoTestFixture::f, the Tango::DeviceProxy through f->proxy
 */
template<typename Configuration = NullConfiguration>
struct TangoTestFixture : TangoTestFixtureImpl {
  TangoTestFixture() : TangoTestFixtureImpl() {
    Configuration::apply(static_cast<TangoTestFixtureImpl&>(*this));
    startup();
  }

  ~TangoTestFixture() override = default;
};

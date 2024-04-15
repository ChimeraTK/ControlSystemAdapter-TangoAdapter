// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <tango/tango.h>

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

#include <boost/test/unit_test.hpp>

#include <thread>

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

  ThreadedTangoServer& overrideNames(const std::string newNames);

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

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/

struct TangoTestFixtureImpl {
  TangoTestFixtureImpl();
  virtual ~TangoTestFixtureImpl();

  void setManualLoopControl(bool manualControl);
  void startup();

  template<typename T>
  void write(const std::string& attribute, T value);

  bool manualLoopControl{false};
  ChimeraTK::ApplicationFactory<ReferenceTestApplication> theFactory;
  ThreadedTangoServer theServer;
  ReferenceTestApplication* theApp{nullptr};
  std::unique_ptr<Tango::DeviceProxy> proxy{nullptr};
  static TangoTestFixtureImpl* f;

  static std::tuple<TangoTestFixtureImpl&, ReferenceTestApplication&, Tango::DeviceProxy&> getContents() {
    auto& fixture = *f;
    auto& app = *f->theApp;
    auto& proxy = *f->proxy;
    return {fixture, app, proxy};
  }
};

struct NullConfiguration {
  static void apply(TangoTestFixtureImpl&) {}
};

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

/*********************************************************************************************************************/

template<typename T>
inline void TangoTestFixtureImpl::write(const std::string& attribute, T value) {
  Tango::DeviceAttribute writeValue(attribute, value);
  try {
    proxy->write_attribute(writeValue);
  } catch (CORBA::Exception & e) {
    Tango::Except::print_exception(e);
    std::rethrow_exception(std::current_exception());
  }
}

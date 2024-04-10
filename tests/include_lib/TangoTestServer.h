// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

#include <thread>

#include <boost/test/unit_test.hpp>

#include <tango/tango.h>

struct ThreadedTangoServer {
  explicit ThreadedTangoServer(const std::string& testName, bool verbose = false);

  ~ThreadedTangoServer();

  /**
   * @brief Start the Tango server
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
  [[nodiscard]] const std::string& device() const { return deviceString; }

  std::vector<std::string> argv;
  std::thread tangoServerThread;
  std::string deviceString{};
  Tango::Util *tg{nullptr};
  static std::atomic<bool> shutdownRequested;
};

/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/

/**
 * @brief Test fixture to use in all TangoAdapter tests
 *
 * The test fixture will create a reference application and a client. It will wait for the server to
 * start and be in state "ON".
 *
 * The fixture itself is available through TangoTestFixture::f, the Tango::DeviceProxy through f->proxy
 */
struct TangoTestFixture {
  TangoTestFixture();
  ~TangoTestFixture();

  ChimeraTK::ApplicationFactory<ReferenceTestApplication> theFactory;
  ThreadedTangoServer theServer;
  std::unique_ptr<Tango::DeviceProxy> proxy{nullptr};
  static TangoTestFixture* f;
};


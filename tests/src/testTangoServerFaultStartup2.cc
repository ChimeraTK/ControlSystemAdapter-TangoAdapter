// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE serverTestFaultNoDmap

#include "TangoTestServer.h"

#include <boost/test/included/unit_test.hpp>

#include <filesystem>

#include <tango/tango.h>

BOOST_AUTO_TEST_CASE(testServerFaultNoDmap) {
  auto currentFolder = std::filesystem::current_path();

  ChimeraTK::ApplicationFactory<ReferenceTestApplication> theFactory;
  ThreadedTangoServer ts(boost::unit_test::framework::master_test_suite().p_name.value);
  ts.setOfflineDatabase("testServerFaultNoDmap");

  ts.start();

  // The database contains a broken attribute filter. The server should come up in fault
  auto url = ts.getClientUrl();
  auto proxy = Tango::DeviceProxy(url);
  BOOST_CHECK(proxy.is_connected());
  BOOST_CHECK(proxy.state() == Tango::FAULT);

  // Set a proper attribute filter, then call Init
  ts.setProperty("WorkingFolder", currentFolder);

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  proxy.command_inout("Init");

  using clock = std::chrono::steady_clock;
  auto start = clock::now();
  while(proxy.state() != Tango::ON) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() > 5) {
      std::cerr << "Timeout while waiting for Tango server to go to state \"ON\"" << std::endl;
      break;
    }
  }

  BOOST_TEST(proxy.state() == Tango::ON);
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoAdapter.h"

// Check if crash reporting is used.
#if defined(ENABLE_CRASH_REPORT)
#  include <crashreporting/crash_report.h>
#else
#  define DECLARE_CRASH_HANDLER
#  define INSTALL_CRASH_HANDLER
#endif

DECLARE_CRASH_HANDLER

int main(int argc, char* argv[]) {
  INSTALL_CRASH_HANDLER

  TangoAdapter::TangoAdapter::getInstance().run(argc, argv);
  Tango::Util::instance()->server_cleanup();
  return (0);
}

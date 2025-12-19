// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <tango/tango.h>

// For compatibility with Tango < 9.3.7 (we only require 9.3)

#ifndef TANGO_MAKE_VERSION
#  define TANGO_MAKE_VERSION(a, b, c) ((a) * 10000 + (b) * 100 + (c))
#endif

#ifndef TANGO_VERSION
#  define TANGO_VERSION TANGO_MAKE_VERSION(TANGO_VERSION_MAJOR, TANGO_VERSION_MINOR, TANGO_VERSION_PATCH)
#endif

#if TANGO_VERSION >= TANGO_MAKE_VERSION(10, 3, 0)
namespace TangoAdapter {
  Tango::DServer* constructor(Tango::DeviceClass* cl_ptr, const std::string& name, const std::string& desc,
      Tango::DevState state, const std::string& status);
} // namespace TangoAdapter
#endif

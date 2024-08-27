// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

// Support Tango's new logging macros for 9.4 to 9.3 facilities
// Mapping is copied from logging.h
// This is required naming by Tango, so disable the linter
// NOLINTBEGIN(readability-identifier-naming)
#ifndef TANGO_LOG_DEBUG
#  define TANGO_LOG_DEBUG cout3
#  define TANGO_LOG_INFO  cout1
#endif
// NOLINTEND(readability-identifier-naming)

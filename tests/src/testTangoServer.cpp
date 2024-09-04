// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

static ChimeraTK::ApplicationFactory<ReferenceTestApplication> theFactory{"testVariableExistence"};

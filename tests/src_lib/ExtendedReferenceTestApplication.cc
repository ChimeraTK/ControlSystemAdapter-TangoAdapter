// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ExtendedReferenceTestApplication.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>


void ExtendedReferenceTestApplication::initialise() {
  for (const auto &[direction, name, size]: ExtendedReferenceTestApplication::additionalVariables) {
    additionalProcessArrays.emplace_back(_processVariableManager->createProcessArray<int>(direction, name, size));
  }
  ReferenceTestApplication::initialise();
}

std::list<ExtendedReferenceTestApplication::VariableHolder> ExtendedReferenceTestApplication::additionalVariables;

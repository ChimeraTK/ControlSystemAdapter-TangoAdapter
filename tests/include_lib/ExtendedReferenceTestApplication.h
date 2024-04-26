// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/ControlSystemAdapter/SynchronizationDirection.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

/**
 * @brief Extended test application for tests
 *
 * Extends the ReferenceTestApplication so that it is possible to register additional
 * variables for extended name mapping (regression) tests.
 */
class ExtendedReferenceTestApplication: public ReferenceTestApplication {
 public:
  using ReferenceTestApplication::ReferenceTestApplication;

  using VariableHolder = std::tuple<ChimeraTK::SynchronizationDirection, std::string, size_t>;

  void initialise() override;

  static std::list<VariableHolder> additionalVariables;
  std::list<ChimeraTK::ProcessArray<int>::SharedPtr> additionalProcessArrays;
};

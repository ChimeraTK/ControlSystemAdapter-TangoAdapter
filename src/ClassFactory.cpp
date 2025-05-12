// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AdapterDeviceClass.h"
#include "AttributeMapper.h"
#include "TangoAdapter.h"

#include <tango/tango.h>

// Factory function required from Tango.
void Tango::DServer::class_factory() {
  auto& mapper = TangoAdapter::TangoAdapter::getInstance().getMapper();
  for(auto& className : mapper.getClasses()) {
    add_class(std::make_unique<TangoAdapter::AdapterDeviceClass>(className).release());
  }
}

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AttributeMapper.h"
#include "AttributeProperty.h"
#include "ChimeraTK/TransferElementAbstractor.h"

#include <tango/tango.h>

#include <AttributeProperty.h>

// Most of the non-conforming naming is a requierment from Tango, so just disable this check
// NOLINTBEGIN(readability-identifier-naming)

/**
 *  AdapterDeviceImpl class description:
 *    Test of TangoAdapterfor ChimeraTK
 */

namespace TangoAdapter {
  class AdapterDeviceImpl : public TANGO_BASE_CLASS {
   public:
    AdapterDeviceImpl(Tango::DeviceClass* cl, std::string& s);
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s);
    AdapterDeviceImpl(Tango::DeviceClass* cl, const char* s, const char* d);
    ~AdapterDeviceImpl() override { delete_device(); }

    void delete_device() override;
    void init_device() override;
    void get_device_property();
    void always_executed_hook() override;

    void attachToClassAttributes(const std::shared_ptr<AttributeMapper::DeviceClass>& deviceClass);
    ChimeraTK::TransferElementAbstractor getPvForAttribute(const std::string& attributeName);

    // Lint: Disabling because this is Tango code
    // NOLINTNEXTLINE(google-runtime-int)
    void read_attr_hardware(std::vector<long>& attr_list) override;
    void add_dynamic_attributes();
    void add_dynamic_commands();

   private:
    std::map<std::string, ChimeraTK::TransferElementAbstractor> _attributeToPvMap;
    void restoreMemoriedSpectra(const std::list<AttributeProperty>&);
  };
} // namespace TangoAdapter
  // NOLINTEND(readability-identifier-naming)

// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"
#include "AttributeImpl.h"
#include "AttributeProperty.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace TangoAdapter {
  template<typename TangoType, typename AdapterType>
  class ScalarAttributeProxy : public Tango::Attr {
   public:
    explicit ScalarAttributeProxy(AttributeProperty& attProperty);
    ~ScalarAttributeProxy() override = default;

    void read(Tango::DeviceImpl* dev, Tango::Attribute& att) override;
    void write(Tango::DeviceImpl* dev, Tango::WAttribute& att) override;
    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    }

   private:
    using GenericAccessor = ChimeraTK::NDRegisterAccessor<AdapterType>;
    using BooleanAccessor = ChimeraTK::NDRegisterAccessor<ChimeraTK::Boolean>;
    using ImplType = AttributeImpl<ScalarAttributeProxy<TangoType, AdapterType>, TangoType, AdapterType>;
  };

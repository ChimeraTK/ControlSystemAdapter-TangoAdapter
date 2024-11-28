// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"
#include "AttributeProperty.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace TangoAdapter {
  template<typename TangoType, typename AdapterType>
  class ScalarAttribTempl : public Tango::Attr {
   public:
    explicit ScalarAttribTempl(AttributeProperty& attProperty);
    ~ScalarAttribTempl() override = default;

    void read(Tango::DeviceImpl* dev, Tango::Attribute& att) override;
    void write(Tango::DeviceImpl* dev, Tango::WAttribute& att) override;
    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    }

   private:
    using GenericAccessor = ChimeraTK::NDRegisterAccessor<AdapterType>;
    using BooleanAccessor = ChimeraTK::NDRegisterAccessor<ChimeraTK::Boolean>;
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  ScalarAttribTempl<TangoType, AdapterType>::ScalarAttribTempl(AttributeProperty& attProperty)
  : Tango::Attr(attProperty.name.c_str(), attProperty.getDataType(), attProperty.writeType) {
    // memory the written value and write at initialization
    if(attProperty.writeType == Tango::READ_WRITE || attProperty.writeType == Tango::WRITE) {
      set_memorized();
      set_memorized_init(true);
    }

    Tango::UserDefaultAttrProp att_prop;

    // The c_str() are fine, tango internally creates a std::string of them.
    att_prop.set_label(attProperty.name.c_str());

    att_prop.set_description(attProperty.desc.c_str());

    if(!(attProperty.unit.empty())) {
      att_prop.set_unit(attProperty.unit.c_str());
    }

    switch(attProperty.notificationType) {
      case AttributeProperty::NotificationType::change:
        set_change_event(true, true);
        break;
      case AttributeProperty::NotificationType::data_ready:
        set_data_ready_event(true);
        break;
      default:
        break;
    }

    // Since Tango does not support int8 natively and we have to resort to SHORT,
    // limit the values to the int8 limits
    if constexpr(std::is_same_v<AdapterType, int8_t>) {
      att_prop.set_min_value(std::to_string(std::numeric_limits<int8_t>::min()).c_str());
      att_prop.set_max_value(std::to_string(std::numeric_limits<int8_t>::max()).c_str());
    }
    set_default_properties(att_prop);
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  void ScalarAttribTempl<TangoType, AdapterType>::read(Tango::DeviceImpl* dev, Tango::Attribute& att) {
    auto* adapterDevice = dynamic_cast<TangoAdapter::AdapterDeviceImpl*>(dev);
    assert(adapterDevice != nullptr);

    auto pv = adapterDevice->getPvForAttribute(att.get_name());
    assert(pv.getValueType() == typeid(AdapterType));

    auto processScalar = boost::reinterpret_pointer_cast<GenericAccessor>(pv.getHighLevelImplElement());

    if(!processScalar) {
      DEV_WARN_STREAM(dev) << "pv type mismatch (expected: " << boost::core::demangle(pv.getValueType().name())
                           << ", got :" << boost::core::demangle(typeid(AdapterType).name()) << ")" << std::endl;
      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
      return;
    }

    if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
      auto value = std::make_unique<Tango::DevString>();
      *(value.get()) = Tango::string_dup(processScalar->accessData(0).c_str());
      att.set_value(value.release(), 1, 0, true);
    }
    else if constexpr(std::is_same_v<TangoType, Tango::DevBoolean>) {
      auto pvAsBool = boost::reinterpret_pointer_cast<BooleanAccessor>(processScalar);
      assert(pvAsBool != nullptr);

      auto value = std::make_unique<Tango::DevBoolean>();
      if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
        *(value.get()) = true;
      }
      else {
        *(value.get()) = pvAsBool->accessData(0);
      }
      att.set_value(value.release(), 1, 0, true);
    }
    else {
      // Lint: Not even sure why this triggers here.
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto value = std::make_unique<TangoType>(processScalar->accessData(0));
      // Lint: for the signed char adapter type we need to assign it to short, since Tango does not
      // have a signed char type. Silence the warning, we are not dealing with characters here
      // NOLINTNEXTLINE(bugprone-signed-char-misue)
      att.set_value(value.release(), 1, 0, true);
    }

    if(processScalar->dataValidity() != ChimeraTK::DataValidity::ok) {
      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
    }
    else {
      att.set_quality(Tango::AttrQuality::ATTR_VALID);
    }
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  void ScalarAttribTempl<TangoType, AdapterType>::write(Tango::DeviceImpl* dev, Tango::WAttribute& att) {
    auto* adapterDevice = dynamic_cast<TangoAdapter::AdapterDeviceImpl*>(dev);
    assert(adapterDevice != nullptr);

    auto pv = adapterDevice->getPvForAttribute(att.get_name());
    assert(pv.getValueType() == typeid(AdapterType));

    auto processScalar = boost::reinterpret_pointer_cast<GenericAccessor>(pv.getHighLevelImplElement());

    if(!processScalar) {
      DEV_WARN_STREAM(dev) << "pv type mismatch (expected: " << boost::core::demangle(pv.getValueType().name())
                           << ", got :" << boost::core::demangle(typeid(AdapterType).name()) << ")" << std::endl;
      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
      return;
    }

    if constexpr(std::is_same_v<AdapterType, std::string>) {
      Tango::DevString st_value;
      att.get_write_value(st_value);
      processScalar->accessData(0) = std::string(st_value);
    }
    else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Boolean>) {
      auto pvAsBool = boost::reinterpret_pointer_cast<BooleanAccessor>(processScalar);
      Tango::DevBoolean b_value;
      att.get_write_value(b_value);
      pvAsBool->accessData(0) = b_value;
    }
    else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
      // do nothing with the data, just write the accessor
    }
    else {
      TangoType value;
      att.get_write_value(value);
      processScalar->accessData(0) = value;
    }

    if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID) {
      processScalar->setDataValidity(ChimeraTK::DataValidity::faulty);
    }
    else {
      processScalar->setDataValidity(ChimeraTK::DataValidity::ok);
    }
    processScalar->write();
  }

} // namespace TangoAdapter

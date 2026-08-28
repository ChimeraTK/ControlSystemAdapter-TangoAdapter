// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"
#include "AttributeProperty.h"
#include "ChangeEventSource.h"

#include <ChimeraTK/NDRegisterAccessor.h>

#include <mutex>
#include <vector>

namespace TangoAdapter {
  template<typename TangoType, typename AdapterType>
  class ScalarAttribTempl : public Tango::Attr, public ChangeEventSource {
   public:
    explicit ScalarAttribTempl(AttributeProperty& attProperty);
    ~ScalarAttribTempl() override = default;

    void read(Tango::DeviceImpl* dev, Tango::Attribute& att) override;
    void write(Tango::DeviceImpl* dev, Tango::WAttribute& att) override;
    void updateFromPv(ChimeraTK::TransferElementAbstractor pv) override;
    void pushChangeEvent(Tango::DeviceImpl* dev) override;
    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    }

   private:
    using GenericAccessor = ChimeraTK::NDRegisterAccessor<AdapterType>;

    /// Produce a Tango-owned buffer from the cached value (adapter domain). Locks the attribute mutex.
    std::unique_ptr<TangoType> makeTangoValue(Tango::AttrQuality& quality);

    // Per-attribute cache. Protected by _mutex. The only thread that touches the live ChimeraTK accessor
    // data is the updater thread (via updateFromPv); read()/pushChangeEvent()/write() serve from here.
    mutable std::mutex _mutex;
    std::vector<AdapterType> _cachedValue{1};
    Tango::AttrQuality _cachedQuality{Tango::AttrQuality::ATTR_INVALID};
  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  ScalarAttribTempl<TangoType, AdapterType>::ScalarAttribTempl(AttributeProperty& attProperty)
  : Tango::Attr(attProperty.description.name.c_str(), attProperty.getDataType(), attProperty.writeType) {
    // memory the written value and write at initialization
    if(attProperty.writeType == Tango::READ_WRITE || attProperty.writeType == Tango::WRITE) {
      set_memorized();
      set_memorized_init(true);
    }

    Tango::UserDefaultAttrProp att_prop;

    // The c_str() are fine, tango internally creates a std::string of them.
    att_prop.set_label(attProperty.description.name.c_str());

    att_prop.set_description(attProperty.description.description.value().c_str());

    if(attProperty.description.unit) {
      att_prop.set_unit(attProperty.description.unit.value().c_str());
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
  void ScalarAttribTempl<TangoType, AdapterType>::updateFromPv(ChimeraTK::TransferElementAbstractor pv) {
    auto processScalar = boost::reinterpret_pointer_cast<GenericAccessor>(pv.getHighLevelImplElement());

    std::lock_guard<std::mutex> lock(_mutex);
    if(!processScalar) {
      _cachedQuality = Tango::AttrQuality::ATTR_INVALID;
      return;
    }

    if constexpr(!std::is_same_v<AdapterType, ChimeraTK::Void>) {
      _cachedValue[0] = processScalar->accessData(0);
    }
    _cachedQuality = processScalar->dataValidity() == ChimeraTK::DataValidity::ok ? Tango::AttrQuality::ATTR_VALID :
                                                                                    Tango::AttrQuality::ATTR_INVALID;
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  std::unique_ptr<TangoType> ScalarAttribTempl<TangoType, AdapterType>::makeTangoValue(Tango::AttrQuality& quality) {
    std::lock_guard<std::mutex> lock(_mutex);
    quality = _cachedQuality;

    auto value = std::make_unique<TangoType>();
    if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
      *(value.get()) = Tango::string_dup(_cachedValue[0].c_str());
    }
    else if constexpr(std::is_same_v<TangoType, Tango::DevBoolean>) {
      if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
        *(value.get()) = true;
      }
      else {
        *(value.get()) = static_cast<Tango::DevBoolean>(_cachedValue[0]);
      }
    }
    else {
      *(value.get()) = static_cast<TangoType>(_cachedValue[0]);
    }
    return value;
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  void ScalarAttribTempl<TangoType, AdapterType>::read([[maybe_unused]] Tango::DeviceImpl* dev, Tango::Attribute& att) {
    Tango::AttrQuality quality;
    auto value = makeTangoValue(quality);

    att.set_value(value.release(), 1, 0, true);
    att.set_quality(quality);
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType>
  void ScalarAttribTempl<TangoType, AdapterType>::pushChangeEvent(Tango::DeviceImpl* dev) {
    Tango::AttrQuality quality;
    auto value = makeTangoValue(quality);
    // release=true -> the buffer is handed over to Tango which frees it after serialization
    dev->push_change_event(get_name(), value.release(), 1, 0, true);
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

    // Keep the read cache coherent with the value just written, without reading back the accessor (which the
    // updater thread may be accessing concurrently for read-back PVs).
    AdapterType writtenValue{};
    if constexpr(std::is_same_v<AdapterType, std::string>) {
      Tango::DevString st_value;
      att.get_write_value(st_value);
      processScalar->accessData(0) = std::string(st_value);
      writtenValue = std::string(st_value);
    }
    else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Boolean>) {
      Tango::DevBoolean b_value;
      att.get_write_value(b_value);
      processScalar->accessData(0) = static_cast<ChimeraTK::Boolean>(b_value);
      writtenValue = static_cast<ChimeraTK::Boolean>(b_value);
    }
    else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
      // do nothing with the data, just write the accessor
    }
    else {
      TangoType value;
      att.get_write_value(value);
      processScalar->accessData(0) = static_cast<AdapterType>(value);
      writtenValue = static_cast<AdapterType>(value);
    }

    if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID) {
      processScalar->setDataValidity(ChimeraTK::DataValidity::faulty);
    }
    else {
      processScalar->setDataValidity(ChimeraTK::DataValidity::ok);
    }
    processScalar->write();

    {
      std::lock_guard<std::mutex> lock(_mutex);
      if constexpr(!std::is_same_v<AdapterType, ChimeraTK::Void>) {
        _cachedValue[0] = writtenValue;
      }
      _cachedQuality = att.get_quality() == Tango::AttrQuality::ATTR_INVALID ? Tango::AttrQuality::ATTR_INVALID :
                                                                               Tango::AttrQuality::ATTR_VALID;
    }
  }

} // namespace TangoAdapter

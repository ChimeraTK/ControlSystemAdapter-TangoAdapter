// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AttributeProperty.h"
#include "AdapterDeviceImpl.h"
#include "TangoPropertyHelper.h"

#include <ChimeraTK/NDRegisterAccessor.h>


namespace ChimeraTK {
  template<typename TangoType, typename AdapterType , typename TangoWriteType = TangoType>
  class SpectrumAttribTempl : public Tango::SpectrumAttr {
   public:
    explicit SpectrumAttribTempl(AttributeProperty& attProperty);
    ~SpectrumAttribTempl() override = default;

    void read(Tango::DeviceImpl* dev, Tango::Attribute& att) override;
    void write(Tango::DeviceImpl* dev, Tango::WAttribute& att) override;
    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    }

    std::string memoriedPropertyName;
   private:
    using GenericAccessor = ChimeraTK::NDRegisterAccessor<AdapterType>;
    using BooleanAccessor = ChimeraTK::NDRegisterAccessor<ChimeraTK::Boolean>;

  };

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType, typename TangoWriteType>
  SpectrumAttribTempl<TangoType, AdapterType, TangoWriteType>::SpectrumAttribTempl(AttributeProperty& attProperty)
  : Tango::SpectrumAttr(attProperty.name.c_str(), ChimeraTK::deriveDataType(attProperty.dataType),
        attProperty.writeType, attProperty.length) {
    if(attProperty.writeType != Tango::READ) {
      memoriedPropertyName = "__Memoried_" + attProperty.name;
    }

    Tango::UserDefaultAttrProp axis_prop;
    axis_prop.set_label(attProperty.name.c_str());

    axis_prop.set_description(attProperty.desc.c_str());

    if(!(attProperty.unit.empty())) {
      axis_prop.set_unit(attProperty.unit.c_str());
    }

    // Since Tango does not support int8 natively and we have to resort to SHORT,
    // limit the values to the int8 limits
    if constexpr(std::is_same_v<AdapterType, int8_t>) {
      axis_prop.set_min_value(std::to_string(std::numeric_limits<int8_t>::min()).c_str());
      axis_prop.set_max_value(std::to_string(std::numeric_limits<int8_t>::max()).c_str());
    }

    set_default_properties(axis_prop);
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType, typename TangoWriteType>
  void SpectrumAttribTempl<TangoType, AdapterType, TangoWriteType>::read(
      Tango::DeviceImpl* dev, Tango::Attribute& att) {
    auto* adapterDevice = dynamic_cast<TangoAdapter::AdapterDeviceImpl*>(dev);
    assert(adapterDevice != nullptr);

    auto pv = adapterDevice->getPvForAttribute(att.get_name());
    assert(pv.getValueType() == typeid(AdapterType));

    auto processSpectrum = boost::reinterpret_pointer_cast<GenericAccessor>(pv.getHighLevelImplElement());

    if(!processSpectrum) {
      DEV_WARN_STREAM(dev) << "pv type mismatch (expected: " << boost::core::demangle(pv.getValueType().name())
                           << ", got :" << boost::core::demangle(typeid(AdapterType()).name()) << ")" << std::endl;

      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
      return;
    }

    auto length = processSpectrum->getNumberOfSamples();
    // NO, lint. Tango need a runtime-length C-Style array here, cannot use std::array.
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    auto data = std::make_unique<TangoType[]>(length);

    for(unsigned int i = 0; i < length; i++) {
      if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
        data.get()[i] = Tango::string_dup(processSpectrum->accessData(i).c_str());
      }
      else if constexpr(std::is_same_v<AdapterType, int8_t>) {
        // Lint: We need to use Short as transport for int8_t here as Tango does not support this type,
        // there are no chars involved.
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        data.get()[i] = Tango::DevShort(processSpectrum->accessData(i));
      }
      else {
        data.get()[i] = processSpectrum->accessData(i);
      }
    }
    att.set_value(data.release(), length, 0, true);

    if(processSpectrum->dataValidity() != ChimeraTK::DataValidity::ok) {
      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
    }
    else {
      att.set_quality(Tango::AttrQuality::ATTR_VALID);
    }
  }

  /********************************************************************************************************************/

  template<typename TangoType, typename AdapterType, typename TangoWriteType>
  void SpectrumAttribTempl<TangoType, AdapterType, TangoWriteType>::write(
      Tango::DeviceImpl* dev, Tango::WAttribute& att) {
    auto* adapterDevice = dynamic_cast<TangoAdapter::AdapterDeviceImpl*>(dev);
    assert(adapterDevice != nullptr);

    auto pv = adapterDevice->getPvForAttribute(att.get_name());
    assert(pv.getValueType() == typeid(AdapterType));

    auto processSpectrum = boost::reinterpret_pointer_cast<GenericAccessor>(pv.getHighLevelImplElement());

    if(!processSpectrum) {
      DEV_WARN_STREAM(dev) << "pv type mismatch (expected: " << boost::core::demangle(pv.getValueType().name())
                           << ", got :" << boost::core::demangle(typeid(AdapterType()).name()) << ")" << std::endl;

      att.set_quality(Tango::AttrQuality::ATTR_INVALID);
      return;
    }

    auto length = processSpectrum->getNumberOfSamples();

    auto& processVector = processSpectrum->accessChannel(0);

    auto arraySize = att.get_write_value_length();
    std::string memoried_value;

    if(arraySize != length) {
      std::stringstream msg;
      msg << "Written size " << arraySize << " does not match length " << length << "\n";
      DEV_ERROR_STREAM(dev) << "WRITE_ERROR " << msg.str() << std::endl;

      Tango::Except::throw_exception("WRITE_ERROR", msg.str(), "SpectrumAttribTempl::write()");
    }

    const TangoWriteType* data = nullptr;
    if(att.get_user_set_write_value()) {
      if constexpr(std::is_same_v<Tango::DevUChar, TangoType>) {
        data = att.get_last_written_uch()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevUShort, TangoType>) {
        data = att.get_last_written_ush()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevULong, TangoType>) {
        data = att.get_last_written_ulg()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevULong64, TangoType>) {
        data = att.get_last_written_ulg64()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevShort, TangoType>) {
        data = att.get_last_written_sh()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevLong, TangoType>) {
        data = att.get_last_written_lg()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevLong64, TangoType>) {
        data = att.get_last_written_lg64()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevFloat, TangoType>) {
        data = att.get_last_written_fl()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevDouble, TangoType>) {
        data = att.get_last_written_db()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevBoolean, TangoType>) {
        data = att.get_last_written_boo()->get_buffer();
      }
      else if constexpr(std::is_same_v<Tango::DevString, TangoType>) {
        data = att.get_last_written_str()->get_buffer();
      }
      else {
        DEV_ERROR_STREAM(dev) << "Unsupported tango type " << boost::core::demangle(typeid(TangoType).name())
                              << std::endl;
        assert(false);
      }
    }
    else {
      att.get_write_value(data);

      memoried_value = arrayToString(data, std::min<size_t>(arraySize, length));
    }

    if(data != nullptr) {
      if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
        std::transform(data, data + arraySize, processVector.begin(), [](auto v) { return std::string(v); });
      }
      else {
        std::copy(data, data + arraySize, processVector.begin());
      }
    }
    else {
      DEV_WARN_STREAM(dev) << "Did not get any written data from " << att.get_name() << std::endl;
    }

    if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID || data == nullptr) {
      processSpectrum->setDataValidity(DataValidity::faulty);
    }
    else {
      processSpectrum->setDataValidity(DataValidity::ok);
    }

    processSpectrum->write();
    if(att.get_user_set_write_value()) {
      // if user writing, set read value
      att.set_rvalue();
    }
    else {
      if(Tango::Util::_UseDb) {
        // memory the written value if connected to database
        setProperty<std::string>(dev, memoriedPropertyName, memoried_value);
      }
    }
  }

} // namespace ChimeraTK

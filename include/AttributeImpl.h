// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "AdapterDeviceImpl.h"
#include "AttributeProperty.h"

namespace TangoAdapter {
  template<typename Base, typename TangoType, typename AdapterType, typename TangoWriteType = TangoType>
  class AttributeImpl {
   public:
    using GenericAccessor = ChimeraTK::NDRegisterAccessor<AdapterType>;
    /******************************************************************************************************************/

    explicit AttributeImpl(AttributeProperty& prop) : _eventing(prop.description.attributeEventing) {}

    /******************************************************************************************************************/

    void updateMetaData(AttributeProperty& prop) {
      auto base = static_cast<Base const&>(*this);
      Tango::UserDefaultAttrProp axisProperties;
      axisProperties.set_label(prop.description.name.c_str());

      std::cout << "n: " << prop.description.name << std::endl;
      std::cout << "d: " << prop.description.description.value() << std::endl;

      axisProperties.set_description(prop.description.description.value().c_str());

      if(prop.description.unit) {
        std::cout << "u: " << prop.description.unit.value() << std::endl;
        axisProperties.set_unit(prop.description.unit.value().c_str());
      }

      // Since Tango does not support int8 natively and we have to resort to SHORT,
      // limit the values to the int8 limits
      if constexpr(std::is_same_v<AdapterType, int8_t>) {
        axisProperties.set_min_value(std::to_string(std::numeric_limits<int8_t>::min()).c_str());
        axisProperties.set_max_value(std::to_string(std::numeric_limits<int8_t>::max()).c_str());
      }

      base.set_default_properties(axisProperties);
      switch(prop.description.attributeEventing) {
        case TangoAdapter::AttributeEventing::DATA:
          base.set_change_event(true, false);
          break;
        case TangoAdapter::AttributeEventing::DATA_READY:
          base.set_data_ready_event(true);
          break;
        default:
          break;
      }
    }

    /******************************************************************************************************************/

    void sendEvent(AdapterDeviceImpl* device) {
      auto base = static_cast<Base const&>(*this);
      switch(_eventing) {
        case AttributeEventing::NONE:
          break;
        case AttributeEventing::DATA_READY:
          device->push_data_ready_event(base, _eventCtr++);
          break;
        case AttributeEventing::DATA: {
          // FIXME: Thread issue incoming
          auto [buffer, validity, version] = getReadBuffer(device);
          if(not buffer) {
            return;
          }

          Tango::TangoTimestamp t(version.getTime());
          device->push_change_event(buffer.release(), t,
              validity == ChimeraTK::DataValidity::ok ? Tango::AttrQuality::ATTR_VALID :
                                                        Tango::AttrQuality::ATTR_INVALID,
              base.get_name(), base.get_max_dim_x(), 0, true);
          break;
        }
      }
    }

    /******************************************************************************************************************/

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> getPvForAttribute(AdapterDeviceImpl* device) {
      auto base = static_cast<Base const&>(*this);

      auto pv = device->getPvForAttribute(base.get_name());
      assert(pv.getValueType() == typeid(AdapterType));

      return boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<AdapterType>>(pv.getHighLevelImplElement());
    }

    /******************************************************************************************************************/

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    std::tuple<std::unique_ptr<TangoType[]>, ChimeraTK::DataValidity, ChimeraTK::VersionNumber> getReadBuffer(
        AdapterDeviceImpl* device) {
      auto processVariable = getPvForAttribute(device);
      if(!processVariable) {
        DEV_WARN_STREAM(device) << "pv type mismatch (expected: "
                                << boost::core::demangle(processVariable->getValueType().name())
                                << ", got :" << boost::core::demangle(typeid(AdapterType()).name()) << ")" << std::endl;

        return {nullptr, ChimeraTK::DataValidity::faulty, ChimeraTK::VersionNumber()};
      }

      auto length = processVariable->getNumberOfSamples();
      // NO, lint. Tango need a runtime-length C-Style array here, cannot use std::array.
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      auto data = std::make_unique<TangoType[]>(length);

      for(unsigned int i = 0; i < length; i++) {
        if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
          data.get()[i] = Tango::string_dup(processVariable->accessData(i).c_str());
        }
        else if constexpr(std::is_same_v<AdapterType, int8_t>) {
          // Lint: We need to use Short as transport for int8_t here as Tango does not support this type,
          // there are no chars involved.
          // NOLINTNEXTLINE(bugprone-signed-char-misuse)
          data.get()[i] = Tango::DevShort(processVariable->accessData(i));
        }
        else if constexpr(std::is_same_v<AdapterType, ChimeraTK::Void>) {
          assert(length == 1);
          data.get()[i] = true;
        }
        else {
          data.get()[i] = processVariable->accessData(i);
        }
      }

      auto v = processVariable->dataValidity();
      return {std::move(data), v, processVariable->getVersionNumber()};
    }

    /******************************************************************************************************************/

    void readImpl(AdapterDeviceImpl* dev, Tango::Attribute& att) {
      auto* adapterDevice = dynamic_cast<TangoAdapter::AdapterDeviceImpl*>(dev);
      assert(adapterDevice != nullptr);
      auto [buffer, validity, version] = getReadBuffer(adapterDevice);

      if(buffer) {
        att.set_value(buffer.release(), att.get_max_dim_x(), 0, true);
      }

      att.set_date(Tango::TangoTimestamp(version.getTime()));

      if(validity != ChimeraTK::DataValidity::ok) {
        att.set_quality(Tango::AttrQuality::ATTR_INVALID);
      }
      else {
        att.set_quality(Tango::AttrQuality::ATTR_VALID);
      }
    }

    /******************************************************************************************************************/

   private:
    AttributeEventing _eventing;
    Tango::DevLong _eventCtr{0};
  };

} // namespace TangoAdapter

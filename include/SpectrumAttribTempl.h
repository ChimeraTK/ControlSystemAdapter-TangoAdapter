#pragma once
#include "AttributeProperty.h"
#include "TangoPropertyHelper.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
  template<typename TangoType, typename AdapterType , typename TangoWriteType = TangoType>
  class SpectrumAttribTempl : public Tango::SpectrumAttr, Tango::LogAdapter {
   public:
    SpectrumAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> pv,
        std::shared_ptr<AttributeProperty> attProperty)
    : Tango::SpectrumAttr(
          attProperty->name.c_str(), attProperty->dataType, attProperty->writeType, pv->getNumberOfSamples()),
      Tango::LogAdapter(tangoDevice), processSpectrum(pv), dataType(attProperty->dataType),
      length(pv->getNumberOfSamples()) {
      DEBUG_STREAM << " SpectrumAttribTempl::SpectrumAttribTempl  Name: " << attProperty->name.c_str() << " Type"
                   << attProperty->dataType << " _writeType: " << attProperty->writeType << " size:" << length
                   << std::endl;

      if(attProperty->writeType != Tango::READ) {
        memoriedPropertyName = "__Memoried_" + attProperty->name;
      }

      Tango::UserDefaultAttrProp axis_prop;
      axis_prop.set_label(attProperty->name.c_str());

      axis_prop.set_description(attProperty->desc.c_str());

      if(!(attProperty->unit.empty())) {
        axis_prop.set_unit(attProperty->unit.c_str());
      }

      // Since Tango does not support int8 natively and we have to resort to SHORT,
      // limit the values to the int8 limits
      if constexpr(std::is_same_v<AdapterType, int8_t>) {
        axis_prop.set_min_value(std::to_string(std::numeric_limits<int8_t>::min()).c_str());
        axis_prop.set_max_value(std::to_string(std::numeric_limits<int8_t>::max()).c_str());
      }

      set_default_properties(axis_prop);
    }

    ~SpectrumAttribTempl() override = default;

    void read([[maybe_unused]] Tango::DeviceImpl* dev, Tango::Attribute& att) override {
      DEBUG_STREAM << "SpectrumAttribTempl::read " << get_name() << std::endl;

      TangoType* data = new TangoType[length];

      for(unsigned int i = 0; i < length; i++) {
        if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
          data[i] = Tango::string_dup(processSpectrum->accessData(i).c_str());
        }
        else {
          data[i] = processSpectrum->accessData(i);
        }
      }
      att.set_value(data, length, 0, true);

      if(processSpectrum->dataValidity() != ChimeraTK::DataValidity::ok) {
        att.set_quality(Tango::AttrQuality::ATTR_INVALID);
        ERROR_STREAM << "SprectrumAttribTempl::read " << get_name() << " is not valid" << std::endl;
      }
      else {
        att.set_quality(Tango::AttrQuality::ATTR_VALID);
      }
    }

    void write(Tango::DeviceImpl* dev, Tango::WAttribute& att) override {
      DEBUG_STREAM << "SpectrumAttribTempl::write " << get_name() << std::endl;

      auto& processVector = processSpectrum->accessChannel(0);

      auto arraySize = att.get_write_value_length();
      std::string memoried_value;

      if(arraySize != length) {
        std::stringstream msg;
        msg << "Written size " << arraySize << " does not match length " << length << "\n";
        ERROR_STREAM << "WRITE_ERROR " << msg.str() << std::endl;

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
          std::cerr << "Unsupported tango type" << std::endl;
          assert(false);
        }
      }
      else {
        att.get_write_value(data);

        memoried_value = arrayToString(data, std::min<size_t>(arraySize, length));
      }

      if (data != nullptr) {
        if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
          std::transform(data, data + arraySize, processVector.begin(), [](auto v) { return std::string(v); });
        }
        else {
          std::copy(data, data + arraySize, processVector.begin());
        }
      } else {
        ERROR_STREAM << "Did not get any written data from " << att.get_name() <<std::endl;
      }

      if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID || data == nullptr) {
        processSpectrum->setDataValidity(DataValidity::faulty);
      }
      else {
        processSpectrum->setDataValidity(DataValidity::ok);
      }

      try {
        processSpectrum->write();
        if(att.get_user_set_write_value()) {
          // if user writing, set read value
          att.set_rvalue();
        }
        else {
          if (Tango::Util::_UseDb) {
            // memory the written value if connected to database
            setProperty<std::string>(dev, memoriedPropertyName, memoried_value);
          }
        }
      }
      catch(...) {
        ERROR_STREAM << " SpectrumAttribTempl::write cannot write to " << processSpectrum->getName() << std::endl;
        std::string msg = "Attribute " + get_name() + ": Cannot write to " + processSpectrum->getName();

        Tango::Except::throw_exception("ERROR", msg, "SpectrumAttribTempl::write()");
      }

      DEBUG_STREAM << "SpectrumAttribTempl::write END" << std::endl;
    }

    bool is_allowed([[maybe_unused]] Tango::DeviceImpl* dev, [[maybe_unused]] Tango::AttReqType ty) override {
      return true;
    }

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> processSpectrum;
    Tango::CmdArgType dataType;
    unsigned int length;
    std::vector<Tango::DevString> attrStringRead{};
    Tango::DevBoolean* attrBoolRead;
    std::string memoriedPropertyName;
  };

} // namespace ChimeraTK

#pragma once
#include "AttributProperty.h"
#include "TangoPropertyHelper.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
  template<typename TangoType, typename AdapterType>
  class SpectrumAttribTempl : public Tango::SpectrumAttr, Tango::LogAdapter {
   public:
    SpectrumAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<AdapterType>> pv,
        std::shared_ptr<AttributProperty> attProperty)
    : Tango::SpectrumAttr(
          attProperty->name.c_str(), attProperty->dataType, attProperty->writeType, pv->getNumberOfSamples()),
      processSpectrum(pv), dataType(attProperty->dataType), length(pv->getNumberOfSamples()),
      Tango::LogAdapter(tangoDevice) {
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

      if(arraySize > length) {
        std::stringstream msg;
        msg << "Array size cannot be greater than" << length << "\n";
        ERROR_STREAM << "WRITE_ERROR " << msg.str() << std::endl;

        Tango::Except::throw_exception("WRITE_ERROR", msg.str(), "SpectrumAttribTempl::write()");
      }

      const TangoType* data = nullptr;
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
        if constexpr(std::is_same_v<Tango::DevString, TangoType>) {
          // Need to cast to const Tango::ConstDevString* here because otherwise Tangos
          // internal type dispatch thinks it's an array of enums.
          auto* dataCasted = reinterpret_cast<const Tango::ConstDevString*>(data);
          att.get_write_value(dataCasted);
        }
        else {
          att.get_write_value(data);
        }
        memoried_value = arrayToString(data, length);
      }

      if constexpr(std::is_same_v<TangoType, Tango::DevString>) {
        std::transform(data, data + arraySize, processVector.begin(), [](auto v) { return std::string(v); });
      }
      else {
        std::copy(data, data + arraySize, processVector.begin());
      }

      if(att.get_quality() == Tango::AttrQuality::ATTR_INVALID) {
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
          // memory the written value
          memoried_value.pop_back(); // remove the last seperator
          setProperty<std::string>(dev, memoriedPropertyName, memoried_value);
        }
      }
      catch(...) {
        ERROR_STREAM << " SpectrumAttribTempl::write cannot write to " << processSpectrum->getName() << std::endl;
        std::string msg = "Attribut " + get_name() + ": Cannot write to " + processSpectrum->getName();

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

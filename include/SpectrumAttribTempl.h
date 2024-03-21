#pragma once
#include "AttributProperty.h"
#include "TangoPropertyHelper.h"

#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
  template<typename T>
  class SpectrumAttribTempl : public Tango::SpectrumAttr, Tango::LogAdapter {
   public:
    SpectrumAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv,
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

      if constexpr(std::is_same<T, std::string>::value) {
        attrStringRead.reserve(length);
      }
      else if(dataType == Tango::DEV_BOOLEAN) {
        // FIXME: Find a nicer way than plain pointers
        attrBoolRead = new Tango::DevBoolean[length];
      }
      set_default_properties(axis_prop);
    }

    ~SpectrumAttribTempl() override { delete[] attrBoolRead; };

    void read([[maybe_unused]] Tango::DeviceImpl* dev, Tango::Attribute& att) override {
      DEBUG_STREAM << "SpectrumAttribTempl::read " << get_name() << std::endl;

      if constexpr(std::is_same<T, std::string>::value) {
        for(unsigned int i = 0; i < length; i++) {
          // char * is required by the Tango API. The data is copied internally
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
          attrStringRead[i] = const_cast<char*>(processSpectrum->accessData(i).c_str());
        }
        att.set_value(attrStringRead.data(), length);
      }
      else if(dataType == Tango::DEV_BOOLEAN) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<Boolean>> pv =
            boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processSpectrum);
        for(unsigned int i = 0; i < length; i++) {
          attrBoolRead[i] = processSpectrum->accessData(i);
        }

        att.set_value(attrBoolRead, length);
      }
      else {
        att.set_value(processSpectrum->accessChannel(0).data(), length);
      }
      if(processSpectrum->dataValidity() != ChimeraTK::DataValidity::ok) {
        ERROR_STREAM << "SpectrumAttribTempl::read " << get_name() << " is not valid" << std::endl;
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

      switch(dataType) {
        case Tango::DEV_UCHAR: {
          const uint8_t* c_value;
          if(att.get_user_set_write_value()) {
            c_value = att.get_last_written_uch()->get_buffer();
          }
          else {
            att.get_write_value(c_value);
            memoried_value = arrayToString(c_value, length);
          }

          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = c_value[i];
          }
          break;
        }
        case Tango::DEV_USHORT: {
          const Tango::DevUShort* us_value;
          if(att.get_user_set_write_value()) {
            us_value = att.get_last_written_ush()->get_buffer();
          }
          else {
            att.get_write_value(us_value);
            memoried_value = arrayToString(us_value, length);
          }

          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = us_value[i];
          }
          break;
        }
        case Tango::DEV_ULONG: {
          const uint32_t* ulg_value;
          if(att.get_user_set_write_value()) {
            ulg_value = att.get_last_written_ulg()->get_buffer();
          }
          else {
            att.get_write_value(ulg_value);
            memoried_value = arrayToString(ulg_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = ulg_value[i];
          }
          break;
        }

        case Tango::DEV_ULONG64: {
          const uint64_t* ulg64_value;
          if(att.get_user_set_write_value()) {
            ulg64_value = att.get_last_written_ulg64()->get_buffer();
          }
          else {
            att.get_write_value(ulg64_value);
            memoried_value = arrayToString(ulg64_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = ulg64_value[i];
          }
          break;
        }
        case Tango::DEV_SHORT: {
          const int16_t* sh_value;
          if(att.get_user_set_write_value()) {
            sh_value = att.get_last_written_sh()->get_buffer();
          }
          else {
            att.get_write_value(sh_value);
            memoried_value = arrayToString(sh_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = sh_value[i];
          }
          break;
        }
        case Tango::DEV_LONG: {
          const int32_t* lg_value;
          if(att.get_user_set_write_value()) {
            lg_value = att.get_last_written_lg()->get_buffer();
          }
          else {
            att.get_write_value(lg_value);
            memoried_value = arrayToString(lg_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = lg_value[i];
          }

          break;
        }
        case Tango::DEV_LONG64: {
          const int64_t* lg64_value;
          if(att.get_user_set_write_value()) {
            lg64_value = att.get_last_written_lg64()->get_buffer();
          }
          else {
            att.get_write_value(lg64_value);
            memoried_value = arrayToString(lg64_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = lg64_value[i];
          }
          break;
        }
        case Tango::DEV_FLOAT: {
          const float* fl_value;
          if(att.get_user_set_write_value()) {
            fl_value = att.get_last_written_fl()->get_buffer();
          }
          else {
            att.get_write_value(fl_value);
            memoried_value = arrayToString(fl_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = fl_value[i];
          }
          break;
        }

        case Tango::DEV_DOUBLE: {
          const double* db_value;
          if(att.get_user_set_write_value()) {
            db_value = att.get_last_written_db()->get_buffer();
          }
          else {
            att.get_write_value(db_value);
            memoried_value = arrayToString(db_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = db_value[i];
          }
          break;
        }
        case Tango::DEV_BOOLEAN: {
          const Tango::DevBoolean* bool_value;
          if(att.get_user_set_write_value()) {
            bool_value = att.get_last_written_boo()->get_buffer();
          }
          else {
            att.get_write_value(bool_value);
            memoried_value = arrayToString(bool_value, length);
          }
          for(size_t i = 0; i < arraySize; ++i) {
            processVector[i] = bool_value[i];
          }
          break;
        }
        case Tango::DEV_STRING: {
          if constexpr(std::is_same<T, std::string>::value) {
            const Tango::ConstDevString* str_value;
            if(att.get_user_set_write_value()) {
              str_value = att.get_last_written_str()->get_buffer();
            }
            else {
              att.get_write_value(str_value);
              memoried_value = arrayToString(str_value, length);
            }

            for(size_t i = 0; i < arraySize; ++i) {
              processVector[i] = std::string(str_value[i]);
            }
          }
          break;
        }
        default:
          ERROR_STREAM << " SpectrumAttribTempl::write " << get_name() << " unsupported dataTye " << dataType
                       << std::endl;
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

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> processSpectrum;
    Tango::CmdArgType dataType;
    unsigned int length;
    std::vector<Tango::DevString> attrStringRead{};
    Tango::DevBoolean* attrBoolRead;
    std::string memoriedPropertyName;
  };

} // namespace ChimeraTK

#pragma once
#include "AttributProperty.h"
#include "TangoPropertyHelper.h"
#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
template <typename T>
class SpectrumAttribTempl : public Tango::SpectrumAttr,Tango::LogAdapter
{
public:
	SpectrumAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty,
	    std::string description="description", std::string unit=""):
		Tango::SpectrumAttr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType,pv->getNumberOfSamples()),_processSpectrum(pv),
		_dataType(attProperty->_dataType), _length ( pv->getNumberOfSamples()), Tango::LogAdapter(tangoDevice)
	{
		DEBUG_STREAM<<" SpectrumAttribTempl::SpectrumAttribTempl  Name: "<<attProperty->_name.c_str()<<
		              " Type"<<attProperty->_dataType<<" _writeType: "<<attProperty->_writeType<<" size:"<<_length<<std::endl;

		if (attProperty->_writeType != Tango::READ)
			_memoried_property_name = "__Memoried_" + attProperty->_name;


		Tango::UserDefaultAttrProp axis_prop;
		axis_prop.set_label(attProperty->_name.c_str());

		axis_prop.set_description(attProperty->_desc .c_str());

		if (!(attProperty->_unit.empty()))
		    	axis_prop.set_unit(attProperty->_unit.c_str());

		if constexpr (std::is_same<T,std::string>::value) {
	    	attr_String_read = new Tango::DevString[_length];
	    }
	    else if (_dataType == Tango::DEV_BOOLEAN){
	    	attr_Bool_read = new Tango::DevBoolean[_length];
	    }
		set_default_properties(axis_prop);
	}

	virtual ~SpectrumAttribTempl(void)
	{
		if (attr_Bool_read != nullptr) delete attr_Bool_read;
		if (attr_String_read != nullptr) delete attr_String_read;

	};

	virtual void read(Tango::DeviceImpl *dev,
			  Tango::Attribute &att)
	{
		DEBUG_STREAM<<"SpectrumAttribTempl::read "<< get_name()<<std::endl;

		if constexpr (std::is_same<T,std::string>::value) {

			for (unsigned int i = 0;i < _length;i++) {
	    		attr_String_read[i] = const_cast<char *>(_processSpectrum->accessData(i).c_str());
	    		att.set_value(attr_String_read,_length);
			}
		}
		else if (_dataType==Tango::DEV_BOOLEAN)
		{
			boost::shared_ptr<ChimeraTK::NDRegisterAccessor<Boolean>> pv =boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(_processSpectrum);
			for (unsigned int i = 0;i < _length;i++) {
				attr_Bool_read[i] = _processSpectrum->accessData(i);
			}

			att.set_value(attr_Bool_read,_length);

		}
		else {
			att.set_value(_processSpectrum->accessChannel(0).data(),_length);
		}
		if(_processSpectrum->dataValidity() != ChimeraTK::DataValidity::ok)	{
			ERROR_STREAM<< "SpectrumAttribTempl::read "<< get_name() <<" is not valid"<<std::endl;
		}

	}

	virtual void write(Tango::DeviceImpl *dev, Tango::WAttribute &att)
	{
		DEBUG_STREAM<< "SpectrumAttribTempl::write "<< get_name()<<std::endl;

		auto& processVector = _processSpectrum->accessChannel(0);

		long arraySize = att.get_write_value_length();
		std::string memoried_value;

		if ( arraySize > _length )	{
			std::stringstream msg;
			msg<< "Array size cannot be greater than"<< _length<<"\n";
			ERROR_STREAM<<"WRITE_ERROR "<<msg.str()<<std::endl;

			Tango::Except::throw_exception("WRITE_ERROR", msg.str(), "SpectrumAttribTempl::write()");
		}

      switch (_dataType) {
        case Tango::DEV_UCHAR: {
          const uint8_t *c_value;
          if (att.get_user_set_write_value()) {
            c_value = att.get_last_written_uch()->get_buffer();
          }
          else {
            att.get_write_value(c_value);
            memoried_value = array_to_string(c_value,_length);
          }

          for (size_t i = 0; i < arraySize; ++i) processVector[i] = c_value[i];
          break;
        }
        case Tango::DEV_USHORT: {
          const Tango::DevUShort *us_value;
          if (att.get_user_set_write_value())	{
            us_value = att.get_last_written_ush()->get_buffer();
          }
          else {
            att.get_write_value(us_value);
            memoried_value = array_to_string(us_value,_length);
          }

          for (size_t i = 0; i < arraySize; ++i) processVector[i] = us_value[i];
          break;
        }
        case Tango::DEV_ULONG: {
          const uint32_t *ulg_value;
          if (att.get_user_set_write_value())	{
            ulg_value = att.get_last_written_ulg()->get_buffer();
          }
          else {
            att.get_write_value(ulg_value);
            memoried_value = array_to_string(ulg_value,_length);
          }
          for (size_t i = 0; i < arraySize; ++i) processVector[i] = ulg_value[i];
          break;
        }

        case Tango::DEV_ULONG64: {
          const uint64_t *ulg64_value;
          if (att.get_user_set_write_value())	{
            ulg64_value = att.get_last_written_ulg64()->get_buffer();
          }
          else {
            att.get_write_value(ulg64_value);
            memoried_value = array_to_string(ulg64_value,_length);
          }
          for(size_t i = 0; i < arraySize; ++i) processVector[i] = ulg64_value[i];
          break;
        }
		case  Tango::DEV_SHORT: {
          const int16_t *sh_value;
          if (att.get_user_set_write_value())	{
            sh_value = att.get_last_written_sh()->get_buffer();
          }
          else {
            att.get_write_value(sh_value);
            memoried_value = array_to_string(sh_value,_length);
          }
          for (size_t i = 0; i < arraySize; ++i) processVector[i] = sh_value[i];
          break;

   		}
		case Tango::DEV_LONG: {
          const int32_t *lg_value;
          if (att.get_user_set_write_value())	{
            lg_value = att.get_last_written_lg()->get_buffer();
          }
          else {
            att.get_write_value(lg_value);
            memoried_value = array_to_string(lg_value,_length);
          }
          for (size_t i = 0; i < arraySize; ++i)  processVector[i] = lg_value[i];

          break;
		}
		case Tango::DEV_LONG64: {
          const int64_t *lg64_value;
          if (att.get_user_set_write_value())	{
            lg64_value = att.get_last_written_lg64()->get_buffer();
          }
          else {
            att.get_write_value(lg64_value);
            memoried_value = array_to_string(lg64_value,_length);
          }
          for (size_t i = 0; i < arraySize; ++i)   processVector[i] = lg64_value[i];
          break;
		}
		case Tango::DEV_FLOAT: {
          const float *fl_value;
          if (att.get_user_set_write_value())	{
            fl_value = att.get_last_written_fl()->get_buffer();
          }
          else {
            att.get_write_value(fl_value);
            memoried_value = array_to_string(fl_value,_length);
          }
          for(size_t i = 0; i < arraySize; ++i)  processVector[i] = fl_value[i];
          break;
		}

		case Tango::DEV_DOUBLE: {
          const double *db_value;
          if (att.get_user_set_write_value())	{
            db_value = att.get_last_written_db()->get_buffer();
          }
          else {
            att.get_write_value(db_value);
            memoried_value = array_to_string(db_value,_length);
          }
          for(size_t i = 0; i < arraySize; ++i)  processVector[i] = db_value[i];
          break;
		}
		case Tango::DEV_BOOLEAN: {
          const Tango::DevBoolean	*bool_value;
          if (att.get_user_set_write_value())	{
            bool_value = att.get_last_written_boo()->get_buffer();
          }
          else {
            att.get_write_value(bool_value);
            memoried_value = array_to_string(bool_value,_length);
          }
          for (size_t i = 0; i < arraySize; ++i)  processVector[i] = bool_value[i];
          break;
        }
		case Tango::DEV_STRING: {
          if constexpr (std::is_same<T, std::string>::value) {
            const Tango::ConstDevString *str_value;
            if (att.get_user_set_write_value())	{
              str_value = att.get_last_written_str()->get_buffer();
            }
            else {
              att.get_write_value(str_value);
              memoried_value = array_to_string(str_value,_length);
            }

            for(size_t i = 0; i < arraySize; ++i)  processVector[i] = std::string(str_value[i]);
          }
          break;
        }
        default:
          ERROR_STREAM<<" SpectrumAttribTempl::write "<<get_name()<<" unsupported dataTye "<<_dataType<< std::endl;
        }

        try	{
          _processSpectrum->write();
          if  (att.get_user_set_write_value()) {
            //if user writing, set read value
            att.set_rvalue();
          }
          else {
            // memory the written value
            memoried_value.pop_back();//remove the last seperator
            set_property<std::string>(dev, _memoried_property_name,memoried_value);
          }
        }
        catch(...)
        {
          ERROR_STREAM<<" SpectrumAttribTempl::write cannot write to "<<_processSpectrum->getName()<< std::endl;
          std::string msg = "Attribut "+get_name() +": Cannot write to "+_processSpectrum->getName();

          Tango::Except::throw_exception("ERROR", msg, "SpectrumAttribTempl::write()");

        }

        DEBUG_STREAM<< "SpectrumAttribTempl::write END"<< std::endl;
    }


    virtual bool is_allowed(Tango::DeviceImpl *dev,	Tango::AttReqType ty){return true;}

    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> _processSpectrum;
    long _dataType;
    unsigned int _length;
    Tango::DevString *attr_String_read;
    Tango::DevBoolean *attr_Bool_read;
    std::string _memoried_property_name;

};

} // namespace ChimeraTK

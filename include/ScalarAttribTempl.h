#pragma once

#include "AttributProperty.h"

#include <ChimeraTK/NDRegisterAccessor.h>


namespace ChimeraTK {
template <typename T>
class ScalarAttribTempl : public Tango::Attr,Tango::LogAdapter
{
public:
	ScalarAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty):
		Tango::Attr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType),_processScalar(pv),
		_dataType(attProperty->_dataType), Tango::LogAdapter(tangoDevice)
	{

		//memory the written value and write at initialization
		if (attProperty->_writeType==Tango::READ_WRITE || attProperty->_writeType==Tango::WRITE )
		{
			set_memorized ();
			set_memorized_init(true);
		}

		Tango::UserDefaultAttrProp att_prop;
		att_prop.set_label(attProperty->_name.c_str());

		att_prop.set_description(attProperty->_desc .c_str());

		if (!(attProperty->_unit.empty()))
			att_prop.set_unit(attProperty->_unit.c_str());

		set_default_properties(att_prop);

		if constexpr (std::is_same<T,std::string>::value) {
			attr_String_read = new Tango::DevString[1];
		}

		if (_dataType==Tango::DEV_BOOLEAN){
			attr_Bool_read = new Tango::DevBoolean[1];
		}

	}

	virtual ~ScalarAttribTempl(void)
	{
		if (attr_Bool_read != nullptr) delete attr_Bool_read;
		if (attr_String_read != nullptr) delete attr_String_read;
	}

	virtual void read(Tango::DeviceImpl *dev,
			  Tango::Attribute &att)
	{

		DEBUG_STREAM<< "ScalarAttribTempl::read "<< get_name()<<std::endl;

		if constexpr (std::is_same<T,std::string>::value) {

			*attr_String_read = const_cast<char *>(_processScalar->accessData(0).c_str());
			att.set_value(attr_String_read);

		}
		else if (_dataType==Tango::DEV_BOOLEAN)
	  {
	    boost::shared_ptr<ChimeraTK::NDRegisterAccessor<Boolean>> pv =boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(_processScalar);

			*attr_Bool_read= _processScalar->accessData(0);
      att.set_value(attr_Bool_read);

	  }
	  else {
			att.set_value(&_processScalar->accessData(0));
		}

		if(_processScalar->dataValidity() != ChimeraTK::DataValidity::ok)
		{
			ERROR_STREAM<< "ScalarAttribTempl::read "<< get_name() <<" is not valid"<<std::endl;
		}

	}

	virtual void write(Tango::DeviceImpl *dev,
			   Tango::WAttribute &att)
	{
		DEBUG_STREAM<< "ScalarAttribTempl::write "<< get_name()<<std::endl;

		switch (_dataType)
		{
	    case Tango::DEV_UCHAR:
	    {
			uint8_t c_value;
			att.get_write_value(c_value);
			_processScalar->accessData(0) = c_value;
			break;
		}
		case Tango::DEV_USHORT:
		{
			uint16_t us_value;
			att.get_write_value(us_value);
			_processScalar->accessData(0) = us_value;
			break;
		}
	    case Tango::DEV_ULONG:
	    {
			uint32_t ul_value;
			att.get_write_value(ul_value);
			_processScalar->accessData(0) = ul_value;
			break;
		}
	    case Tango::DEV_ULONG64:
	    {
			uint64_t ul64_value;
			att.get_write_value(ul64_value);
			_processScalar->accessData(0) = ul64_value;
			break;
		}
	    case  Tango::DEV_SHORT:
		{
			int16_t s_value;
			att.get_write_value(s_value);
			_processScalar->accessData(0) = s_value;
			break;
		}
	    case Tango::DEV_LONG:
		{
			int32_t l_value;
			att.get_write_value(l_value);
			_processScalar->accessData(0) = l_value;
			break;
	    }
		case Tango::DEV_LONG64:
		{
			int64_t l64_value;
			att.get_write_value(l64_value);
			_processScalar->accessData(0) = l64_value;
			break;
		}
	    case Tango::DEV_FLOAT:
		{
			float f_value;
			att.get_write_value(f_value);
			_processScalar->accessData(0) = f_value;
			break;
		}
	    case Tango::DEV_DOUBLE:
		{
			double d_value;
			att.get_write_value(d_value);
			_processScalar->accessData(0) = d_value;
			break;
		}
	    case Tango::DEV_BOOLEAN:
	    {
			auto pv =boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(_processScalar);
			bool b_value;
			att.get_write_value(b_value);
			pv->accessData(0) = b_value;
			break;
		}
	    case Tango::DEV_STRING:
			if constexpr (std::is_same<T, std::string>::value) {
				Tango::DevString st_value;
				att.get_write_value(st_value);
				_processScalar->accessData(0) = std::string(st_value);
			}

			break;
		default:

			break;
		}
		 _processScalar->write();
	}

	virtual bool is_allowed(Tango::DeviceImpl *dev,
				Tango::AttReqType ty){return true;};


	boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> _processScalar;
	long _dataType;
  Tango::DevString  *attr_String_read ;
  Tango::DevBoolean *attr_Bool_read;

};

} // namespace ChimeraTK

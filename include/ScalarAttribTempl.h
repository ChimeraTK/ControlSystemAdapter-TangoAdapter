#ifndef _SCALAR_ATTRIB_H_
#define _SCALAR_ATTRIB_H_


#include "AttributProperty.h"
#include <boost/shared_ptr.hpp>
#include <ChimeraTK/ScalarRegisterAccessor.h>
#include <ChimeraTK/OneDRegisterAccessor.h>


namespace ChimeraTK {
template <typename T>	
class ScalarAttribTempl : public Tango::Attr
{
public:
	ScalarAttribTempl( boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty):
		Tango::Attr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType),_processScalar(pv),_dataType(attProperty->_dataType)
	{
	
		std::cout<<" ScalarAttribTempl :"<<attProperty->_name.c_str()<<" type: "<<attProperty->_dataType<<" _writeType: "<<attProperty->_writeType<<std::endl;
        if (attProperty->_writeType==Tango::READ_WRITE || attProperty->_writeType==Tango::WRITE )
        {
        	set_memorized ();
        	set_memorized_init(true);
        }
		Tango::UserDefaultAttrProp att_prop;
		att_prop.set_label(attProperty->_name.c_str());

		att_prop.set_description(attProperty->_desc .c_str());
	    att_prop.set_unit(attProperty->_unit.c_str());
	    
		set_default_properties(att_prop);
		if constexpr (is_same<T,std::string>::value) {
			attr_String_read = new Tango::DevString;
		}
		
	}

	virtual ~ScalarAttribTempl(void) {};

	virtual void read(Tango::DeviceImpl *dev,
			  Tango::Attribute &att)
	{	
	    try{	
        	_processScalar->readLatest();
    	}
    	catch (std::exception &e)
    	{
    		std::cout<<"Exception: "<<e.what()<<std::endl;
    	}
    	catch (...)
    	{
    		std::cout<<"unknown Exception"<<std::endl;
    	}
		
		//std::cout<<" Attr "<<_processScalar->getName()<<" "<<_processScalar->accessData(0)<<std::endl;
		
		if constexpr (is_same<T,std::string>::value) {

       		*attr_String_read = const_cast<char *>(_processScalar->accessData(0).c_str());
       		att.set_value(attr_String_read);
	    	//att.set_value(attr_String_read,1,0,true);
	    }
	    else {
			att.set_value(&_processScalar->accessData(0));
		}


	}

	virtual void write(Tango::DeviceImpl *dev,
			   Tango::WAttribute &att)
	{	
		std::cout<<" write start "<<std::endl;

	    switch (_dataType)
	    {
	    case Tango::DEV_UCHAR:			
			uint8_t c_value;
			att.get_write_value(c_value);
			_processScalar->accessData(0) = c_value;
			break;
		case Tango::DEV_USHORT:		
			
			uint16_t us_value;
			att.get_write_value(us_value);
			_processScalar->accessData(0) = us_value;
			break;
		
	    case Tango::DEV_ULONG:
			uint32_t ul_value;
			att.get_write_value(ul_value);
			_processScalar->accessData(0) = ul_value;
			break;
	    case Tango::DEV_ULONG64:
			uint64_t ul64_value;
			att.get_write_value(ul64_value);
			_processScalar->accessData(0) = ul64_value;
			break;
	    case  Tango::DEV_SHORT:
					
			int16_t s_value;
			att.get_write_value(s_value);
			_processScalar->accessData(0) = s_value;
			break;
	    case Tango::DEV_LONG:
			
			int32_t l_value;
			att.get_write_value(l_value);
			_processScalar->accessData(0) = l_value;
			break;
	    		
		case Tango::DEV_LONG64:
			
			int64_t l64_value;
			att.get_write_value(l64_value);
			_processScalar->accessData(0) = l64_value;
			break;
	    case Tango::DEV_FLOAT:
		
			float f_value;
			att.get_write_value(f_value);
			_processScalar->accessData(0) = f_value;
			break;
		
	    case Tango::DEV_DOUBLE:
		
			double d_value;
			att.get_write_value(d_value);
			//std::cout<<"  d_value= "<<d_value<<std::endl;	
			_processScalar->accessData(0) = d_value;
			break;
		
	    case Tango::DEV_BOOLEAN:
	    	{
		  //error: cannot bind non-const lvalue reference of type ‘bool&’ to an rvalue of type ‘bool’
			boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint8_t>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(_processScalar);
			uint8_t b_value;
			att.get_write_value(b_value);
			pv->accessData(0) = b_value;
			}
			break;
	    case Tango::DEV_STRING:
			if constexpr (is_same<T, std::string>::value) {
				Tango::DevString st_value;
				att.get_write_value(st_value);
				_processScalar->accessData(0) = std::string(st_value);				
			}	

			break;
		default:
			std::cout<<"   "<<std::endl;	
			break;
		}	
		 _processScalar->write();
	}

	virtual bool is_allowed(Tango::DeviceImpl *dev,
				Tango::AttReqType ty){return true;};

	boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> _processScalar;
	long _dataType; 
    Tango::DevString  *attr_String_read ;
protected:

};

} // namespace ChimeraTK

#endif // _SCALAR_ATTRIB_H_

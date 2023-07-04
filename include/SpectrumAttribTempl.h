#ifndef _SPECTRUM_ATTRIB_H_
#define _SPECTRUM_ATTRIB_H_


#include "AttributProperty.h"
#include <boost/shared_ptr.hpp>
#include <ChimeraTK/ScalarRegisterAccessor.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
 

namespace ChimeraTK {
template <typename T>	
class SpectrumAttribTempl : public Tango::SpectrumAttr
{
public:
	SpectrumAttribTempl( boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty
		/*const char *name, long data_type, Tango::AttrWriteType w_type*/, std::string description="description", std::string unit=""):
		Tango::SpectrumAttr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType,pv->getNumberOfSamples()),_processSpectrum(pv),_dataType(attProperty->_dataType)
	{
		//std::cout<<" SpectrumAttribTempl :"<<"type: "<<attProperty->_dataType<<" _writeType: "<<attProperty->_writeType<<std::endl;
		Tango::UserDefaultAttrProp axis_prop;
		axis_prop.set_label(attProperty->_name.c_str());
		//axis_prop.set_format("%9.5f");

		axis_prop.set_description(attProperty->_desc .c_str());
	    axis_prop.set_unit(attProperty->_unit.c_str());

	    length = _processSpectrum->getNumberOfSamples();

		if constexpr (is_same<T,std::string>::value) {	    
	    	attr_String_read = new Tango::DevString[length];
	    }
	    
		set_default_properties(axis_prop);
	}

	virtual ~SpectrumAttribTempl(void) {};

	virtual void read(Tango::DeviceImpl *dev,
			  Tango::Attribute &att)
	{
		//DEBUG_STREAM<<"SpectrumAttribTempl::read"<<endl;
	    try{	
        	_processSpectrum->readLatest();
    	}
    	catch (std::exception &e)
    	{
    		std::cout<<"Exeption: "<<e.what()<<std::endl;
    	}
    	catch (...)
    	{
    		std::cout<<"Exeption inconnu"<<std::endl;
    	}
						
		if constexpr (is_same<T,std::string>::value) {
	    	for (unsigned int i = 0;i < length;i++){
		    	attr_String_read[i] = const_cast<char *>(_processSpectrum->accessData(i).c_str());
    			att.set_value(attr_String_read,length);
    		}
	    }
	    else {
	    	unsigned int length = _processSpectrum->getNumberOfSamples();
			att.set_value(_processSpectrum->accessChannel(0).data(),length);
		}
	}

	virtual void write(Tango::DeviceImpl *dev,
			   Tango::WAttribute &att)
	{	

    auto& processVector = _processSpectrum->accessChannel(0);
  
    long arraySize = att.get_write_value_length();

    switch (_dataType)
	    {
	    case Tango::DEV_UCHAR:			
			const uint8_t *c_value;
			att.get_write_value(c_value);
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = c_value[i];
      		}				
			break;
		case Tango::DEV_USHORT:		
			const Tango::DevUShort *us_value;
			att.get_write_value(us_value);
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = us_value[i];
      		}				
			break;
		
		case Tango::DEV_ULONG:
			const uint32_t *ul_value;
			att.get_write_value(ul_value);
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = ul_value[i];
      		}				
			break;
		case Tango::DEV_ULONG64:
			const uint64_t *ul64_value;
			att.get_write_value(ul64_value);			
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = ul64_value[i];
      		}				
			break;
		case  Tango::DEV_SHORT:				
			const int16_t *s_value;
			att.get_write_value(s_value);				
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = s_value[i];
      		}				
			break;

		case Tango::DEV_LONG:				
			const int32_t *l_value;
			att.get_write_value(l_value);
			for(size_t i = 0; i < arraySize; ++i) {
	      		processVector[i] = l_value[i];
	        }
			break;
		    		
		case Tango::DEV_LONG64:				
			const int64_t *l64_value;
			att.get_write_value(l64_value);	
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = l64_value[i];
      		}							
			break;

		case Tango::DEV_FLOAT:			
			const float *f_value;
			att.get_write_value(f_value);				
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = f_value[i];
      		}
			break;
		case Tango::DEV_DOUBLE:			
			const double *d_value;
			att.get_write_value(d_value);
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = d_value[i];
       		}
			break;
					
		}			

	  _processSpectrum->write();

	}

	virtual bool is_allowed(Tango::DeviceImpl *dev,	Tango::AttReqType ty){return true;}

	boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> _processSpectrum;
	long _dataType; 
	unsigned int length;
	Tango::DevString *attr_String_read;
protected:

};

} // namespace ChimeraTK

#endif // _SPECTRUM_ATTRIB_H_

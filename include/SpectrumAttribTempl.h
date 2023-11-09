#pragma once
#include "AttributProperty.h"
#include "TangoPropertyHelper.h"
#include <ChimeraTK/NDRegisterAccessor.h>

namespace ChimeraTK {
template <typename T>	
class SpectrumAttribTempl : public Tango::SpectrumAttr,Tango::LogAdapter
{
public:
	SpectrumAttribTempl(TANGO_BASE_CLASS* tangoDevice, boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty
		/*const char *name, long data_type, Tango::AttrWriteType w_type*/, std::string description="description", std::string unit=""):
		Tango::SpectrumAttr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType,pv->getNumberOfSamples()),_processSpectrum(pv),
		_dataType(attProperty->_dataType), Tango::LogAdapter(tangoDevice)
	{
		DEBUG_STREAM<<" SpectrumAttribTempl::SpectrumAttribTempl  Name: "<<attProperty->_name.c_str()<<
		              " Type"<<attProperty->_dataType<<" _writeType: "<<attProperty->_writeType<<endl;

		if (attProperty->_writeType != Tango::READ)
			_memoried_property_name = "__Memoried_" + attProperty->_name;


		Tango::UserDefaultAttrProp axis_prop;
		axis_prop.set_label(attProperty->_name.c_str());
		//axis_prop.set_format("%9.5f");

		axis_prop.set_description(attProperty->_desc .c_str());

		if (!(attProperty->_unit.empty()))
		    	axis_prop.set_unit(attProperty->_unit.c_str());

		_length = _processSpectrum->getNumberOfSamples();

		if constexpr (is_same<T,std::string>::value) {	    
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
		DEBUG_STREAM<<"SpectrumAttribTempl::read "<< get_name()<<endl;
						
		DEBUG_STREAM<<"getNumberOfSamples: "<<_processSpectrum->getNumberOfSamples()<<endl;

		if constexpr (is_same<T,std::string>::value) {

			for (unsigned int i = 0;i < _length;i++){
	    			attr_String_read[i] = const_cast<char *>(_processSpectrum->accessData(i).c_str());
	    			att.set_value(attr_String_read,_length);
			}
		}
		else if (_dataType==Tango::DEV_BOOLEAN)
		{
			boost::shared_ptr<ChimeraTK::NDRegisterAccessor<Boolean>> pv =boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(_processSpectrum);
			for (unsigned int i = 0;i < _length;i++){
				attr_Bool_read[i] = _processSpectrum->accessData(i);
			}

			att.set_value(attr_Bool_read,_length);

		}
		else
		{
			att.set_value(_processSpectrum->accessChannel(0).data(),_length);
			for (unsigned int i = 0;i < _length;i++){
				DEBUG_STREAM<<"pv["<<i<<"]= "<<_processSpectrum->accessData(i)<<endl;
			}

		}/*
		if(_processSpectrum->dataValidity() != ChimeraTK::DataValidity::ok) {
			// set data invalid
			att.set_quality(Tango::ATTR_INVALID, true);
		}
		else {
			att.set_quality(Tango::ATTR_VALID, true);
		}*/
	}

	virtual void write(Tango::DeviceImpl *dev,
			   Tango::WAttribute &att)
	{	
		DEBUG_STREAM<< "SpectrumAttribTempl::write "<< get_name()<<endl;

		auto& processVector = _processSpectrum->accessChannel(0);
  
		long arraySize = att.get_write_value_length();
		std::string memoried_value;

		if ( arraySize > _length )
		{
		std::stringstream msg;
		msg<< "Array size cannot be greater than"<< _length<<"\n";

		ERROR_STREAM<<"WRITE_ERROR "<<msg.str()<<endl;

		Tango::Except::throw_exception("WRITE_ERROR",
                        msg.str(),
                        "SpectrumAttribTempl::write()");
		}

		switch (_dataType)
		{
		case Tango::DEV_UCHAR:
	    	{
			const uint8_t *c_value;
			att.get_write_value(c_value);
			for (size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = c_value[i];
      			}
      			memoried_value = array_to_string(c_value,_length);
      		}
			break;
		case Tango::DEV_USHORT:		
	    	{
			const Tango::DevUShort *us_value;
			att.get_write_value(us_value);
			for (size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = us_value[i];
      			}
      			memoried_value = array_to_string(us_value,_length);
      		}
			break;
		
		case Tango::DEV_ULONG:
	    	{
			const uint32_t *ul_value;
			att.get_write_value(ul_value);
			for (size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = ul_value[i];
      			}
      			memoried_value = array_to_string(ul_value,_length);
      		}
			break;
		case Tango::DEV_ULONG64:
	    	{
			const uint64_t *ul64_value;
			att.get_write_value(ul64_value);			
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = ul64_value[i];
      			}
      			memoried_value = array_to_string(ul64_value,_length);
      		}
			break;
		case  Tango::DEV_SHORT:				
	    	{
			const int16_t *s_value;
			att.get_write_value(s_value);				
			for (size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = s_value[i];
      			}
      			memoried_value = array_to_string(s_value,_length);
		}
			break;

		case Tango::DEV_LONG:				
	    	{
			const int32_t *l_value;
			att.get_write_value(l_value);
			for (size_t i = 0; i < arraySize; ++i) {
	      		processVector[i] = l_value[i];
	        	}
	        	memoried_value = array_to_string(l_value,_length);
		}
			break;
		    		
		case Tango::DEV_LONG64:				
	    	{
			const int64_t *l64_value;
			att.get_write_value(l64_value);	
			for (size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = l64_value[i];
      			}
      			memoried_value = array_to_string(l64_value,_length);
      		}
			break;

		case Tango::DEV_FLOAT:			
		{
			const float *f_value;
			att.get_write_value(f_value);				
			DEBUG_STREAM<< "arraySize "<<arraySize<<endl;
			for(size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = f_value[i];
      				DEBUG_STREAM<<"processVector["<<i<<"] =" <<f_value[i]<<endl;
      			}
      			memoried_value = array_to_string(f_value,_length);
      		}
			break;
		case Tango::DEV_DOUBLE:
	    	{
			const double *d_value;
			att.get_write_value(d_value);
			for(size_t i = 0; i < arraySize; ++i) {
      			processVector[i] = d_value[i];
       			}
       			memoried_value = array_to_string(d_value,_length);
       		}
			break;
		case Tango::DEV_BOOLEAN:
	    	{
			const Tango::DevBoolean	*w_val;
			att.get_write_value(w_val);
			for (size_t i = 0; i < arraySize; ++i) {
      				processVector[i] = w_val[i];
       			}
       			memoried_value = array_to_string(w_val,_length);
       		}
			break;
		case Tango::DEV_STRING:
			if constexpr (is_same<T, std::string>::value) {
				const Tango::ConstDevString *w_val;
				att.get_write_value(w_val);
				for(size_t i = 0; i < arraySize; ++i) {
	      				processVector[i] = std::string(w_val[i]);
	       			}
	       			memoried_value = array_to_string(w_val,_length);
			}
			break;
		default:
			ERROR_STREAM<<" SpectrumAttribTempl::write unsupported dataTye "<<_dataType<< endl;
		}

		_processSpectrum->write();

		set_property<std::string>(dev, _memoried_property_name,memoried_value);
		DEBUG_STREAM<< "SpectrumAttribTempl::write END"<< endl;
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


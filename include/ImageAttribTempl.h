#ifndef _Image_ATTRIB_H_
#define _Image_ATTRIB_H_


#include "AttributProperty.h"
#include <boost/shared_ptr.hpp>
#include <ChimeraTK/ScalarRegisterAccessor.h>
#include <ChimeraTK/OneDRegisterAccessor.h>
 

namespace ChimeraTK {
template <typename T>	
class ImageAttribTempl : public Tango::ImageAttr
{
public:
	ImageAttribTempl( boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv, std::shared_ptr<AttributProperty>attProperty
		/*const char *name, long data_type, Tango::AttrWriteType w_type*/, std::string description="description", std::string unit=""):
		Tango::ImageAttr(attProperty->_name.c_str(), attProperty->_dataType, attProperty->_writeType,pv->getNumberOfSamples()),_processImage(pv),_dataType(attProperty->_dataType)
	{
		Tango::UserDefaultAttrProp axis_prop;
		axis_prop.set_label(attProperty->_name.c_str());

		att_prop.set_description(attProperty->_desc .c_str());
	    att_prop.set_unit(attProperty->_unit.c_str());
		set_default_properties(axis_prop);
	}

	virtual ~ImageAttribTempl(void) {};

	virtual void read(Tango::DeviceImpl *dev,
			  Tango::Attribute &att)
	{
		_processImage->readLatest();
		auto data=_processImage->accessChannels();
		
		unsigned int w = _processImage->getNumberOfSamples();
		unsigned int h = _processImage->getNumberOfChannels();

		switch (_dataType)
	    {
	    	case Tango::DEV_UCHAR:
	    		uint8_t *data = new uint8_t[h*w];
	    		for(unsigned int i = 0; i < h; ++i) {
      				for(unsigned int j = 0; i < w; ++j)
     					data[i*w+j] = _processImage->accessData(i,j);
    			}
				attr.set_value(data, w, h);a
			break;
			case Tango::DEV_USHORT:
				Tango::DevUShort *data = new uint16_t[h*w];;

		        for(unsigned int i = 0; i < h; ++i) {
		      		for(unsigned int j = 0; i < w; ++j)
		     			data[i*w+j] = _processImage->accessData(i,j);
				}
				attr.set_value(data, w, h);
			break;


	    case  Tango::DEV_SHORT:

			Tango::DevUShort *data = new int16_t[h*w];;

		    for(unsigned int i = 0; i < h; ++i) {
		      	for(unsigned int j = 0; i < w; ++j)
     				_processImage[i][j] = d_value[i*w+j];
			}
			break;

		}


	}

	virtual void write(Tango::DeviceImpl *dev,
			   Tango::WAttribute &att)
	{	


    auto& processVector = _processImage->accessChannel();
	unsigned int w = _processImage->getNumberOfSamples();
	unsigned int h = _processImage->getNumberOfChannels();

    unsigned int arraySize = w*h;

    switch (_dataType)
	    {
	    case Tango::DEV_UCHAR:			
			const uint8_t *c_value;
			att.get_write_value(c_value);
      		for(unsigned int i = 0; i < h; ++i) {
      			for(unsigned int j = 0; i < w; ++j) 
     				_processImage[i][j] = c_value[i*w+j];
    		}
						
			break;
		case Tango::DEV_USHORT:		
			const Tango::DevUShort *us_value;
			
			att.get_write_value(us_value);
	        for(unsigned int i = 0; i < h; ++i) {
	      		for(unsigned int j = 0; i < w; ++j) 
	     			_processImage[i][j] = us_value[i*w+j];

			}				
			break;
		
	
	    case  Tango::DEV_SHORT:
					
			const int16_t *s_value;
			att.get_write_value(s_value);
		    for(unsigned int i = 0; i < h; ++i) {
		      	for(unsigned int j = 0; i < w; ++j) 
     				_processImage[i][j] = d_value[i*w+j];    		
			}				
			break;

		}			


	    _processImage->write();

	}

	virtual bool is_allowed(Tango::DeviceImpl *dev,	Tango::AttReqType ty){return true;}

	boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> _processImage;
	// image type
	long _dataType;


protected:

};

} // namespace ChimeraTK

#endif // _Image_ATTRIB_H_

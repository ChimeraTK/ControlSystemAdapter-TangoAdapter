
#ifndef TANGO_ADPATER_H
#define TANGO_ADPATER_H




#include <string>
#include <tango.h>

#include <boost/shared_ptr.hpp>
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/PVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>

#include "ScalarAttribTempl.h"
#include "SpectrumAttribTempl.h"


namespace ChimeraTK::TangoAdapter {

class AdapterImpl: public Tango::LogAdapter
{
public:
    AdapterImpl(TANGO_BASE_CLASS* tangoDevice , std::vector<std::string> attributList);
    ~AdapterImpl();
    
    const TANGO_BASE_CLASS* getDevice() const {return _device;}
        
    
    void create_dynamic_attributes(void);

    void create_Scalar_Attr(std::shared_ptr<AttributProperty> const& AttributProperty);
    void create_Spectrum_Attr(std::shared_ptr<AttributProperty> const& AttributProperty);
    void create_Image_Attr(std::shared_ptr<AttributProperty> const& AttributProperty);
    
    void detach_dynamic_attributes_from_device(void);
    void attach_dynamic_attributes_to_device(void);

    void write_inited_values(void);
    
    boost::shared_ptr<DevicePVManager> const& getDevicePVManager() const { return _devicePVManager; }

    boost::shared_ptr<ControlSystemPVManager> const& getControlSystemPVManager() const {
       return _controlSystemPVManager;
  }
private:
    TANGO_BASE_CLASS *_device;
    vector<string>  _attributeList;
    boost::shared_ptr<ControlSystemPVManager> _controlSystemPVManager;
    boost::shared_ptr<DevicePVManager> _devicePVManager;
    std::vector<Tango::Attr *> _dynamic_attribute_list;
    std::vector<int> _index_write_spectrum_attr_list;
        
    };

} // namespace ChimeraTK
#endif // TANGO_ADPATER_H

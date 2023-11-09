#pragma once


#include <string>
#include <tango.h>

#include <boost/shared_ptr.hpp>
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/PVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>


#include "TangoUpdater.h"

namespace ChimeraTK {

class TangoAdapter: public Tango::LogAdapter
{
public:
    TangoAdapter(TANGO_BASE_CLASS* tangoDevice , std::vector<std::string> attributList);
    ~TangoAdapter();
    
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
    boost::shared_ptr<TangoUpdater> _updater;
    TANGO_BASE_CLASS *_device;

    boost::shared_ptr<ControlSystemPVManager> _controlSystemPVManager;
    boost::shared_ptr<DevicePVManager> _devicePVManager;
    // dynamic attribute list
    std::vector<Tango::Attr *> _dynamic_attribute_list;
    // list W spectrum attributs for initialization
    std::map<std::shared_ptr<AttributProperty>,int> _write_spectrum_attr_list;

        
    };

} // namespace ChimeraTK


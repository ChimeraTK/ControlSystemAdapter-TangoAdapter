



#include <stdexcept>


#include <ChimeraTK/ReadAnyGroup.h>
#include <ChimeraTK/RegisterPath.h>
#include "AttributMapper.h"
#include "TangoAdapter.h"
#include "getAllVariableNames.h"



namespace ChimeraTK {
//+----------------------------------------------------------------------------
//
// method :        
//
// description :    
//
//-----------------------------------------------------------------------------
TangoAdapter::TangoAdapter(TANGO_BASE_CLASS* tangoDevice,  std::vector<std::string> attributList):  Tango::LogAdapter(tangoDevice) {

    DEBUG_STREAM<<"TangoAdapter::TangoAdapter starting ... "<<endl;
    _device = tangoDevice;


    std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
        createPVManager();

    _controlSystemPVManager = pvManagers.first;
    _devicePVManager = pvManagers.second;

	//ApplicationBase::getInstance().getName().c_str();
	ApplicationBase::getInstance().setPVManager(_devicePVManager);

    ApplicationBase::getInstance().initialise();

    // the variable manager can only be filled after we have the CS manager  
    std::set<std::string> names= getAllVariableNames(_controlSystemPVManager);

    for (std::set<std::string>::iterator it=names.begin(); it!=names.end(); ++it)
      std::cout<< *it <<endl;

    // no configuration, import all
    if (attributList.size()==0 || (attributList.size()==1&& attributList[0]==""))
    {
        ChimeraTK::AttributMapper::getInstance().setCSPVManager(_controlSystemPVManager);
        ChimeraTK::AttributMapper::getInstance().directImport(names);   
    }
    // configured attributs from property
    else {
        ChimeraTK::AttributMapper::getInstance().prepareOutput(attributList); 
    }
    // create the dynamic attributes for Tango devices
    create_dynamic_attributes();

    // only need to write spectrum attributs
    //scalar attributs are memoried and initialized by Tango
    write_inited_values();

    ApplicationBase::getInstance().run();
    
    DEBUG_STREAM<<"TangoAdapter::TangoAdapter end"<<endl;
}
//+----------------------------------------------------------------------------
//
// method :        
//
// description :   
//
//-----------------------------------------------------------------------------
TangoAdapter::~TangoAdapter(){

	DEBUG_STREAM << " TangoAdapter::~TangoAdapter" << endl;
	detach_dynamic_attributes_from_device();
    // Attention to shut down application here but impossible
    //ChimeraTK::ApplicationBase::getInstance().shutdown();

}

void TangoAdapter::write_inited_values(void){

    DEBUG_STREAM << " TangoAdapter::write_inited_values " << endl;
    //read spectrum values from memoried properties then write as initialised values
    for (int i : _index_write_spectrum_attr_list)
    {
        //auto* spectrum_attr_t = dynamic_cast<SpectrumAttribTempl*>( _dynamic_attribute_list[i]);
        //spectrum_attr_t->initialise();
    }
    // check for variables not yet initialised - we must guarantee that all to-application variables are written exactly
    // once at server start.
    for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
        if(!pv->isWriteable()) continue;
  
        if(pv->getVersionNumber() == ChimeraTK::VersionNumber(nullptr)) {
            // The variable has not yet been written. Do it now, even if we just send a 0.
            //pv->write();
        }
    }

}


//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::create_dynamic_attributes()
//
// description :    This method creates the dynamic attrs.
//
//-----------------------------------------------------------------------------
void TangoAdapter::create_dynamic_attributes(void){

    DEBUG_STREAM << " TangoAdapter::create_dynamic_attributes " << endl;

    auto descList = AttributMapper::getInstance().getAttDescList();
    for (auto attDesc:descList)
    {

        if (attDesc->getDataFormat()==SCALAR)
        {
            create_Scalar_Attr(attDesc);            
        }
        else if (attDesc->getDataFormat()==SPECTRUM)
        {
            create_Spectrum_Attr(attDesc);    
        }
        /*
        else if (attDesc->getDataFormat()==IMAGE)
        {
            create_Image_Attr(attDesc);
        }*/
    }

    attach_dynamic_attributes_to_device();
	


}


//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::create_Scalar_Attr()
//
// description :    This method creates the scalar attr.
//
//-----------------------------------------------------------------------------
void TangoAdapter::create_Scalar_Attr(std::shared_ptr<AttributProperty> const& attProp)
{
    DEBUG_STREAM<<"TangoAdapter::create_Scalar_Attr"<<endl;
    
 	ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->_path);

    if (processVariable->isWriteable() && processVariable->isReadable()){
        attProp->_writeType=Tango::READ_WRITE;        
    }
    else if (processVariable->isWriteable()){
        attProp->_writeType=Tango::WRITE;
    }
    else { 
        attProp->_writeType=Tango::READ;
    }

    switch (attProp->_dataType){
        case Tango::DEV_UCHAR :
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
            auto* scalar_attr_t=new ScalarAttribTempl<uint8_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_USHORT:
        {
            boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint16_t>> pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
            auto* scalar_attr_t=new ScalarAttribTempl<uint16_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_SHORT:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
            auto* scalar_attr_t=new ScalarAttribTempl<int16_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_DOUBLE:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<double>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_FLOAT:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
            auto* scalar_attr_t=new ScalarAttribTempl<float>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_ULONG64:
        {
            auto pv =
              boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<uint64_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_LONG64:
        {
            auto pv =
              boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<int64_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_ULONG:
        {
            auto pv =
              boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<uint32_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_LONG:
        {
            auto pv =
              boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<int32_t>(_device, pv, attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_STRING:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
            auto* scalar_attr_t = new ScalarAttribTempl<std::string>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        case Tango::DEV_BOOLEAN:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
            //Attribut Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
            auto cast_pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(pv);
            auto* scalar_attr_t = new ScalarAttribTempl<uint8_t>(_device, cast_pv,attProp);
            _dynamic_attribute_list.push_back(scalar_attr_t);
            break;
        }
        default:

            ERROR_STREAM <<" Not supported data type in scalar: "<< attProp->_dataType<<endl;
            break;
    }
    
}

//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::create_Spectrum_Attr()
//
// description :    This method creates the scalar attr.
//
//-----------------------------------------------------------------------------
void TangoAdapter::create_Spectrum_Attr(std::shared_ptr<AttributProperty> const& attProp)
{
    DEBUG_STREAM <<"TangoAdapter::create_Spectrum_Attr"<<endl;
    
    ProcessVariable::SharedPtr processVariable = _controlSystemPVManager->getProcessVariable(attProp->_path);
    
    if (processVariable->isWriteable() && processVariable->isReadable()){
        attProp->_writeType=Tango::READ_WRITE;
    }
    else if (processVariable->isWriteable()){
        attProp->_writeType=Tango::WRITE;
    }
    else { 
        attProp->_writeType=Tango::READ;
    }

    if (processVariable->isWriteable()) std::cout<<"create_Spectrum_Attr WRITE"<<std::endl;

    switch (attProp->_dataType){
        case Tango::DEV_UCHAR :
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<uint8_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_USHORT:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
            auto* spectrum_attr_t = new SpectrumAttribTempl<uint16_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_SHORT:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int16_t>>(processVariable);
            auto* spectrum_attr_t = new SpectrumAttribTempl<int16_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_ULONG:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
            auto* spectrum_attr_t = new SpectrumAttribTempl<uint32_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_LONG:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
            auto* spectrum_attr_t = new SpectrumAttribTempl<int32_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_DOUBLE:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<double>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_FLOAT:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<float>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_ULONG64:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<uint64_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_LONG64:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<int64_t>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_STRING:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
            auto* spectrum_attr_t=new SpectrumAttribTempl<std::string>(_device, pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;
        }
        case Tango::DEV_BOOLEAN:
        {
            auto pv = boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<Boolean>>(processVariable);
            //Attribut Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
            auto casted_pv = boost::reinterpret_pointer_cast<ChimeraTK::NDRegisterAccessor<uint8_t>>(pv);

            auto* spectrum_attr_t = new SpectrumAttribTempl<uint8_t>(_device, casted_pv,attProp);
            _dynamic_attribute_list.push_back(spectrum_attr_t);
            break;

        }
    }

    _index_write_spectrum_attr_list.push_back(_dynamic_attribute_list.size()-1);

}


//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::attach_dynamic_attributes_to_device()
//
// description :    This method attachs the dynamics attributes to the device.
//
//-----------------------------------------------------------------------------
void TangoAdapter::attach_dynamic_attributes_to_device(void)
{
    DEBUG_STREAM <<"TangoAdapter::attach_dynamic_attributes_to_device"<<endl;

    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i) {    
        _device->add_attribute(_dynamic_attribute_list[i]);
    }

    _device->set_state(Tango::ON);
}

//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::detach_dynamic_attributes_from_device()
//
// description :    This method detachs the dynamics attributes from the device.
//
//-----------------------------------------------------------------------------
void TangoAdapter::detach_dynamic_attributes_from_device(void)
{
    DEBUG_STREAM <<"TangoAdapter::detach_dynamic_attributes_from_device"<<endl;

    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i)
        _device->remove_attribute(_dynamic_attribute_list[i], false);
}
} // namespace ChimeraTK




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
TangoAdapter::TangoAdapter(TANGO_BASE_CLASS* tangoDevice,  std::vector<std::string> attributList){
//plan to set dmapPath inside the construction

    device = tangoDevice;


    std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
        createPVManager();

    _controlSystemPVManager = pvManagers.first;
    _devicePVManager = pvManagers.second;

	ApplicationBase::getInstance().getName().c_str();
	ApplicationBase::getInstance().setPVManager(_devicePVManager);

    ApplicationBase::getInstance().initialise();

    // the variable manager can only be filled after we have the CS manager  
    std::set<std::string> names= getAllVariableNames(_controlSystemPVManager);

    for (std::set<std::string>::iterator it=names.begin(); it!=names.end(); ++it)
      std::cout << *it<<std::endl;
    std::cout<<attributList.size()<<std::endl;

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
    
    write_inited_values();

    ApplicationBase::getInstance().run();
    

}
//+----------------------------------------------------------------------------
//
// method :        
//
// description :   
//
//-----------------------------------------------------------------------------
TangoAdapter::~TangoAdapter(){
	
	detach_dynamic_attributes_from_device();
    // Attention to shut down application here but impossible
    //ChimeraTK::ApplicationBase::getInstance().shutdown();

}

void TangoAdapter::write_inited_values(void){

   //write memoried values first
   

  // check for variables not yet initialised - we must guarantee that all to-application variables are written exactly
  // once at server start.
  for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
    if(!pv->isWriteable()) continue;
  
    if(pv->getVersionNumber() == ChimeraTK::VersionNumber(nullptr)) {
      // The variable has not yet been written. Do it now, even if we just send a 0.
      // Don't write anything now
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

    std::cout<<"TangoAdapter::create_dynamic_attributes"<<std::endl;    
 
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
        else if (attDesc->getDataFormat()==IMAGE)
        {
            create_Image_Attr(attDesc);
        }
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
    std::cout<<"TangoAdapter::create_Scalar_Attr"<<std::endl;
    
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

    if (attProp->_dataType==Tango::DEV_USHORT) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint16_t>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        ScalarAttribTempl<uint16_t>* scalar_attr_t=new ScalarAttribTempl<uint16_t>(pv,attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);

    }
    if (attProp->_dataType==Tango::DEV_DOUBLE) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        ScalarAttribTempl<double>* scalar_attr_t=new ScalarAttribTempl<double>(pv,attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        
    }
    if (attProp->_dataType == Tango::DEV_ULONG64) {
        auto pv =
          boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint64_t>(pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
    }
    if (attProp->_dataType == Tango::DEV_LONG64) {
        auto pv =
          boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<int64_t>(pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
    }
    if (attProp->_dataType == Tango::DEV_ULONG) {
        auto pv =
          boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<uint32_t>(pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
    }
    if (attProp->_dataType == Tango::DEV_LONG) {
        auto pv =
          boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        auto* scalar_attr_t = new ScalarAttribTempl<int32_t>(pv, attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
    }

    else if (attProp->_dataType==Tango::DEV_FLOAT) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<float>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        ScalarAttribTempl<float>* scalar_attr_t=new ScalarAttribTempl<float>(pv,attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        
    }
    else if (attProp->_dataType==Tango::DEV_STRING) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<std::string>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        ScalarAttribTempl<std::string>* scalar_attr_t=new ScalarAttribTempl<std::string>(pv,attProp);
        _dynamic_attribute_list.push_back(scalar_attr_t);
        
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


    if (attProp->_dataType==Tango::DEV_USHORT) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<uint16_t>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<uint16_t>>(processVariable);
        SpectrumAttribTempl<uint16_t>* spectrum_attr_t=new SpectrumAttribTempl<uint16_t>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);

    }
    if (attProp->_dataType==Tango::DEV_LONG) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<int32_t>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int32_t>>(processVariable);
        SpectrumAttribTempl<int32_t>* spectrum_attr_t=new SpectrumAttribTempl<int32_t>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        
    }    
    if (attProp->_dataType==Tango::DEV_DOUBLE) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<double>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<double>>(processVariable);
        SpectrumAttribTempl<double>* spectrum_attr_t=new SpectrumAttribTempl<double>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        
    }
    else if (attProp->_dataType==Tango::DEV_FLOAT) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<float>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<float>>(processVariable);
        SpectrumAttribTempl<float>* spectrum_attr_t=new SpectrumAttribTempl<float>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        
    }
    else if (attProp->_dataType==Tango::DEV_LONG64) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<int64_t>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<int64_t>>(processVariable);
        SpectrumAttribTempl<int64_t>* spectrum_attr_t=new SpectrumAttribTempl<int64_t>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        
    }    
    
    else if (attProp->_dataType==Tango::DEV_STRING) {
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<std::string>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<std::string>>(processVariable);
        SpectrumAttribTempl<std::string>* spectrum_attr_t=new SpectrumAttribTempl<std::string>(pv,attProp);
        _dynamic_attribute_list.push_back(spectrum_attr_t);
        
    }        
}

//+----------------------------------------------------------------------------
//
// method :         TangoAdapter::create_Image_Attr()
//
// description :    This method creates the scalar attr.
//
//-----------------------------------------------------------------------------
void TangoAdapter::create_Image_Attr(std::shared_ptr<AttributProperty> const& attProp)
{
    std::cout<<"TangoAdapter::create_Image_Attr"<<std::endl;
    
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

    //..........
  
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
    
    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i) {    
        device->add_attribute(_dynamic_attribute_list[i]);
    }

    device->set_state(Tango::ON);
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
    for(size_t i = 0; i < _dynamic_attribute_list.size(); ++i)
        device->remove_attribute(_dynamic_attribute_list[i], false);
}
} // namespace ChimeraTK
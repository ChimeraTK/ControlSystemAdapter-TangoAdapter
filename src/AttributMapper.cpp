#include "AttributMapper.h"
#include <ChimeraTK/Utilities.h>
#include <ChimeraTK/RegisterPath.h>
#include <algorithm>
#include <locale>
#include <regex>

namespace ChimeraTK {

  /********************************************************************************************************************/

  AttributMapper& AttributMapper::getInstance() {
    static AttributMapper instance;
    return instance;
  }
  /********************************************************************************************************************/
  void AttributMapper::directImport(std::set<std::string> inputVariables){

    clear();
   
    _inputVariables = inputVariables;    
   //import all
    import("/",""); 
  }
  /********************************************************************************************************************/
  void AttributMapper::prepareOutput(std::vector<std::string> attributList){
    clear();    
    for (auto attrDesc:attributList)
    {      
      // prepare the property description
      auto attributProperty = std::make_shared<AttributProperty>(attrDesc);
      _descriptions.push_back(attributProperty);
      _usedInputVariables.insert(attributProperty->_path);
    }
  }
  /********************************************************************************************************************/
  void AttributMapper::clear(){

    _inputVariables.clear();
    _usedInputVariables.clear();
  }
  /********************************************************************************************************************/
  std::list<std::shared_ptr<AttributProperty>> const& AttributMapper::getAttDescList(void) const{
    return _descriptions;
  }
  /********************************************************************************************************************/
  void AttributMapper::import(std::string importSource, std::string importLocationName, std::string directory) {
    
    // a slash will be added after the source, so we make the source empty for an
    // import of everythingprocessVariableName
    if (importSource == "/") {
      importSource = "";
    }

    // loop source tree, cut beginning, replace / with _ and add a property
    for (auto const& processVariableName : _inputVariables) {
      if (_usedInputVariables.find(processVariableName) != _usedInputVariables.end()) {
        continue;
      }

      if(processVariableName.find(importSource + "/") == 0) {
        // processVariableName starts with wanted source
        auto nameSource = processVariableName.substr(importSource.size() + 1); // add the slash to be removed

        // we use the register path because it removes duplicate separators and
        // allows to use . as separater to replace all / with .
        ChimeraTK::RegisterPath propertyName;
        std::string locationName;

        auto locationAndPropertyName = splitStringAtFirstSlash(nameSource);
        locationName = locationAndPropertyName.first;
        propertyName = locationAndPropertyName.second;
        propertyName.setAltSeparator(".");
        
        //erase the first "/" 
        std::string attrName=locationName+"_"+propertyName.getWithAltSeparator();

        std::string::size_type i = attrName.find("/");
        if (i != std::string::npos)
           attrName.erase(i, 1);

        if(locationName.empty()) {
          throw std::logic_error(std::string("Invalid XML content in global import of ") +
              (importSource.empty() ? "/" : importSource) + ":  Cannot create location name from '" + nameSource +
              "', one hirarchy level is missing.");
        }
        // derive the datatype      
        auto processVariable = _controlSystemPVManager->getProcessVariable(processVariableName);
        std::type_info const& valueType = processVariable->getValueType();


        deriveType(valueType);

        // detect dataFormat
        size_t nSamples;
        size_t nChannels;
     
        // function in Device Access to check if valueType is in the map userTypeMap
        callForType(valueType, [&](auto t) {  
          using T = decltype(t);
          boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv =boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<T>>(processVariable);
          nSamples = pv->getNumberOfSamples();
          nChannels = pv->getNumberOfChannels();


        });        

        ChimeraTK::AttrDataFormat dataFormat = SCALAR;
        if (nChannels>1)
        {  
          dataFormat = IMAGE;
        }
        else if (nSamples>1)
        {
          dataFormat = SPECTRUM;
        }

        //creating attribut property
        auto attributProperty = std::make_shared<AttributProperty>(attrName,nameSource,dataFormat, 
                                 _dataType,processVariable->getDescription(),processVariable->getUnit());
        _descriptions.push_back(attributProperty);
        _usedInputVariables.insert(attributProperty->_path);        
      }
    }
  }

  /********************************************************************************************************************/
  void AttributMapper::deriveType(std::type_info const& info) {
       _dataType = 0;

      if(info == typeid(uint8_t)) _dataType = Tango::DEV_UCHAR;
      if(info == typeid(int8_t))  _dataType = Tango::DEV_SHORT;

      if (info == typeid(uint16_t)) _dataType = Tango::DEV_USHORT;
    if (info == typeid(int16_t)) _dataType = Tango::DEV_SHORT;
    if (info == typeid(uint32_t)) _dataType = Tango::DEV_ULONG;
    if (info == typeid(int32_t))  _dataType = Tango::DEV_LONG;
    if (info == typeid(uint64_t)) _dataType = Tango::DEV_ULONG64;
    if (info == typeid(int64_t)) _dataType = Tango::DEV_LONG64;
    if (info == typeid(float)) _dataType = Tango::DEV_FLOAT;
    if (info == typeid(double)) _dataType = Tango::DEV_DOUBLE;
    if (info == typeid(std::string)) _dataType = Tango::DEV_STRING;
    if (info == typeid(ChimeraTK::Boolean)) _dataType = Tango::DEV_BOOLEAN;
    if (info == typeid(ChimeraTK::Void))    _dataType = DataType::Void;

    //std::cout<<_dataType<<std::endl;
  }  

} // namespace ChimeraTK
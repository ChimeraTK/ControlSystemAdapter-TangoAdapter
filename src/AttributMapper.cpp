#include "AttributeMapper.h"

#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/Utilities.h>

#include <algorithm>
#include <locale>
#include <regex>

namespace ChimeraTK {

  namespace util {
    static std::pair<std::string, std::string> splitStringAtFirstSlash(std::string& input) {
      // find first slash
      auto slashPosition = input.find_first_of('/');
      if(slashPosition == 0) { // ignore leading slash
        input = input.substr(1);
        slashPosition = input.find_first_of('/');
      }
      // no slash found: return empty location name
      if(slashPosition == std::string::npos) {
        return std::make_pair(std::string(), input);
      }
      // split at first slash into location name and property name
      auto locationName = input.substr(0, slashPosition);
      auto propertyName = input.substr(slashPosition + 1);
      // replace any remaining slashes in property name with dots
      while((slashPosition = propertyName.find_first_of('/')) != std::string::npos) {
        propertyName[slashPosition] = '.';
      }
      return std::make_pair(locationName, propertyName);
    }

  } // namespace util

  /********************************************************************************************************************/
  void AttributeMapper::directImport(std::set<std::string>& inputVariables) {
    clear();

    _inputVariables = inputVariables;
    // import all
    import("/", std::string(""));
  }
  /********************************************************************************************************************/
  void AttributeMapper::prepareOutput(std::vector<std::shared_ptr<ChimeraTK::AttributeProperty>>& attributeList) {
    clear();

    _descriptions.insert(_descriptions.end(), attributeList.begin(), attributeList.end());
    for(const auto& attrDesc : _descriptions) {
      _usedInputVariables.insert(attrDesc->path);
    }
  }
  /********************************************************************************************************************/
  void AttributeMapper::clear() {
    _inputVariables.clear();
    _usedInputVariables.clear();
  }
  /********************************************************************************************************************/
  std::list<std::shared_ptr<AttributeProperty>> const& AttributeMapper::getAttDescList() const {
    return _descriptions;
  }
  /********************************************************************************************************************/
  void AttributeMapper::import(std::string importSource, [[maybe_unused]] const std::string& importLocationName,
      [[maybe_unused]] const std::string& directory) {
    // a slash will be added after the source, so we make the source empty for an
    // import of everythingprocessVariableName
    if(importSource == "/") {
      importSource = "";
    }

    // loop source tree, cut beginning, replace / with _ and add a property
    for(auto const& processVariableName : _inputVariables) {
      if(_usedInputVariables.find(processVariableName) != _usedInputVariables.end()) {
        continue;
      }

      if(processVariableName.find(importSource + "/") == 0) {
        // processVariableName starts with wanted source
        auto nameSource = processVariableName.substr(importSource.size() + 1); // add the slash to be removed

        // we use the register path because it removes duplicate separators and
        // allows to use . as separater to replace all / with .
        ChimeraTK::RegisterPath propertyName;
        std::string locationName;

        auto locationAndPropertyName = util::splitStringAtFirstSlash(nameSource);
        locationName = locationAndPropertyName.first;
        propertyName = locationAndPropertyName.second;
        propertyName.setAltSeparator(".");

        std::string attrName;
        // erase the first "/"
        if (!locationName.empty()) {
          attrName = locationName + "_" + propertyName.getWithAltSeparator();
        } else {
          attrName = propertyName.getWithAltSeparator();
        }

        auto i = attrName.find('/');
        if(i != std::string::npos) {
          attrName.erase(i, 1);
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
          boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv =
              boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<T>>(processVariable);
          nSamples = pv->getNumberOfSamples();
          nChannels = pv->getNumberOfChannels();
        });

        ChimeraTK::AttrDataFormat dataFormat = SCALAR;
        if(nChannels > 1) {
          dataFormat = IMAGE;
        }
        else if(nSamples > 1) {
          dataFormat = SPECTRUM;
        }

        // creating attribute property
        auto attributeProperty = std::make_shared<AttributeProperty>(
            attrName, nameSource, dataFormat, _dataType, processVariable->getDescription(), processVariable->getUnit());
        _descriptions.push_back(attributeProperty);
        _usedInputVariables.insert(attributeProperty->path);
      }
    }
  }

  /********************************************************************************************************************/
  void AttributeMapper::deriveType(std::type_info const& info) {
    if(info == typeid(uint8_t)) {
      _dataType = Tango::DEV_UCHAR;
    }

    if(info == typeid(int8_t)) {
      _dataType = Tango::DEV_ENUM;
    }

    if(info == typeid(uint16_t)) {
      _dataType = Tango::DEV_USHORT;
    }
    if(info == typeid(int16_t)) {
      _dataType = Tango::DEV_SHORT;
    }
    if(info == typeid(uint32_t)) {
      _dataType = Tango::DEV_ULONG;
    }
    if(info == typeid(int32_t)) {
      _dataType = Tango::DEV_LONG;
    }
    if(info == typeid(uint64_t)) {
      _dataType = Tango::DEV_ULONG64;
    }
    if(info == typeid(int64_t)) {
      _dataType = Tango::DEV_LONG64;
    }
    if(info == typeid(float)) {
      _dataType = Tango::DEV_FLOAT;
    }
    if(info == typeid(double)) {
      _dataType = Tango::DEV_DOUBLE;
    }
    if(info == typeid(std::string)) {
      _dataType = Tango::DEV_STRING;
    }
    if(info == typeid(ChimeraTK::Boolean)) {
      _dataType = Tango::DEV_BOOLEAN;
    }
    if(info == typeid(ChimeraTK::Void)) {
      _dataType = Tango::DEV_VOID;
    }

    // std::cout<<_dataType<<std::endl;
  }

} // namespace ChimeraTK

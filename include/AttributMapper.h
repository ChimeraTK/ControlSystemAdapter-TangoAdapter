#pragma once
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <iostream>
#include <list>
#include <set>
#include <AttributProperty.h>


namespace ChimeraTK {

  class AttributMapper {
   public:
    static AttributMapper& getInstance();
    AttributMapper(AttributMapper&) = delete;
    void operator=(AttributMapper const&) = delete;
    void directImport(std::set<std::string> inputVariables);    
    void prepareOutput(std::vector<std::string> attributList);    
    const std::set<std::string>& getUsedVariables() const { return _usedInputVariables; }
    // empty the created mapping
    void clear();
    std::list<std::shared_ptr<AttributProperty>> const& getAttDescList(void) const;
    void import(std::string importSource, std::string importLocationName, std::string directory="");
    void deriveType(std::type_info const& info) ;
    void setCSPVManager(boost::shared_ptr<ControlSystemPVManager> csPVManager){_controlSystemPVManager=csPVManager;}
    inline std::pair<std::string, std::string> splitStringAtFirstSlash(std::string input) {
    // find first slash
    auto slashPosition = input.find_first_of("/");
    if(slashPosition == 0) { // ignore leading slash
      input = input.substr(1);
      slashPosition = input.find_first_of("/");
    }
    // no slash found: return empty location name
    if(slashPosition == std::string::npos) {
      return std::make_pair(std::string(), input);
    }
    // split at first slash into location name and property name
    auto locationName = input.substr(0, slashPosition);
    auto propertyName = input.substr(slashPosition + 1);
    // replace any remaining slashes in property name with dots
    while((slashPosition = propertyName.find_first_of("/")) != std::string::npos) {
      propertyName[slashPosition] = '.';
    }
    return std::make_pair(locationName, propertyName);
  }
   protected:
    AttributMapper() = default;

    std::set<std::string> _inputVariables;
    std::set<std::string> _usedInputVariables; // For tracing which variables are
                                               // not to be imported.
    std::list<std::shared_ptr<AttributProperty>> _descriptions;
    boost::shared_ptr<ControlSystemPVManager> _controlSystemPVManager;
    long _dataType;    
  };


} // namespace ChimeraTK


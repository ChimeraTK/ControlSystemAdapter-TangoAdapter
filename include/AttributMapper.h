#pragma once
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>

#include <AttributProperty.h>
#include <iostream>
#include <list>
#include <set>

namespace ChimeraTK {

  class AttributMapper {
   public:
    static AttributMapper& getInstance();
    AttributMapper(AttributMapper&) = delete;
    void operator=(AttributMapper const&) = delete;
    void directImport(std::set<std::string>& inputVariables);
    void prepareOutput(std::vector<std::shared_ptr<ChimeraTK::AttributProperty>>& attributList);
    [[nodiscard]] const std::set<std::string>& getUsedVariables() const { return _usedInputVariables; }
    // empty the created mapping
    void clear();
    [[nodiscard]] std::list<std::shared_ptr<AttributProperty>> const& getAttDescList() const;
    void import(std::string importSource, const std::string& importLocationName, const std::string& directory = "");
    void deriveType(std::type_info const& info);
    void setCSPVManager(boost::shared_ptr<ControlSystemPVManager> csPVManager) {
      _controlSystemPVManager = std::move(csPVManager);
    }

   protected:
    AttributMapper() = default;

    std::set<std::string> _inputVariables;
    std::set<std::string> _usedInputVariables; // For tracing which variables are
                                               // not to be imported.
    std::list<std::shared_ptr<AttributProperty>> _descriptions;
    boost::shared_ptr<ControlSystemPVManager> _controlSystemPVManager;
    Tango::CmdArgType _dataType{Tango::CmdArgType::DATA_TYPE_UNKNOWN};
  };
} // namespace ChimeraTK

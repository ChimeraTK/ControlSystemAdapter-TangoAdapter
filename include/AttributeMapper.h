// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>

#include <libxml++/libxml++.h>

#include <AttributeProperty.h>

#include <list>
#include <set>

namespace TangoAdapter {

  class AttributeMapper {
   public:
    struct DeviceClass;

    struct DeviceInstance {
      DeviceInstance(std::string ourName, DeviceClass* parent) : name(std::move(ourName)), ourClass(parent) {}
      std::string name;
      std::map<std::string, std::string> attributeToSource;
      DeviceClass* ourClass;
    };

    /******************************************************************************************************************/

    struct DeviceClass {
      std::string name;
      std::optional<std::string> title;
      std::optional<std::string> description;

      std::list<AttributeProperty> attributes;
      std::map<std::string, std::shared_ptr<DeviceInstance>> devicesInDeviceClass;

      explicit DeviceClass(std::string ourName) : name(std::move(ourName)) {}

      bool hasDevice(const std::string& deviceName) {
        return devicesInDeviceClass.find(deviceName) != devicesInDeviceClass.end();
      }

      std::shared_ptr<DeviceInstance> getDevice(const std::string& deviceName);
    };

    /******************************************************************************************************************/

    AttributeMapper() = default;
    AttributeMapper(AttributeMapper&) = delete;
    void operator=(AttributeMapper const&) = delete;

    [[nodiscard]] std::set<std::string> getUnusedVariables();
    [[nodiscard]] std::list<std::shared_ptr<AttributeProperty>> const& getAttDescList(const std::string& device) const;

    void setCSPVManager(boost::shared_ptr<ChimeraTK::ControlSystemPVManager> csPVManager) {
      _controlSystemPVManager = std::move(csPVManager);
    }

    std::set<std::string> getCsVariableNames();
    std::list<std::string> getClasses();
    std::shared_ptr<DeviceClass> getClass(std::string& name) { return _classes[name]; }

    void readMapperFile();

    Tango::Attr* createAttribute(AttributeProperty& attr);

   private:
    void processDeviceClassNode(xmlpp::Node* classNode);
    void processDeviceInstanceNode(const std::shared_ptr<DeviceClass>&, xmlpp::Node* instanceNode);
    void processAttributeNode(std::shared_ptr<DeviceInstance> device, xmlpp::Node* node);
    void import(std::string importSource, std::shared_ptr<DeviceInstance> device);

    // For passing on to the class constructor in the class factory
    std::map<std::string, std::shared_ptr<DeviceClass>> _classes;

    std::set<std::string> _inputVariables;
    std::set<std::string> _usedInputVariables; // For tracing which variables are
                                               // not to be imported.
    std::map<std::string, std::list<std::shared_ptr<AttributeProperty>>> _descriptions;
    boost::shared_ptr<ChimeraTK::ControlSystemPVManager> _controlSystemPVManager;

    void addAttribute(std::shared_ptr<DeviceInstance>& device, const std::string& attrName,
        const std::string& processVariableName, std::optional<std::string> unit,
        const std::optional<std::string>& description);
  };

} // namespace TangoAdapter

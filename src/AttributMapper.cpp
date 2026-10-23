// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AttributeMapper.h"
#include "TangoAdapter.h"
#include "TangoLogCompat.h"

#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/Utilities.h>
#include <ChimeraTK/VoidRegisterAccessor.h>

#include <libxml++/libxml++.h>

#include <algorithm>
#include <memory>
#include <regex>

namespace TangoAdapter {

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

    /******************************************************************************************************************/

    /**
     * @brief Normalise a source name written in a mapper file to the process variable name form.
     *
     * A mapper file writes a source without a leading slash ("INT/TO_DEVICE_SCALAR"), while
     * ControlSystemPVManager::getCsVariableNames() returns the process variable names with a leading slash
     * ("/INT/TO_DEVICE_SCALAR"). ChimeraTK::RegisterPath accepts both forms and its string conversion returns the
     * canonical form, so a source is normalised to the canonical form before it enters _commandSources,
     * _usedInputVariables and the source maps. Without this the removal comparison between a mapper source and a
     * process variable name silently misses and the attribute/command exclusion never fires.
     */
    std::string normaliseSourceName(const std::string& source) {
      if(source.empty() || source == "/") {
        return source;
      }
      // The RegisterPath conversion returns the canonical name with a leading slash, whatever separator the
      // source uses.
      return std::string(ChimeraTK::RegisterPath(source));
    }

    /******************************************************************************************************************/

    bool nodeIsWhitespace(const xmlpp::Node* node) {
      const auto* nodeAsText = dynamic_cast<const xmlpp::TextNode*>(node);
      if(nodeAsText) {
        return nodeAsText->is_white_space();
      }
      return false;
    }

    /******************************************************************************************************************/

    static void iterateChildrenFiltered(
        xmlpp::Node* node, const std::function<void(xmlpp::Node*)>& functor,
        const std::function<bool(xmlpp::Node*)>& filter = [](xmlpp::Node*) { return true; }) {
      for(auto* child : node->get_children()) {
        if(util::nodeIsWhitespace(node)) {
          continue;
        }
        if(dynamic_cast<xmlpp::CommentNode const*>(node)) {
          continue;
        }

        if(!filter(child)) {
          continue;
        }

        functor(child);
      }
    }

    /******************************************************************************************************************/

    std::optional<std::string> childContentAsOptional(xmlpp::Node* node, const std::string& child) {
      std::optional<std::string> result;
      iterateChildrenFiltered(
          node,
          [&result](auto* n) {
            result = std::make_optional<std::string>();
            for(const auto* eguChild : n->get_children()) {
              if(util::nodeIsWhitespace(eguChild)) {
                continue;
              }
              const auto* nodeAsText = dynamic_cast<const xmlpp::TextNode*>(eguChild);
              if(nodeAsText == nullptr) {
                continue;
              }

              *result += nodeAsText->get_content();
            }
          },
          [&child](auto* n) { return n->get_name() == child; });

      return result;
    }

    /******************************************************************************************************************/

    bool parseBoolAttribute(xmlpp::Element* element, const std::string& name, bool& value) {
      auto* attribute = element->get_attribute(name);
      if(attribute == nullptr) {
        return false;
      }

      const auto& text = attribute->get_value();
      if(text == "true") {
        value = true;
      }
      else if(text == "false") {
        value = false;
      }
      else {
        throw ChimeraTK::logic_error("Invalid boolean value \"" + text + "\" for attribute \"" + name + "\" in line " +
            std::to_string(element->get_line()));
      }

      return true;
    }

    /******************************************************************************************************************/

    std::string deriveAttributeName(const std::string& pvName, const std::string& importSource = {}) {
      // processVariableName starts with wanted source
      auto nameSource = pvName.substr(importSource.size() + 1); // add the slash to be removed
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
      if(!locationName.empty()) {
        attrName = locationName + "_" + propertyName.getWithAltSeparator();
      }
      else {
        attrName = propertyName.getWithAltSeparator();
      }

      auto i = attrName.find('/');
      if(i != std::string::npos) {
        attrName.erase(i, 1);
      }

      return attrName;
    }

    /******************************************************************************************************************/

    Tango::CmdArgType deriveType(std::type_info const& info) {
      Tango::CmdArgType dataType{Tango::CmdArgType::DATA_TYPE_UNKNOWN};

      if(info == typeid(uint8_t)) {
        dataType = Tango::DEV_UCHAR;
      }

      if(info == typeid(int8_t)) {
        dataType = Tango::DEV_ENUM;
      }

      if(info == typeid(uint16_t)) {
        dataType = Tango::DEV_USHORT;
      }
      if(info == typeid(int16_t)) {
        dataType = Tango::DEV_SHORT;
      }
      if(info == typeid(uint32_t)) {
        dataType = Tango::DEV_ULONG;
      }
      if(info == typeid(int32_t)) {
        dataType = Tango::DEV_LONG;
      }
      if(info == typeid(uint64_t)) {
        dataType = Tango::DEV_ULONG64;
      }
      if(info == typeid(int64_t)) {
        dataType = Tango::DEV_LONG64;
      }
      if(info == typeid(float)) {
        dataType = Tango::DEV_FLOAT;
      }
      if(info == typeid(double)) {
        dataType = Tango::DEV_DOUBLE;
      }
      if(info == typeid(std::string)) {
        dataType = Tango::DEV_STRING;
      }
      if(info == typeid(ChimeraTK::Boolean)) {
        dataType = Tango::DEV_BOOLEAN;
      }
      if(info == typeid(ChimeraTK::Void)) {
        dataType = Tango::DEV_VOID;
      }

      return dataType;
    }

    /******************************************************************************************************************/

    Tango::CmdArgType deriveCommandType(const std::type_info& valueType, AttrDataFormat dataFormat) {
      // Tango has no command type for an image, whatever its value type is, so it is rejected before the value type is
      // looked at.
      if(dataFormat != AttrDataFormat::SCALAR && dataFormat != AttrDataFormat::SPECTRUM) {
        return Tango::DATA_TYPE_UNKNOWN;
      }

      // Tango has no command type for a scalar uint8_t or int8_t, and no array of int8_t (there is no signed char).
      // A 1D array of char means a uint8_t array, because ChimeraTK has no char process variable type.
      if(dataFormat == AttrDataFormat::SCALAR) {
        if(valueType == typeid(uint8_t) || valueType == typeid(int8_t)) {
          return Tango::DATA_TYPE_UNKNOWN;
        }

        return deriveType(valueType);
      }

      if(valueType == typeid(uint8_t)) {
        return Tango::DEVVAR_CHARARRAY;
      }
      if(valueType == typeid(ChimeraTK::Boolean)) {
        return Tango::DEVVAR_BOOLEANARRAY;
      }
      if(valueType == typeid(uint16_t)) {
        return Tango::DEVVAR_USHORTARRAY;
      }
      if(valueType == typeid(int16_t)) {
        return Tango::DEVVAR_SHORTARRAY;
      }
      if(valueType == typeid(uint32_t)) {
        return Tango::DEVVAR_ULONGARRAY;
      }
      if(valueType == typeid(int32_t)) {
        return Tango::DEVVAR_LONGARRAY;
      }
      if(valueType == typeid(uint64_t)) {
        return Tango::DEVVAR_ULONG64ARRAY;
      }
      if(valueType == typeid(int64_t)) {
        return Tango::DEVVAR_LONG64ARRAY;
      }
      if(valueType == typeid(float)) {
        return Tango::DEVVAR_FLOATARRAY;
      }
      if(valueType == typeid(double)) {
        return Tango::DEVVAR_DOUBLEARRAY;
      }
      if(valueType == typeid(std::string)) {
        return Tango::DEVVAR_STRINGARRAY;
      }

      return Tango::DATA_TYPE_UNKNOWN;
    }

    /******************************************************************************************************************/

    AttrDataFormat deriveDataFormat(const ChimeraTK::ProcessVariable::SharedPtr& processVariable) {
      size_t nSamples = 0;
      size_t nChannels = 0;

      ChimeraTK::callForType(processVariable->getValueType(), [&](auto t) {
        using T = decltype(t);
        boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv =
            boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<T>>(processVariable);
        nSamples = pv->getNumberOfSamples();
        nChannels = pv->getNumberOfChannels();
      });

      if(nChannels > 1) {
        return AttrDataFormat::IMAGE;
      }
      if(nSamples > 1) {
        return AttrDataFormat::SPECTRUM;
      }

      return AttrDataFormat::SCALAR;
    }

  } // namespace util

  /********************************************************************************************************************/
  /********************************************************************************************************************/
  /********************************************************************************************************************/

  std::list<std::string> AttributeMapper::getClasses() {
    std::list<std::string> keys;

    for(auto& [key, value] : _classes) {
      keys.push_back(key);
    }

    return keys;
  }

  /********************************************************************************************************************/

  void AttributeMapper::addAttribute(std::shared_ptr<DeviceInstance>& device, const std::string& attrName,
      std::string processVariableName, std::optional<std::string> unit, const std::optional<std::string>& description,
      EventingMode eventing) {
    // A mapper file writes a source without a leading slash while getCsVariableNames() returns the canonical name, so
    // the source is normalised here: the _commandSources and _usedInputVariables membership tests would otherwise miss
    // and the attribute/command exclusion never fire.
    processVariableName = util::normaliseSourceName(processVariableName);

    if(attrName == "Status" || attrName == "State") {
      throw ChimeraTK::logic_error("Reserved attribute name \"" + attrName + "\", derived from " + processVariableName +
          ". Modify your mapping");
    }

    // derive the datatype
    auto processVariable = _controlSystemPVManager->getProcessVariable(processVariableName);
    std::type_info const& valueType = processVariable->getValueType();

    auto tangoType = util::deriveType(valueType);

    auto dataFormat = util::deriveDataFormat(processVariable);

    size_t nSamples = 0;
    ChimeraTK::callForType(valueType, [&](auto t) {
      using T = decltype(t);
      boost::shared_ptr<ChimeraTK::NDRegisterAccessor<T>> pv =
          boost::dynamic_pointer_cast<ChimeraTK::NDRegisterAccessor<T>>(processVariable);
      nSamples = pv->getNumberOfSamples();
    });

    // A write-only ChimeraTK::Void source is the trigger of an auto-mapped command while the option is in force, so an
    // explicit <attribute> naming it has to be written as a <command> instead. The command sources of an explicit
    // <command> are rejected here as well, independent of the element order.
    if(_commandSources.find(processVariableName) != _commandSources.end() ||
        (device->autoMapCommandsFromVoid && valueType == typeid(ChimeraTK::Void) && processVariable->isWriteable() &&
            !processVariable->isReadable())) {
      throw ChimeraTK::logic_error("Process variable " + processVariableName +
          " is used by a command and cannot be "
          "mapped as attribute \"" +
          attrName + "\". Use a <command> element instead");
    }

    if(eventing != EventingMode::Off) {
      if(!processVariable->isReadable()) {
        throw ChimeraTK::logic_error("Eventing requested for attribute \"" + attrName +
            "\", but the process variable " + processVariableName + " is not readable");
      }
      if(dataFormat == AttrDataFormat::IMAGE) {
        throw ChimeraTK::logic_error(
            "Eventing requested for attribute \"" + attrName + "\", but image attributes do not support eventing");
      }
    }

    // creating attribute property
    device->attributeToSource[attrName] = processVariableName;

    device->ourClass->attributes.emplace_back(attrName, dataFormat, tangoType,
        description.value_or(processVariable->getDescription()), unit.value_or(processVariable->getUnit()));

    auto& attr = device->ourClass->attributes.back();
    attr.eventing = eventing;
    TANGO_LOG_DEBUG << "Adding attribute to class: " << attr << std::endl;

    if(processVariable->isWriteable() && processVariable->isReadable()) {
      attr.writeType = Tango::READ_WRITE;
    }
    else if(processVariable->isWriteable()) {
      attr.writeType = Tango::WRITE;
    }
    else {
      attr.writeType = Tango::READ;
    }
    attr.length = nSamples;

    TANGO_LOG_DEBUG << "Adding attribute "
                    << "\n"
                    << attr << std::endl;
    TANGO_LOG_DEBUG << "Adding " << processVariableName << " to used input variables" << std::endl;
    _usedInputVariables.insert(processVariableName);
  }

  /********************************************************************************************************************/

  // The role a command source plays, which decides whether it must be writeable or readable and whether a
  // ChimeraTK::Void value type is allowed.
  enum class CommandRole { Trigger, InputArgument, ReturnValue };

  static Tango::CmdArgType validateCommandSource(boost::shared_ptr<ChimeraTK::ControlSystemPVManager>& csPvManager,
      const std::string& commandName, const std::string& source, CommandRole role) {
    ChimeraTK::ProcessVariable::SharedPtr processVariable;
    try {
      processVariable = csPvManager->getProcessVariable(source);
    }
    catch(ChimeraTK::logic_error& e) {
      throw ChimeraTK::logic_error(
          std::format("Command \"{}\": source {} is not in the variable household of the application ({})", commandName,
              source, e.what()));
    }

    const auto& valueType = processVariable->getValueType();
    const bool isVoid = valueType == typeid(ChimeraTK::Void);

    switch(role) {
      case CommandRole::Trigger:
        // The trigger has no data, so it has to be a write-only ChimeraTK::Void process variable.
        if(!isVoid || !processVariable->isWriteable() || processVariable->isReadable()) {
          throw ChimeraTK::logic_error(std::format(
              "Command \"{}\": trigger {} must be a write-only ChimeraTK::Void process variable", commandName, source));
        }
        return Tango::DEV_VOID;

      case CommandRole::InputArgument: {
        if(isVoid) {
          throw ChimeraTK::logic_error(std::format(
              "Command \"{}\": inputArgument {} cannot be a ChimeraTK::Void process variable", commandName, source));
        }
        if(!processVariable->isWriteable()) {
          throw ChimeraTK::logic_error(
              std::format("Command \"{}\": inputArgument {} is not writeable", commandName, source));
        }
        break;
      }

      case CommandRole::ReturnValue: {
        if(isVoid) {
          throw ChimeraTK::logic_error(std::format(
              "Command \"{}\": returnValue {} cannot be a ChimeraTK::Void process variable", commandName, source));
        }
        if(!processVariable->isReadable()) {
          throw ChimeraTK::logic_error(
              std::format("Command \"{}\": returnValue {} is not readable", commandName, source));
        }
        break;
      }
    }

    auto tangoType = util::deriveCommandType(valueType, util::deriveDataFormat(processVariable));
    if(tangoType == Tango::DATA_TYPE_UNKNOWN) {
      throw ChimeraTK::logic_error(
          std::format("Command \"{}\": process variable {} has no Tango command type", commandName, source));
    }

    return tangoType;
  }

  /********************************************************************************************************************/

  void AttributeMapper::addCommand(std::shared_ptr<DeviceInstance>& device, const std::string& name,
      const CommandSources& sources, std::optional<std::string> inputDescription,
      std::optional<std::string> outputDescription) {
    if(name == "State" || name == "Status" || name == "Init") {
      throw ChimeraTK::logic_error("Reserved command name \"" + name + "\". Modify your mapping");
    }

    auto inType = Tango::DEV_VOID;
    if(sources.inputArgument) {
      inType = validateCommandSource(_controlSystemPVManager, name, *sources.inputArgument, CommandRole::InputArgument);
    }

    auto outType = Tango::DEV_VOID;
    if(sources.returnValue) {
      outType = validateCommandSource(_controlSystemPVManager, name, *sources.returnValue, CommandRole::ReturnValue);
    }

    if(sources.trigger) {
      validateCommandSource(_controlSystemPVManager, name, *sources.trigger, CommandRole::Trigger);
    }

    // A source shared between a command and an attribute is a clash; two attributes may keep mapping one source.
    for(const auto& source : {sources.trigger, sources.inputArgument, sources.returnValue}) {
      if(source && _usedInputVariables.find(*source) != _usedInputVariables.end()) {
        throw ChimeraTK::logic_error(
            "Process variable " + *source + " is used as an attribute and as command \"" + name + "\"");
      }
    }

    auto& command = device->ourClass->commands[name];
    if(command.name.empty()) {
      const auto inDesc = inputDescription.value_or(sources.inputArgument ?
              _controlSystemPVManager->getProcessVariable(*sources.inputArgument)->getDescription() :
              "");
      const auto outDesc = outputDescription.value_or(sources.returnValue ?
              _controlSystemPVManager->getProcessVariable(*sources.returnValue)->getDescription() :
              "");
      command = CommandProperty(name, inType, outType, inDesc, outDesc);
      TANGO_LOG_DEBUG << "Adding command to class: " << command << std::endl;
    }
    else if(command.inType != inType || command.outType != outType) {
      throw ChimeraTK::logic_error(
          "Command \"" + name + "\" has a different input or return type in the device instances of one class");
    }

    if(sources.trigger) {
      _commandSources.insert(*sources.trigger);
    }
    if(sources.inputArgument) {
      _commandSources.insert(*sources.inputArgument);
    }
    if(sources.returnValue) {
      _commandSources.insert(*sources.returnValue);
    }

    device->commands[name] = sources;
  }

  /********************************************************************************************************************/

  void AttributeMapper::processCommandNode(std::shared_ptr<DeviceInstance> device, xmlpp::Node* node) {
    auto* element = dynamic_cast<xmlpp::Element*>(node);
    if(element == nullptr) {
      throw ChimeraTK::logic_error(
          "Invalid argument: command node is not an Element in line " + std::to_string(node->get_line()));
    }

    for(const auto* attribute : element->get_attributes()) {
      const auto& name = attribute->get_name();
      if(name != "name" && name != "trigger" && name != "inputArgument" && name != "returnValue") {
        throw ChimeraTK::logic_error(
            "Unknown command attribute \"" + name + "\" in line " + std::to_string(element->get_line()));
      }
    }

    CommandSources sources;
    if(auto* attribute = element->get_attribute("trigger")) {
      sources.trigger = util::normaliseSourceName(attribute->get_value());
    }
    if(auto* attribute = element->get_attribute("inputArgument")) {
      sources.inputArgument = util::normaliseSourceName(attribute->get_value());
    }
    if(auto* attribute = element->get_attribute("returnValue")) {
      sources.returnValue = util::normaliseSourceName(attribute->get_value());
    }

    // A command needs a trigger or an argument, because both the name derivation and the action itself need a source.
    if(!sources.trigger && !sources.inputArgument) {
      throw ChimeraTK::logic_error(
          "Command with neither trigger nor inputArgument in line " + std::to_string(element->get_line()));
    }

    auto name = std::string(element->get_attribute_value("name"));
    if(name.empty()) {
      // The name defaults to the name derived from the trigger source, or from the inputArgument source when
      // there is no trigger. The two sources are tested separately: value_or() would evaluate and dereference
      // *sources.inputArgument even when the trigger is engaged, and abort on a trigger-only command without an
      // explicit name.
      name = util::deriveAttributeName(sources.trigger ? *sources.trigger : *sources.inputArgument);
    }

    addCommand(device, name, sources, util::childContentAsOptional(node, "inputDescription"),
        util::childContentAsOptional(node, "outputDescription"));
  }

  /********************************************************************************************************************/

  void AttributeMapper::processAttributeNode(std::shared_ptr<DeviceInstance> device, xmlpp::Node* node) {
    auto* element = dynamic_cast<xmlpp::Element*>(node);
    if(element == nullptr) {
      throw ChimeraTK::logic_error(
          "Invalid argument: DeviceInstance node is not an Element in line " + std::to_string(node->get_line()));
    }

    auto* source = element->get_attribute("source");
    if(source == nullptr) {
      throw ChimeraTK::logic_error(
          "Attribute node is missing the \"source\" attribute in line " + std::to_string(element->get_line()));
    }

    const auto sourceName = util::normaliseSourceName(source->get_value());

    auto name = std::string(element->get_attribute_value("name"));
    if(name.empty()) {
      name = util::deriveAttributeName(sourceName);
    }

    auto description = util::childContentAsOptional(node, "description");
    auto unit = util::childContentAsOptional(node, "egu");

    auto eventing = EventingMode::Off;
    if(auto eventingText = util::childContentAsOptional(node, "eventing")) {
      if(*eventingText == "dataReady") {
        eventing = EventingMode::DataReady;
      }
      else if(*eventingText == "change") {
        eventing = EventingMode::Change;
      }
      else if(*eventingText == "off") {
        eventing = EventingMode::Off;
      }
      else {
        throw ChimeraTK::logic_error("Unknown eventing value \"" + *eventingText + "\" for attribute \"" + name +
            "\" in line " + std::to_string(element->get_line()) +
            R"(. Valid values are "dataReady", "change" and "off")");
      }
    }

    addAttribute(device, name, sourceName, unit, description, eventing);
  }

  /********************************************************************************************************************/

  void AttributeMapper::processDeviceInstanceNode(
      const std::shared_ptr<DeviceClass>& deviceClass, xmlpp::Node* instanceNode) {
    assert(instanceNode != nullptr);

    auto* element = dynamic_cast<xmlpp::Element*>(instanceNode);
    if(element == nullptr) {
      throw ChimeraTK::logic_error("Invalid argument: DeviceInstance node is not an Element in line " +
          std::to_string(instanceNode->get_line()));
    }
    auto* attribute = element->get_attribute("name");
    if(attribute == nullptr) {
      throw ChimeraTK::logic_error(
          "DeviceInstance node is missing the name attribute in line " + std::to_string(element->get_line()));
    }

    auto device = deviceClass->getDevice(attribute->get_value());
    TANGO_LOG_DEBUG << "Creating new Instance with name " << device->name << std::endl;

    attribute = element->get_attribute("statusSource");
    if(attribute != nullptr) {
      device->statusSource = attribute->get_value();
      TANGO_LOG_DEBUG << std::format("Device status is fed from {}", attribute->get_value().c_str()) << std::endl;
    }

    attribute = element->get_attribute("stateSource");
    if(attribute != nullptr) {
      device->stateSource = attribute->get_value();
      TANGO_LOG_DEBUG << std::format("Device state is fed from {}", attribute->get_value().c_str()) << std::endl;
    }

    // The most specific level which carries autoMapCommandsFromVoid wins: this device instance, else the device class,
    // else the device server, else the default true.
    bool autoMap = deviceClass->autoMapCommandsFromVoid.value_or(_autoMapCommandsFromVoid.value_or(true));
    util::parseBoolAttribute(element, "autoMapCommandsFromVoid", autoMap);
    device->autoMapCommandsFromVoid = autoMap;

    util::iterateChildrenFiltered(instanceNode, [this, device](auto* node) {
      if(node->get_name() == "attribute") {
        processAttributeNode(device, node);
      }
      else if(node->get_name() == "command") {
        processCommandNode(device, node);
      }
      else if(node->get_name() == "import") {
        util::iterateChildrenFiltered(
            node,
            [this, device](auto* childNode) {
              auto const* textNode = dynamic_cast<const xmlpp::TextNode*>(childNode);
              import(textNode->get_content(), device);
            },
            [](auto* maybeTextNode) { return dynamic_cast<const xmlpp::TextNode*>(maybeTextNode) != nullptr; });
      }
    });
  }

  /********************************************************************************************************************/

  void AttributeMapper::processDeviceClassNode(xmlpp::Node* classNode) {
    auto* classElement = dynamic_cast<xmlpp::Element*>(classNode);
    if(classElement == nullptr) {
      throw ChimeraTK::logic_error("Invalid XML: DeviceClass node is not an Element");
    }

    auto* nameAttribute = classElement->get_attribute("name");
    if(nameAttribute == nullptr) {
      throw ChimeraTK::logic_error("Invalid XML: DeviceClass node is missing \"name\" attribute");
    }

    // Implicitly create new DeviceClass if not exists in map
    auto deviceClass = _classes[nameAttribute->get_value()];
    if(!deviceClass) {
      deviceClass = std::make_shared<DeviceClass>(nameAttribute->get_value());
      deviceClass->description = util::childContentAsOptional(classNode, "description");
      deviceClass->title = util::childContentAsOptional(classNode, "title");
      _classes[nameAttribute->get_value()] = deviceClass;
    }
    TANGO_LOG_DEBUG << "Creating new DeviceClass with name " << deviceClass->name << std::endl;

    if(bool autoMap = false; util::parseBoolAttribute(classElement, "autoMapCommandsFromVoid", autoMap)) {
      deviceClass->autoMapCommandsFromVoid = autoMap;
    }

    util::iterateChildrenFiltered(
        classNode, [this, deviceClass](auto* node) { processDeviceInstanceNode(deviceClass, node); },
        [](auto* node) { return node->get_name() == "deviceInstance"; });
  }

  /********************************************************************************************************************/

  void AttributeMapper::collectCommandSources(xmlpp::Node* rootNode) {
    util::iterateChildrenFiltered(
        rootNode,
        [this](auto* classNode) {
          util::iterateChildrenFiltered(
              classNode,
              [this](auto* instanceNode) {
                util::iterateChildrenFiltered(
                    instanceNode,
                    [this](auto* commandNode) {
                      auto* element = dynamic_cast<xmlpp::Element*>(commandNode);
                      if(element == nullptr) {
                        return;
                      }
                      for(const auto* attribute : element->get_attributes()) {
                        const auto& name = attribute->get_name();
                        if(name == "trigger" || name == "inputArgument" || name == "returnValue") {
                          _commandSources.insert(util::normaliseSourceName(attribute->get_value()));
                        }
                      }
                    },
                    [](auto* node) { return node->get_name() == "command"; });
              },
              [](auto* node) { return node->get_name() == "deviceInstance"; });
        },
        [](auto* node) { return node->get_name() == "deviceClass"; });
  }

  /********************************************************************************************************************/

  void AttributeMapper::readMapperFile() {
    std::string ourName{Tango::Util::instance()->get_ds_exec_name()};

    xmlpp::DomParser parser;
    auto fileName = ourName + "-AttributeMapper.xml";
    try {
      parser.parse_file(fileName);

      if(parser) {
        xmlpp::Node* rootNode = parser.get_document()->get_root_node();

        assert(rootNode->get_name() == "deviceServer");

        auto* rootElement = dynamic_cast<xmlpp::Element*>(rootNode);
        assert(rootElement != nullptr);
        if(bool autoMap = false; util::parseBoolAttribute(rootElement, "autoMapCommandsFromVoid", autoMap)) {
          _autoMapCommandsFromVoid = autoMap;
        }

        // The command sources of the whole file are known before the device classes and instances are processed, so an
        // <attribute> or <import> naming a command source is handled independent of the element order.
        collectCommandSources(rootNode);

        util::iterateChildrenFiltered(
            rootNode, [this](auto* node) { processDeviceClassNode(node); },
            [](auto* node) { return node->get_name() == "deviceClass"; });
      }

      TANGO_LOG_DEBUG << "Dump of mapper:" << std::endl;

      for(auto& [name, dclass] : _classes) {
        TANGO_LOG_DEBUG << "  > Device class: " << name << " " << dclass->name << std::endl;
        for(auto& [n, device] : dclass->devicesInDeviceClass) {
          TANGO_LOG_DEBUG << "    > Device instance: " << n << " " << device->name << std::endl;
          for(auto& [attr, src] : device->attributeToSource) {
            TANGO_LOG_DEBUG << "      > Attribute: " << attr << " -> " << src << std::endl;
          }
        }
      }
    }
    catch(xmlpp::exception& ex) {
      TANGO_LOG_INFO << "Failed to open " << fileName << " (" << ex.what()
                     << "). Will fall-back to auto-import of everything." << std::endl;

      std::regex start{"^ds_?"};
      std::regex end{"_?ds$"};

      ourName = std::regex_replace(ourName, start, "");
      ourName = std::regex_replace(ourName, end, "");

      TANGO_LOG_DEBUG << "Deriving class name from executable name: " << Tango::Util::instance()->get_ds_exec_name()
                      << " -> " << ourName << std::endl;

      auto deviceClass = std::make_shared<DeviceClass>(ourName);
      _classes[ourName] = deviceClass;
      auto deviceInstance = deviceClass->getDevice(TangoAdapter::PLAIN_IMPORT_DUMMY_DEVICE.data());
      import("/", deviceInstance);
    }
  }

  /********************************************************************************************************************/
  std::list<std::shared_ptr<AttributeProperty>> const& AttributeMapper::getAttDescList(
      const std::string& device) const {
    return _descriptions.at(device);
  }
  /********************************************************************************************************************/
  void AttributeMapper::import(std::string importSource, std::shared_ptr<DeviceInstance> device) {
    // a slash will be added after the source, so we make the source empty for an
    // import of everythingprocessVariableName
    if(importSource == "/") {
      importSource = "";
    }

    TANGO_LOG_DEBUG << "Importing " << importSource << " into " << device->name << std::endl;

    // loop source tree, cut beginning, replace / with _ and add a property
    for(auto const& processVariableName : getCsVariableNames()) {
      // A source already mapped into this device instance is skipped, so an explicit <attribute> or <command> before
      // the <import> is not mapped a second time. A source which only another device instance mapped stays available,
      // so every device instance imports the whole tree independently. The command sources of the file are skipped
      // always, as addAttribute() rejects them and import() turns a write-only ChimeraTK::Void source into a command.
      const auto alreadyMappedIntoInstance =
          std::any_of(device->attributeToSource.begin(), device->attributeToSource.end(),
              [&processVariableName](const auto& entry) { return entry.second == processVariableName; });
      if(alreadyMappedIntoInstance || _commandSources.find(processVariableName) != _commandSources.end()) {
        continue;
      }

      if(processVariableName.find(importSource + "/") == 0) {
        auto attrName = util::deriveAttributeName(processVariableName, importSource);

        // A write-only ChimeraTK::Void process variable becomes an argument-less, return-less command unless the
        // compatibility option on the device instance restores the boolean attribute of today.
        auto processVariable = _controlSystemPVManager->getProcessVariable(processVariableName);
        if(device->autoMapCommandsFromVoid && processVariable->getValueType() == typeid(ChimeraTK::Void) &&
            processVariable->isWriteable() && !processVariable->isReadable()) {
          CommandSources sources;
          sources.trigger = processVariableName;
          addCommand(device, attrName, sources);
          continue;
        }

        addAttribute(device, attrName, processVariableName, {}, {});
      }
    }
  }

  /********************************************************************************************************************/

  std::set<std::string> AttributeMapper::getCsVariableNames() {
    std::set<std::string> output;
    for(auto& pv : _controlSystemPVManager->getAllProcessVariables()) {
      output.insert(pv->getName());
    }

    return output;
  }

  /********************************************************************************************************************/

  std::shared_ptr<AttributeMapper::DeviceInstance> AttributeMapper::DeviceClass::getDevice(
      const std::string& deviceName) {
    try {
      return devicesInDeviceClass.at(deviceName);
    }
    catch(std::out_of_range&) {
    }

    auto device = std::make_shared<DeviceInstance>(deviceName, this);
    devicesInDeviceClass[deviceName] = device;

    return device;
  }

  /********************************************************************************************************************/

  std::set<std::string> AttributeMapper::getUnusedVariables() {
    std::set<std::string> unused;
    auto all = getCsVariableNames();

    // The state and status sources are not mapped as ordinary Tango attributes, but the application still publishes
    // them, so they must not be handed to optimiseUnmappedVariables() as unused. The command sources are handled the
    // same way.
    auto used = _usedInputVariables;
    used.insert(_commandSources.begin(), _commandSources.end());
    for(auto& [className, deviceClass] : _classes) {
      for(auto& [deviceName, device] : deviceClass->devicesInDeviceClass) {
        for(const auto& source : {device->stateSource, device->statusSource}) {
          if(source) {
            used.insert(source.value());
          }
        }
      }
    }

    std::set_difference(all.begin(), all.end(), used.begin(), used.end(), std::inserter(unused, unused.begin()));

    return unused;
  }
} // namespace TangoAdapter

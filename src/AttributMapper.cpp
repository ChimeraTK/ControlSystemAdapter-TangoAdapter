#include "AttributeMapper.h"
#include "TangoAdapter.h"
#include "TangoLogCompat.h"
#include <libxml++/libxml++.h>

#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/Utilities.h>
#include <ChimeraTK/VoidRegisterAccessor.h>

#include <memory>
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
    /******************************************************************************************************************/
    /********************************************************************************************************************/

    bool nodeIsWhitespace(const xmlpp::Node* node) {
      const auto* nodeAsText = dynamic_cast<const xmlpp::TextNode*>(node);
      if(nodeAsText) {
        return nodeAsText->is_white_space();
      }
      return false;
    }
    /******************************************************************************************************************/
    /********************************************************************************************************************/

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
    /********************************************************************************************************************/

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
    /********************************************************************************************************************/

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

  } // namespace util

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
      const std::string& processVariableName, std::optional<std::string> unit, std::optional<std::string> description) {
    // derive the datatype
    auto processVariable = _controlSystemPVManager->getProcessVariable(processVariableName);
    std::type_info const& valueType = processVariable->getValueType();

    auto tangoType = util::deriveType(valueType);

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
    device->attributeToSource[attrName] = processVariableName;

    device->ourClass->attributes.emplace_back(attrName, dataFormat, tangoType,
        description.value_or(processVariable->getDescription()), unit.value_or(processVariable->getUnit()));

    auto& attr = device->ourClass->attributes.back();
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

    auto name = std::string(element->get_attribute_value("name"));
    if(name.empty()) {
      name = util::deriveAttributeName(source->get_value());
    }

    auto description = util::childContentAsOptional(node, "description");
    auto unit = util::childContentAsOptional(node, "egu");
    addAttribute(device, name, source->get_value(), unit, description);
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
    util::iterateChildrenFiltered(instanceNode, [this, device](auto* node) {
      if(node->get_name() == "attribute") {
        processAttributeNode(device, node);
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

    util::iterateChildrenFiltered(
        classNode, [this, deviceClass](auto* node) { processDeviceInstanceNode(deviceClass, node); },
        [](auto* node) { return node->get_name() == "deviceInstance"; });
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
      if(_usedInputVariables.find(processVariableName) != _usedInputVariables.end()) {
        continue;
      }

      if(processVariableName.find(importSource + "/") == 0) {
        auto attrName = util::deriveAttributeName(processVariableName, importSource);
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
} // namespace ChimeraTK

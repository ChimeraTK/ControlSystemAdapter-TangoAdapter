// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoUpdater.h"

#include <unordered_set>

#include <ChimeraTK/ReadAnyGroup.h>

namespace ChimeraTK {


  void TangoUpdater::addVariable(
      TransferElementAbstractor variable, std::string attrId, AttrDataFormat attrDataFormat, long dataType) {

    INFO_STREAM<<" TangoUpdater::addVariable "<<attrId<<endl;

    if(variable.isReadable()) {
      auto id = variable.getId();
             //device, Attribut  read(device,attribut)
      if(_descriptorMap.find(variable.getId()) == _descriptorMap.end()) { //jade: push to _elementsToRead if not found in _descriptorMap
        _elementsToRead.push_back(variable);
      }
      else {
        _descriptorMap[variable.getId()].additionalTransferElements.insert(variable.getHighLevelImplElement());
      }
       //push variable.getHighLevelImplElement()/updaterFunction/eq_fct in _descriptorMap
       //and push TransferElementAbstractor _elementsToRead
      _descriptorMap[variable.getId()].attributID.push_back(attrId);
      _descriptorMap[variable.getId()].attributFormat.push_back(attrDataFormat);
      _descriptorMap[variable.getId()].dataType.push_back(dataType);

    }

  }

  void TangoUpdater::update() {
    for(auto& transferElem : _elementsToRead) {
      if(transferElem.readLatest()) {
          auto& descriptor = _descriptorMap[transferElem.getId()];
          updateFonction(descriptor.attributID[0],descriptor.attributFormat[0],descriptor.dataType[0]);
      }
    }
  }

  void TangoUpdater::updateFonction(const std::string attrName, const AttrDataFormat attrDataFormat, const long dataType) {

    vector<Tango::Attr *> &attr_vect = _device->get_device_class()->get_class_attr()->get_attr_list();
    Tango::Attribute &att = _device->get_device_attr()->get_attr_by_name(attrName.c_str());
    vector<Tango::Attr *>::iterator ite;

    long idx = att.get_attr_idx();

    DEBUG_STREAM<<"TangoUpdater::updateFonction  "<<attrName<<endl;

    switch (dataType){
        case Tango::DEV_UCHAR :
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<uint8_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<uint8_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_USHORT:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<uint16_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<uint16_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_SHORT:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<int16_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<int16_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_ULONG:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<uint32_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<uint32_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_LONG:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<int32_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<int32_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_DOUBLE:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<double>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<double>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_FLOAT:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<float>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<float>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_ULONG64:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<uint64_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<uint64_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_LONG64:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<int64_t>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<int64_t>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_STRING:
        {
          if (attrDataFormat==SCALAR)
          {
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<std::string>*>(attr_vect[idx]);
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<std::string>*>(attr_vect[idx]);
            spectrum_attr->read(_device,att);
          }
          break;
        }
        case Tango::DEV_BOOLEAN:
        {
          if (attrDataFormat==SCALAR)
          {
            //Attribut Tango does not accept ChimeraTK::Boolean type, cast to uint8_t
            auto* scalar_attr = dynamic_cast<ScalarAttribTempl<uint8_t>*>(attr_vect[idx]);
            scalar_attr->_dataType =  Tango::DEV_BOOLEAN;
            scalar_attr->read(_device,att);
          }
          else //SPECTRUM
          {
            auto* spectrum_attr = dynamic_cast<SpectrumAttribTempl<uint8_t>*>(attr_vect[idx]);
            spectrum_attr->_dataType =  Tango::DEV_BOOLEAN;
            spectrum_attr->read(_device,att);
          }
          break;
        }
    }
  }


  void TangoUpdater::updateLoop() {
    if(_elementsToRead.empty()) {
      return;
    }

    ReadAnyGroup group(_elementsToRead.begin(), _elementsToRead.end());

    // Call preRead for all TEs on additional transfer elements. waitAny() is doing this for all elements in the
    // ReadAnyGroup. Unnecessary calls to preRead() are anyway ignored and merely pose a performance issue. For large
    // servers, the performance impact is significant, hence we keep track of the TEs which need to be called.
    for(auto& pair : _descriptorMap) {
      for(auto& elem : pair.second.additionalTransferElements) {
        elem->preRead(ChimeraTK::TransferType::read); //TransferElement
      }
    }

    while(true) {
      // Wait until any variable got an update
      auto notification = group.waitAny(); //inside has a //handlePreRead TransferElement
      auto updatedElement = notification.getId();//1 ID 
      auto& descriptor = _descriptorMap[updatedElement]; //one descriptor

      // Complete the read transfer of the process variable
      notification.accept();

      // Call postRead for all TEs on _toDoocsAdditionalTransferElementsMap for the updated ID
      for(auto& elem : descriptor.additionalTransferElements) {
        elem->postRead(ChimeraTK::TransferType::read, true);
      }

      // Call all updater functions

      updateFonction(descriptor.attributID[0],descriptor.attributFormat[0],descriptor.dataType[0]);


      // Call preRead for all TEs on _toDoocsAdditionalTransferElementsMap for the updated ID
      for(auto& elem : descriptor.additionalTransferElements) {
        elem->preRead(ChimeraTK::TransferType::read);
      }

      // Allow shutting down this thread...
      boost::this_thread::interruption_point();
    }
  }

  void TangoUpdater::run() {
    _syncThread = boost::thread(boost::bind(&ChimeraTK::TangoUpdater::updateLoop, this));
  }

  void TangoUpdater::stop() {
    _syncThread.interrupt();
    for(auto& var : _elementsToRead) {
      var.getHighLevelImplElement()->interrupt();
    }
    _syncThread.join();
  }

  TangoUpdater::~TangoUpdater() {
    stop();
  }

} // namespace ChimeraTK

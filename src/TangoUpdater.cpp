// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AdapterDeviceClass.h"
#include "TangoLogCompat.h"
#include "TangoUpdater.h"

#include <ChimeraTK/ReadAnyGroup.h>

namespace ChimeraTK {

  void TangoUpdater::addVariable(TransferElementAbstractor variable, const std::string& attrId) {
    TANGO_LOG_DEBUG << "TangoAdapter::Updater adding variable " << attrId << std::endl;

    if(variable.isReadable()) {
      auto id = variable.getId();
      // device, Attribute  read(device,attribute)
      if(_descriptorMap.find(id) ==
          _descriptorMap.end()) { // jade: push to _elementsToRead if not found in _descriptorMap
        _elementsToRead.push_back(variable);
      }
      else {
        _descriptorMap[id].additionalTransferElements.insert(variable.getHighLevelImplElement());
      }
      // push variable.getHighLevelImplElement()/updaterFunction/eq_fct in _descriptorMap
      // and push TransferElementAbstractor _elementsToRead
      _descriptorMap[id].attributeID.push_back(attrId);
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
      for(const auto& elem : pair.second.additionalTransferElements) {
        elem->preRead(ChimeraTK::TransferType::read); // TransferElement
      }
    }

    while(true) {
      // Wait until any variable got an update
      auto notification = group.waitAny();               // inside has a //handlePreRead TransferElement
      auto updatedElement = notification.getId();        // 1 ID
      auto& descriptor = _descriptorMap[updatedElement]; // one descriptor

      // Complete the read transfer of the process variable
      notification.accept();

      // Call postRead for all TEs for the updated ID
      for(const auto& elem : descriptor.additionalTransferElements) {
        elem->postRead(ChimeraTK::TransferType::read, true);
      }

      // FIXME: Ideally we would fill the Tango buffer for the attribute here, then attribute->read()
      // would just send it out to CORBA
      // FIXME: Also we would need to toggle the event here, once supported

      // Call preRead for all TEs for the updated ID
      for(const auto& elem : descriptor.additionalTransferElements) {
        elem->preRead(ChimeraTK::TransferType::read);
      }

      // Allow shutting down this thread...
      boost::this_thread::interruption_point();
    }
  }

  void TangoUpdater::run() {
    _syncThread = boost::thread([&]() { updateLoop(); });
  }

  void TangoUpdater::stop() {
    try {
      _syncThread.interrupt();
      for(auto& var : _elementsToRead) {
        var.getHighLevelImplElement()->interrupt();
      }
      _syncThread.join();
    }
    catch(boost::thread_interrupted&) {
      // Ignore
    }
    catch(std::system_error& e) {
      TANGO_LOG_INFO << ::TangoAdapter::AdapterDeviceClass::getClassName()<<":Failed to shut down updater thread: " << e.what() << std::endl;
    }
  }

  // ChimeraTK::logic_error is theoretically thrown in ::interrupt(), practically should not happen and if it does,
  // we should have aborted anyway much sooner
  // NOLINTNEXTLINE(bugprone-exception-escape)
  TangoUpdater::~TangoUpdater() {
    stop();
  }

} // namespace ChimeraTK

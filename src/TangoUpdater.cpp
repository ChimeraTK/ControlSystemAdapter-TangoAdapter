// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "TangoUpdater.h"

#include "AdapterDeviceClass.h"
#include "TangoLogCompat.h"

#include <ChimeraTK/ReadAnyGroup.h>

namespace TangoAdapter {

  void TangoUpdater::addVariable(ChimeraTK::TransferElementAbstractor variable, const std::string& attrId,
      std::optional<std::function<void()>> callback) {
    TANGO_LOG_DEBUG << "TangoAdapter::Updater adding variable " << attrId << std::endl;

    if(variable.isReadable()) {
      auto id = variable.getId();

      if(_descriptorMap.find(id) == _descriptorMap.end()) {
        _elementsToRead.push_back(variable);
      }
      else {
        _descriptorMap[id].additionalTransferElements.insert(variable.getHighLevelImplElement());
      }

      _descriptorMap[id].attributeID.push_back(attrId);
      if(callback) {
        _descriptorMap[id].callbacks.push_back(callback.value());
      }
    }
  }

  /********************************************************************************************************************/

  void TangoUpdater::updateLoop() {
    if(_elementsToRead.empty()) {
      return;
    }

    ChimeraTK::ReadAnyGroup group(_elementsToRead.begin(), _elementsToRead.end());

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
      for(auto& f : descriptor.callbacks) {
        f();
      }

      // Call preRead for all TEs for the updated ID
      for(const auto& elem : descriptor.additionalTransferElements) {
        elem->preRead(ChimeraTK::TransferType::read);
      }

      // Allow shutting down this thread...
      boost::this_thread::interruption_point();
    }
  }

  /********************************************************************************************************************/

  void TangoUpdater::run() {
    _syncThread = boost::thread([&]() { updateLoop(); });
  }

  /********************************************************************************************************************/

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
      TANGO_LOG_INFO << ::TangoAdapter::AdapterDeviceClass::getClassName()
                     << ":Failed to shut down updater thread: " << e.what() << std::endl;
    }
  }

  /********************************************************************************************************************/

  // ChimeraTK::logic_error is theoretically thrown in ::interrupt(), practically should not happen and if it does,
  // we should have aborted anyway much sooner
  // NOLINTNEXTLINE(bugprone-exception-escape)
  TangoUpdater::~TangoUpdater() {
    stop();
  }

} // namespace TangoAdapter

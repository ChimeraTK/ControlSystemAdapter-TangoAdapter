
#pragma once

#include "AttributeProperty.h"
#include "ScalarAttribTempl.h"
#include "SpectrumAttribTempl.h"
#include <unordered_map>

#include <ChimeraTK/OneDRegisterAccessor.h>
#include <ChimeraTK/ScalarRegisterAccessor.h>
#include <ChimeraTK/TransferElement.h>
#include <ChimeraTK/TransferElementAbstractor.h>

#include <boost/noncopyable.hpp>

#include <map>
namespace ChimeraTK {

  /** A class to synchronise DeviceToControlSystem variable to Tango.
   *  It contains a list of TransferElements and a thread which is monitoring them
   * for updates. The thread has to be started with the run() functions, which
   * returns immediately when the thread is started, and (FIXME can be stopped by
   * the stop() function which returns after the thread has been joined). This
   * happens latest in the destructor.
   */
  class TangoUpdater : public boost::noncopyable, Tango::LogAdapter {
   public:
    explicit TangoUpdater(TANGO_BASE_CLASS* tangoDevice) : Tango::LogAdapter(tangoDevice), _device(tangoDevice) {}

    // ChimeraTK::logic_error is theoretically thrown in ::interrupt(), practically should not happen and if it does,
    // we should have aborted anyway much sooner
    // NOLINTNEXTLINE(bugprone-exception-escape)
    ~TangoUpdater() override;

    void updateLoop(); // Endless loop with interruption point around the update
                       // function.

    void run();
    void stop();

    // Add a variable to be updated. Together with the TransferElementAbstractor
    // pointing to the ChimeraTK::ProcessArray, the EqFct* to obtain the lock for
    // and a function to be called which executes the actual update should be
    // specified. The lock is held while the updaterFunction is called, so it must
    // neither obtained nor freed within the updaterFunction.
    void addVariable(ChimeraTK::TransferElementAbstractor variable, const std::string& attrId);

    const std::list<ChimeraTK::TransferElementAbstractor>& getElementsToRead() { return _elementsToRead; }

   protected:
    std::list<ChimeraTK::TransferElementAbstractor> _elementsToRead;
    boost::thread _syncThread; // we have to use boost thread to use interruption points

    // Struct used to aggregate the information needed in the updateLoop when an update is received from the
    // application.
    struct UpdateDescriptor {
      std::vector<std::string> attributeID;
      std::set<boost::shared_ptr<ChimeraTK::TransferElement>> additionalTransferElements;
    };
    std::map<ChimeraTK::TransferElementID, UpdateDescriptor> _descriptorMap;
    TANGO_BASE_CLASS* _device;
  };
} // namespace ChimeraTK

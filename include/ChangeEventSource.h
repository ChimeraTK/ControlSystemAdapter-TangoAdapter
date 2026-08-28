// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/TransferElementAbstractor.h>

namespace Tango {
  class DeviceImpl;
}

namespace TangoAdapter {

  /// Interface for Tango attributes that maintain an internal cache of the underlying process-variable value
  /// and can push change ("data") events from the updater thread.
  ///
  /// The concrete *AttribTempl specialisations know the TangoType<->AdapterType mapping. The cache decouples
  /// Tango's read() (and the change-event push) from the live ChimeraTK accessor buffer: updateFromPv() must be
  /// called on the updater thread, i.e. the only thread that touches the accessor's data buffer. read() and
  /// pushChangeEvent() then serve the coherent cached value instead of reading the accessor concurrently.
  class ChangeEventSource {
   public:
    virtual ~ChangeEventSource() = default;

    /// Refresh the internal cache from the (type-erased) accessor for this attribute.
    /// MUST be called on the updater thread, after the accessor's buffer has been populated (single-threaded
    /// access to the accessor's data). Takes the attribute's own mutex.
    virtual void updateFromPv(ChimeraTK::TransferElementAbstractor pv) = 0;

    /// Push a change event carrying the currently cached value.
    /// @param dev   The device owning the attribute (used to push the event).
    virtual void pushChangeEvent(Tango::DeviceImpl* dev) = 0;
  };

} // namespace TangoAdapter

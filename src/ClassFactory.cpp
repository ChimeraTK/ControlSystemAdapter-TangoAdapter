// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ClassFactory.h"

#include "AdapterDeviceClass.h"
#include "AttributeMapper.h"
#include "TangoAdapter.h"

#if TANGO_VERSION >= TANGO_MAKE_VERSION(10, 3, 0)
namespace TangoAdapter {
  class DServerImpl : public Tango::DServer {
   public:
    DServerImpl(Tango::DeviceClass* classPointer, const std::string& name, const std::string& description,
        Tango::DevState deviceState, const std::string& status)
    : DServer(classPointer, name.c_str(), description.c_str(), deviceState, status.c_str()) {}

   private:
    void class_factory() override {
      // Please keep this code here and below in sync necessary
      auto& mapper = TangoAdapter::getInstance().getMapper();
      for(auto& className : mapper.getClasses()) {
        add_class(std::make_unique<AdapterDeviceClass>(className).release());
      }
    }
  };

  Tango::DServer* constructor(Tango::DeviceClass* cl_ptr, const std::string& name, const std::string& desc,
      Tango::DevState state, const std::string& status) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new DServerImpl(cl_ptr, name, desc, state, status);
  }
} // namespace TangoAdapter
#else
// Factory function required from Tango.
void Tango::DServer::class_factory() {
  // Please keep this code here and above in sync necessary
  auto& mapper = TangoAdapter::TangoAdapter::getInstance().getMapper();
  for(auto& className : mapper.getClasses()) {
    add_class(std::make_unique<TangoAdapter::AdapterDeviceClass>(className).release());
  }
}
#endif

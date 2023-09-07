#include "MyApplication.h"

#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>

static ChimeraTK::ApplicationFactory<ExampleApp> theExampleAppFactory;

ExampleApp::ExampleApp() : Application("TestChimeraTK") {
    if(config.get<int>("enableAutomation")) {
      automation = Automation(this, "Automation", "Slow setpoint ramping algorithm");
    }
  }
 

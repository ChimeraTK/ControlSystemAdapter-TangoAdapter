
#pragma once

#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
#include <ChimeraTK/ApplicationCore/ConfigReader.h>
#include <ChimeraTK/ApplicationCore/EnableXMLGenerator.h>
#include <ChimeraTK/ApplicationCore/PeriodicTrigger.h>
#include <ChimeraTK/ApplicationCore/StatusMonitor.h>

namespace ctk = ChimeraTK;

//example 2a
struct Controller : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<float> sp{this, "temperatureSetpoint", "degC", "Description"};
  ctk::ScalarPushInput<float> rb{this, "/heater/temperatureReadback", "degC", "..."};
  ctk::ScalarOutput<float> cur{this, "/heater/heatingCurrent", "mA", "..."};
  ctk::ArrayPushInput<float> voltage{this, "/heater/supplyVoltages", "V", 4,"..."};
  //test boolean
  ctk::ScalarPollInput<ChimeraTK::Boolean> bool_scalar{this, "/heater/bool_scalar","", "..."};
  ctk::ArrayOutput<ChimeraTK::Boolean> bool_arr{this, "/heater/bool_array","",2, "..."};
  //ctk::ArrayPushInput<ChimeraTK::Boolean> bool_arr{this, "/heater/bool_array","",2, "..."};
  //ctk::ScalarOutput<std::string> text{this, "/heater/SCALAR","", "..."};//

  void mainLoop() {
    const float gain = 100.0;
    while(true) {

      readAll(); // waits until rb updated, then reads sp
      cur = gain * (sp - rb);
      writeAll(); // writes any outputs
    }
  }
};

struct Automation : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<float> opSp{this, "operatorSetpoint", "degC", "..."};
  ctk::ScalarOutput<float> actSp{this, "/Controller/temperatureSetpoint", "degC", "..."};
  ctk::ScalarPushInput<uint64_t> trigger{this, "/Timer/tick", "", "..."};

  void mainLoop() {
    const float maxStep = 0.1F;
    while(true) {

      readAll(); // waits until trigger received, then read opSp
      actSp += std::max(std::min(opSp - actSp, maxStep), -maxStep);
      writeAll();
    }
  }
};

struct ExampleApp final : public ctk::Application {
  ExampleApp() : Application("TestChimeraTK") {
    if(config.get<int>("enableAutomation")) {
      automation = Automation(this, "Automation", "Slow setpoint ramping algorithm");
    }
  }
  ~ExampleApp() final { shutdown(); }


  ctk::SetDMapFilePath dmapPath{"example1.dmap"};
  ctk::ConfigReader config{this, "config", "example1.xml"};
  Controller controller{this, "Controller", "The Controller"};
  Automation automation;

  ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for the controller", 1000};

  ctk::DeviceModule oven{this, "oven", "/Timer/tick"};

};


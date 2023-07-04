#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

#include <ChimeraTK/ApplicationCore/ApplicationCore.h>
#include <ChimeraTK/ApplicationCore/ConfigReader.h>
#include <ChimeraTK/ApplicationCore/EnableXMLGenerator.h>
#include <ChimeraTK/ApplicationCore/PeriodicTrigger.h>


namespace ctk = ChimeraTK;

//example 2a
struct Controller : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<double> sp{this, "temperatureSetpoint", "degC", "Description", {"CS"}};
  ctk::ScalarPushInput<double> rb{this, "temperatureReadback", "degC", "...", {"DEV", "CS"}};
  ctk::ScalarOutput<double> cur{this, "heatingCurrent", "mA", "...", {"DEV","CS"}}; //tag CS to connect to CS
  ctk::ArrayOutput<double> voltage{this, "supplyVoltages", "V", 4,"...", {"DEV","CS"}};//  

  void mainLoop() {
    const double gain = 100.0;
    while(true) {
      
      readAll(); // waits until rb updated, then reads sp

      cur = gain * (sp - rb);
      //this->logger->sendMessage("Test",logging::LogLevel::DEBUG);
      std::cout<<" -Controller heatingCurrent = "<<cur<<" sp="<<sp <<" temperatureReadback = "<<rb;
      writeAll(); // writes any outputs
    }
  }
};

struct Automation : public ctk::ApplicationModule {
  using ctk::ApplicationModule::ApplicationModule;
  ctk::ScalarPollInput<double> opSp{this, "operatorSetpoint", "degC", "...", {"CS"}}; //W
  ctk::ScalarOutput<double> actSp{this, "temperatureSetpoint", "degC", "...", {"Controller"}}; // connect to sp de controller
  //ctk::ScalarPushInput<uint64_t> trigger{this, "trigger", "", "..."};
  ctk::ScalarPushInput<uint64_t> trigger{this, "/Timer/tick", "", "..."};
  void mainLoop() {
    const double maxStep = 0.1;
    while(true) {

      readAll(); // waits until trigger received, then read opSp
      actSp += std::max(std::min(opSp - actSp, maxStep), -maxStep);
      std::cout<<" -Automation operatorSetpoint ="<<opSp<<" temperatureSetpoint ="<<actSp<<std::endl;
      writeAll();
    }
  }
};

struct ExampleApp : public ctk::Application {
  ExampleApp() : Application("TestChimeraTK") {
    if(config.get<int>("enableAutomation")) {
      automation = Automation(this, "Automation", "Slow setpoint ramping algorithm");
      // automation.findTag("Controller").connectTo(controller);
      // timer.tick >> automation.trigger;
    }
  }
  ~ExampleApp() { shutdown(); }


  ctk::SetDMapFilePath dmapPath{"example1.dmap"};
  ctk::ConfigReader config{this, "config", "example1.xml"};
  Controller controller{this, "Controller", "The Controller"};
  Automation automation;

  ctk::PeriodicTrigger timer{this, "Timer", "Periodic timer for the controller", 1000};

  ctk::DeviceModule oven{this, "oven", "/Timer/tick"};
  //void defineConnections();

};

/*
void ExampleApp::defineConnections() {



    automation = Automation(this, "Automation", "Slow setpoint ramping algorithm");
    automation.findTag("Controller").connectTo(controller);
    timer.tick >> automation.trigger;


  controller.findTag("DEV").connectTo(oven["heater"], timer.tick); // tag DEV envoyer to device
  findTag("CS").connectTo(cs); //tag CS pour envoyer à CS

  dump();
  std::cout<<"===========================================cs.dump============================================"<<std::endl;
  // show how it looks on the cs side (virtual hierarchy)
  cs.dump();
  std::cout<<"=========================================dumpConnections======================================"<<std::endl;
  // show how it is connected
  dumpConnections();

}*/

#endif
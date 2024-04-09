#include <ChimeraTK/ControlSystemAdapter/ApplicationFactory.h>
#include <ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h>

static ChimeraTK::ApplicationFactory<ReferenceTestApplication>theFactory{"testVariableExistence"};

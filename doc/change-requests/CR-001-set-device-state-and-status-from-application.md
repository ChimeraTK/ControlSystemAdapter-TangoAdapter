# CR-001: Set the Tango device state and status from the application

Synopsis: Let the application drive the Tango device state and status of a
device instance by naming a process variable for each of them in the
AttributeMapper configuration, so the device no longer has to report the fixed
defaults of the adapter. Only values which the application has written and
marked as valid are applied, so a client never sees a state or a status which
is only present because the source has not been filled in or because its data
is invalid.

Status: DONE

## Requirements

Aspect: configuration.

- A `deviceInstance` in `<applicationName>-AttributeMapper.xml` accepts the
optional attributes `stateSource` and `statusSource`.
- Without them a device instance behaves as today, i.e. the adapter defines
the state and the status.

Aspect: application driven device state.

- `stateSource` names a readable, integral process variable.
- The value is mapped to the Tango device state: 0 becomes ON, 1 becomes
FAULT, 2 becomes OFF and 3 becomes ALARM. Those are the values of
`ChimeraTK::StatusAccessorBase::Status` (OK, FAULT, OFF, WARNING).
- Any other value results in state UNKNOWN, and a warning is logged.

Aspect: application driven device status.

- `statusSource` names a readable string process variable, whose value becomes
the Tango device status.

Aspect: invalid configuration.

- A source which is not in the variable household of the application, or which
is not readable, aborts the start-up with a `ChimeraTK::logic_error` naming
the process variable and the purpose it was configured for.
- A `stateSource` whose value type is not integral aborts the start-up in the
same way.
- A `statusSource` whose value type is not a string aborts the start-up in the
same way.

Aspect: update and start-up behaviour.

- State and status follow the source whenever the application publishes a
valid value.
- A value which the application has not written is never applied. Until the
first valid value arrives, a device with a source reports the state ON and
the status "Application is running." which `AdapterDeviceClass` sets, so an
initialisation value of the source never reaches a client.
- A value with faulty data validity is never applied either. The device keeps
the last valid state and status, so a client never sees a value which the
application has marked as invalid.
- The value which the application has already published and marked as valid
when the device is created is applied, so a device with a source does not
stay in the defaults although the application reports something else.
- Those defaults remain the fallback for device instances without a
`stateSource` or `statusSource`.

Aspect: configuration schema and documentation.

- The XML schema declares both attributes.
- The documentation describes the attributes, the state mapping, and the
default behaviour without a source.

## Specifications

Affected components: `include/AttributeMapper.h`, `src/AttributMapper.cpp`,
`include/AdapterDeviceImpl.h`, `src/AdapterDeviceImpl.cpp`,
`xmlschema/tango_variable_tree.xsd`, `doc/mainpage.dox` and
`doc/example-AttributeMapper.xml`. Test assets:
`tests/serverTestStatusState_ds-AttributeMapper.xml`,
`tests/testServerStatusState_template.db` and
`tests/src/testTangoServerStatusState.cpp`.

Aspect: configuration.

- `AttributeMapper::DeviceInstance` gains
`std::optional<std::string> statusSource` and `stateSource`.
- `AttributeMapper::processDeviceInstanceNode()` reads both attributes with
`element->get_attribute()` and stores the value, logging it with
`TANGO_LOG_DEBUG`. A missing attribute leaves the optional empty.
- The sources are stored on the device instance next to, but separate from,
the attribute mapping, so a process variable can still be mapped as an
ordinary Tango attribute.

Aspect: registering the sources.

- `AdapterDeviceImpl::attachToClassAttributes()` calls the new private helper
`addMonitoredDeviceProperty()` once for the state and once for the status.
This happens while the device is created in
`AdapterDeviceClass::device_factory()`, alongside the other attributes.
- `addMonitoredDeviceProperty()` returns immediately for an unset source.
- Otherwise it takes the process variable from
`TangoAdapter::getInstance().getCsPvManager()` and throws a
`ChimeraTK::logic_error` naming the process variable and the purpose when it
is not readable or not in the variable household of the application. The
lookup itself throws a `ChimeraTK::logic_error` naming only the process
variable for a source which is not in the household, so the helper catches
that error and rethrows it with the purpose added.
- A source is not an unused variable: `AttributeMapper::getUnusedVariables()`
keeps both source names out of the set it returns, so
`TangoAdapter::finalizeApplicationStartup()` does not pass them to
`ApplicationBase::optimiseUnmappedVariables()` (src/TangoAdapter.cpp:126).
The application therefore keeps publishing a source which is not mapped as an
ordinary Tango attribute.
- It registers the process variable with
`TangoAdapter::getInstance().getUpdater().addVariable()` under the name
`__ChimeraTK_toTangoState` or `__ChimeraTK_toTangoStatus`, which the updater
uses for logging and error messages.
- The registration also checks the value type of the source, integral for the
state and string for the status. A mismatch throws the same
`ChimeraTK::logic_error`, so a misconfigured device instance fails the
start-up instead of failing later on the updater thread.
- The registration passes a callback which the updater invokes after each
read, on the updater thread. The callback returns while the element is not
initialised and for a value which is dropped as described below, and
otherwise takes a `Tango::AutoTangoMonitor` on the device before calling the
mapping lambda, so the state and the status are only written under the
device lock.
- The updater keys its descriptors by the process variable id, so a source
shared by several device instances is read once and every device instance
still receives its own callback.

Aspect: mapping.

- Both mapping lambdas branch on the value type with
`ChimeraTK::callForTypeNoVoid()` and obtain the typed accessor through
`getHighLevelImplElement()`, because the sources are ordinary process
variables and not accessors created by the adapter.
- The state lambda switches on `accessData(0)` for integral types as described
in Requirements, and logs an unsupported value with `TANGO_LOG_WARN`.
- The status lambda calls `set_status()` with `accessData(0)` of the string.
- Both lambdas only use element 0, so the remaining elements of an array-valued
source are ignored.
- After the registration-time check, only the branch matching the value type
of the source can be taken.

Aspect: values which are dropped.

- The callback compares `pv.getVersionNumber()` with
`ChimeraTK::VersionNumber(nullptr)` and returns without mapping the value
when the two are equal. That version number means that the source has never
carried a value, so the buffer only holds an initialisation value which the
application has not published. `TangoAdapter::run()` uses the same criterion
to find the to-application variables which it has to initialise itself.
- The callback checks `pv.dataValidity()` and returns without mapping the value
unless it is `ChimeraTK::DataValidity::ok`.
- Dropping a value leaves the state and the status untouched, so a device
reports the defaults of `AdapterDeviceClass` until the first valid value
arrives and the last valid value afterwards.
- Both checks happen before the device lock is taken and neither of them logs
a warning: a source which has not been written or which the application has
marked as invalid is not a configuration error.

Aspect: start-up.

- `TangoAdapter::run()` creates the devices during `server_init()` and starts
the application and the updater afterwards in
`finalizeApplicationStartup()`. The updater reads every source and applies
the value which the application has published and marked as valid at that
moment, so no separate initialisation step is needed.
- `AdapterDeviceClass::device_factory()` keeps setting ON and "Application is
running." for a device whose state is still `Tango::UNKNOWN`. A device with a
`stateSource` reports those defaults as well, because every device is still
`Tango::UNKNOWN` when `device_factory()` runs; the updater which applies the
source values starts only afterwards in `finalizeApplicationStartup()`. The
application has already published the from-device variables in `initialise()`,
which runs before `server_init()`.

Aspect: schema and documentation.

- `xmlschema/tango_variable_tree.xsd` declares `stateSource` and
`statusSource` as optional `xs:string` attributes of `DeviceInstanceType`.
The schema is not consumed by the build or the tests, it is an editor aid
kept in the repository.
- `doc/mainpage.dox` gains a subsection describing both attributes, the state
mapping, and that the defaults only apply without a source.
- `doc/example-AttributeMapper.xml` shows both attributes on a device
instance.

### Alternatives considered

- Discarding the first value read after start-up, as an earlier version did
with a per-lambda `static bool firstRun` flag: rejected. That drops the
value which the application has already published when the device is created
although it may be valid, so the device would report ON and "Application is
running." although the application reports something else. Checking the
version number and the data validity of every value keeps the published
initial value and drops only values which are not fit to be shown.
- Leaving the value type check to the mapping lambda, which is where the code
currently throws: rejected. The device server has started at that point, so
the error would surface on the updater thread instead of aborting the
start-up.
- Applying a value with faulty data validity and leaving it to the client to
check the validity of the corresponding attribute: rejected. The device
state and status are what a client looks at first, and a client which only
looks at them would act on a value which the application has marked as
invalid.
- Reporting state UNKNOWN instead of the adapter defaults while the
application has not published a valid value: rejected. ON and "Application
is running." are the defaults which `AdapterDeviceClass` already defines for
a device instance, and they are easier to use than a state which the client
cannot tell apart from the UNKNOWN of an unsupported source value.

## Test plan

- `testTangoServerStatusState.cpp` (module `serverTestStatusState`, offline
database `testServerStatusState_template.db`, configuration
`tests/serverTestStatusState_ds-AttributeMapper.xml` with
`stateSource="INT/FROM_DEVICE_SCALAR"` and
`statusSource="STRING/FROM_DEVICE_SCALAR"`), driven through the manual loop
control of the fixture.
- `testState`: the device reports ON at start-up, from the value which the
reference application has already published and marked as valid. Writing 12
to `INT/TO_DEVICE_SCALAR` yields UNKNOWN, then 0, 1, 2 and 3 yield ON,
FAULT, OFF and ALARM.
- `testStatus`: the device reports the empty string published at start-up.
After writing `TEST_STRING_1` to `STRING_TO_DEVICE_SCALAR` the status is
that string.
- `testFaultyValidity`: the test sets the `dataValidity` member of
`ReferenceTestApplication` to `ChimeraTK::DataValidity::faulty`, runs one
main loop iteration and asserts that the device keeps the state and the
status it reported before. Setting the validity back to ok and running
another iteration applies the value which was published in the meantime.
- Start-up validation: a device server whose `stateSource` is not integral,
or whose `statusSource` is not a string, fails to start with a
`ChimeraTK::logic_error` naming the process variable and the purpose. The
assertion is verified by inspection: a second device server cannot be started
in the test process, because `Tango::Util` and the application instance are
already created by the first one, so a fixture cannot observe the failure.
- A source which the application has never written is verified by inspection:
`ReferenceTestApplication` publishes every from-device variable in its
`initialise()`, so the existing test assets cannot produce a source which has
never carried a value. The criterion is the one which `TangoAdapter::run()`
uses for the to-application variables it initialises.
- All tests poll with a timeout, because the update happens on the updater
thread.
- The full `ctest` run of the project, including the `coding_style` test.

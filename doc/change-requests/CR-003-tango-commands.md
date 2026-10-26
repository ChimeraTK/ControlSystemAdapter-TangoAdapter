# CR-003: Map process variables to Tango commands

Synopsis: Let the AttributeMapper map process variables to Tango commands with
one optional input argument and one optional return value. A write-only
`ChimeraTK::Void` process variable which the mapper imports becomes an
argument-less command without a return value by default, instead of the boolean
attribute of today.

Depends on: CR-002. CR-003 sits on the tip of the CR-002 branch
(`jgeorg-agent/CR-002-tango-eventing`): its test plan relies on the subprocess
and reject-mapper test infrastructure CR-002 introduces, and both change
requests edit the same files. Next to its own files CR-003 also changes three
CR-002 artefacts: `tests/src/testTangoServerEventing.cc` (uses the shared
`runTestTangoServer` helper), `tests/include_lib/TangoTestServer.h` (declares
it) and `tests/serverTestEventing_ds-AttributeMapper.xml` (needs
`autoMapCommandsFromVoid="false"`). CR-003's change set carries no other CR-002
artefact.

Status: READY TO IMPLEMENT (from TESTS REVIEWED)

## Requirements

Aspect: commands.

- A device instance declares commands in the mapper file with a new `<command>`
element.
- A command may take one input argument and may return one value.
- A write-only `ChimeraTK::Void` process variable which the mapper imports
becomes an argument-less command without a return value.
- A readable `ChimeraTK::Void` process variable still becomes an attribute.
- An explicit `<attribute>` naming a write-only `ChimeraTK::Void` process
variable is rejected when the mapper file is read; the user has to write
`<command>` instead.

Aspect: configuration of a command.

- `<command>` carries the optional XML attributes `name`, `trigger`,
`inputArgument` and `returnValue`, and the optional child elements
`inputDescription` and `outputDescription`.
- `trigger` names a write-only `ChimeraTK::Void` process variable; a command
may omit it.
- `inputArgument` names a writeable process variable, `returnValue` a readable
one.
- A command needs at least one of `trigger` and `inputArgument`.
- `name` defaults to the name derived from the `trigger` source, or from the
`inputArgument` source when there is no trigger.
- `inputDescription` and `outputDescription` default to the description of the
process variable the argument and the return value come from.

Aspect: data types of argument and return value.

- The input type follows from the value type and the data format of the
`inputArgument` process variable, the return type from those of the
`returnValue` process variable.
- Without `inputArgument` the command takes no argument, without `returnValue`
it returns nothing.
- The Tango command types are the scalars void, boolean, short, long, long64,
float, double, string, unsigned short, unsigned long and unsigned long64, and
the 1D arrays of char, boolean, short, long, long64, float, double, unsigned
short, unsigned long, unsigned long64 and string.
- A 1D array of char means a `uint8_t` array, because ChimeraTK has no `char`
process variable type.
- A scalar `uint8_t` or `int8_t` process variable, an array of `int8_t`, and an
image are rejected when the mapper file is read, because Tango does not offer a
command type for them.

Aspect: execution of a command.

- A call writes the input argument to the process variable of `inputArgument`.
- It writes the process variable of `trigger` when the command has one; without
a `trigger` the input argument write is the trigger.
- The call blocks until the process variable of `returnValue` carries data
written after the command's own write, and returns its value.
- The answer has to post-date the version number of the argument and trigger
write, so an unrelated update of the return-value process variable which
pre-dates the command's own write, for example a refresh from another source of
the application, does not end the wait. Accepted limitation: an update which
post-dates the write ends the wait whatever its source, so an application which
refreshes the return-value process variable from an unrelated source can make
the call return that refresh's value.
- A return value whose process variable does not carry valid data fails the
call.
- The call waits forever: there is no timeout.
- Accepted limitation of the blocking design: while a command waits for its
return value, a new state or status which the application publishes can block
the adapter's updater thread on the device monitor the waiting call holds, so
the call then never returns. This is a known and accepted limitation, not a
defect to be fixed in this change request.
- Accepted limitation of the version number comparison: version numbers are
ordered within one process. When the application runs in a process of its own,
its answers carry version numbers of its own counter and the wait may miss the
answer of the command's own write. The reference tests run the application in
the adapter's process, where the ordering holds.
- The commands of a device class are the `<command>` elements of its device
instances, merged by name.
- One command name has the same input and return type in every device instance
of the class.
- A device instance which does not declare a command rejects calls to it as not
allowed.

Aspect: invalid configuration.

- A command without `trigger` and without `inputArgument` aborts the start-up
with a `ChimeraTK::logic_error` naming the command.
- A source which is not in the variable household of the application, which is
not readable or not writeable as its role requires, or whose value type or data
format Tango does not offer as a command type, aborts the start-up with a
`ChimeraTK::logic_error` naming the process variable and the command.
- A `ChimeraTK::Void` source may only be the `trigger`: an `inputArgument` or a
`returnValue` naming a `ChimeraTK::Void` process variable is rejected when the
mapper file is read, because neither a void argument carries a value to write
nor a void return value carries data.
- A `trigger` whose value type is not a write-only `ChimeraTK::Void` process
variable aborts the start-up with a `ChimeraTK::logic_error` naming the process
variable and the command.
- The same command name with a different input or return type in the device
instances of one class is rejected in the same way.
- A command named `State`, `Status` or `Init` is rejected in the same way.
- A process variable which a command of a device instance uses cannot also be
mapped as an attribute of that instance, decided in the mapper file's element
order as the DOOCS adapter's `import()` does: whichever of `<attribute>` and
`<command>` names the source first wins and the other is rejected when the
mapper file is read.
- An `<import>` skips every source the device instance it maps into has already
mapped as an attribute or as a command. A source which only another device
instance of the file maps is unaffected.

Aspect: compatibility option.

- The XML attribute `autoMapCommandsFromVoid` on `deviceServer`, `deviceClass`
and `deviceInstance` restores the behaviour of today when it is `false`: a
write-only `ChimeraTK::Void` process variable becomes a boolean attribute
again, and an explicit `<attribute>` naming one is allowed unless a `<command>`
of the same device instance names it as a source and was processed before the
`<attribute>`.
- The default is `true`, and the value set on the most specific level wins.

Aspect: schema and documentation.

- The XML schema declares the new element, its children and the option on the
three levels.
- The documentation describes commands, the three command forms (a `trigger`
only, a `trigger` with an argument, and an argument whose write is the trigger),
the type restrictions, the blocking behaviour and the option.

## Specifications

Affected components: `include/CommandProperty.h`, `include/Command.h` and
`src/Command.cpp` (new),
`include/TangoPropertyHelper.h` (the shared conversion helper), the mapper
(`include/AttributeMapper.h`, `src/AttributMapper.cpp`), the device class
(`include/AdapterDeviceClass.h`, `src/AdapterDeviceClass.cpp`), the device
instance (`include/AdapterDeviceImpl.h`, `src/AdapterDeviceImpl.cpp`),
`xmlschema/tango_variable_tree.xsd`, `doc/mainpage.dox` and
`doc/example-AttributeMapper.xml`. Build: `CMakeLists.txt` gains
the `file(COPY ...)` entries for the new test assets. Test assets:
`tests/src/testTangoServerCommand.cpp`,
`tests/src/testTangoServerCommandOption.cpp`,
`tests/src/testTangoServerCommandOptionServer.cpp`,
`tests/serverTestCommand_ds-AttributeMapper.xml` (declaring the one device
instance `tango/test/serverTestCommand`),
`tests/serverTestCommandOption_ds-AttributeMapper.xml`,
`tests/serverTestCommandOptionServer_ds-AttributeMapper.xml`,
`tests/src/testTangoServerCommandOptionRoot.cpp`,
`tests/serverTestCommandOptionRoot_ds-AttributeMapper.xml` and
`tests/testCommandOption_template.db`. The shared test application helper
`tests/include_lib/ExtendedReferenceTestApplication.h` and
`tests/src_lib/ExtendedReferenceTestApplication.cc` gains a `description`
element in its `VariableHolder` tuple, so a fixture can add a described process
variable.

Aspect: mapper data model.

- `util::deriveCommandType()` maps a value type and a data format to a
`Tango::CmdArgType`, or to `DATA_TYPE_UNKNOWN` when Tango has no command type
for it. It rejects a data format which is neither a scalar nor a 1D array before
it looks at the value type, so an image becomes `DATA_TYPE_UNKNOWN` whatever
its value type is; the image check must not sit inside the scalar branch. The
mapping does not share the value type mapping with
`util::deriveType()`: a scalar `uint8_t` (which `util::deriveType()` maps to
`DEV_UCHAR`) and a scalar `int8_t` (which it maps to `DEV_ENUM`) both become
`DATA_TYPE_UNKNOWN`. A `uint8_t` array becomes `DEVVAR_CHARARRAY`, a
`ChimeraTK::Boolean` array becomes `DEVVAR_BOOLEANARRAY`, and every other
supported element type its `DEVVAR_*ARRAY`.
- `AttributeMapper::CommandProperty` holds the name, the input and return type
(`DEV_VOID` without an argument or a return value) and the two descriptions.
- `DeviceClass` gains a `std::map<std::string, CommandProperty> commands`, keyed
by name, so a name clash is detected.
- `DeviceInstance` gains a `std::map<std::string, CommandSources>`, with the
three optional source names per declared command.
- `processDeviceInstanceNode()` handles a `command` child: it parses the
attributes and children, rejects an unknown attribute, derives the name and the
two types and calls `addCommand()`.
- `processDeviceInstanceNode()` rejects a `command` element which names neither
`trigger` nor `inputArgument` with a `ChimeraTK::logic_error` naming the
command. The check precedes the name derivation, which has neither source to
take a default name from, and the `addCommand()` call.
- `addCommand()` inserts the command into the class map, or checks the input and
return type of an existing entry and throws a `ChimeraTK::logic_error` when they
differ, and records the sources on the device instance.
- `addCommand()` rejects a command named `State`, `Status` or `Init` with a
`ChimeraTK::logic_error` naming the command, as `addAttribute()` already rejects
the reserved attribute names.
- `addCommand()` rejects a `ChimeraTK::Void` source for `inputArgument` and for
`returnValue`, and a `trigger` whose value type is not `ChimeraTK::Void`, with a
`ChimeraTK::logic_error` naming the process variable and the command.
- `addCommand()` rejects an input or return type of `DATA_TYPE_UNKNOWN` with a
`ChimeraTK::logic_error` naming the process variable and the command, so a value
type or data format Tango has no command type for aborts the start-up.
- Two attributes may keep mapping one source, as `testTangoServerMapTwice`
relies on; only a source shared between a command and an attribute is a clash.
- `addAttribute()` rejects a source which an earlier command of the device
instance has declared, and `addCommand()` rejects a source which an earlier
attribute of the instance has mapped.
- `DeviceInstance` records the sources of the commands it declares as the
command elements are processed, which is the only command-source state: the
exclusion is instance-local and follows the element order, so whichever of
`<attribute>` and `<command>` names a source first wins. `AttributeMapper` keeps
no file-wide command-source set.
- A mapper element names a source in the process variable form
(`INT/TO_DEVICE_SCALAR`), while `getCsVariableNames()` and the source
comparisons work on the canonical form with a leading slash
(`/INT/TO_DEVICE_SCALAR`). Every source name a mapper element provides is
normalised to the canonical form before it enters `attributeToSource` and the
comparisons against the process variable names, so a mapper file may write
either form and the attribute/command exclusion and the derived command name see
one form.
- `import()` skips every source which the device instance it maps into has
already mapped as an attribute or as a command, but not a source which only
another device instance mapped. Two device classes may each import the whole
tree independently and derive the same attributes.
- `import()` gains the auto-mapping branch: a write-only `ChimeraTK::Void`
process variable becomes a trigger-only command through `addCommand()`, or the
attribute of today when `autoMapCommandsFromVoid` is `false`. An explicit
`<attribute>` naming a write-only `ChimeraTK::Void` process variable is
rejected with a `ChimeraTK::logic_error` when the mapper file is read, because
`addAttribute()` rejects a source while the device instance has
`autoMapCommandsFromVoid` in force and the source is a write-only,
non-readable `ChimeraTK::Void` process variable. Only when no `<command>` of the
same device instance has declared that source before the `<attribute>` does
`autoMapCommandsFromVoid="false"` let the explicit `<attribute>` map the boolean
attribute of today.
- `getUnusedVariables()` adds the command sources like the state and status
sources, so the application keeps publishing them.

Aspect: compatibility option.

- `readMapperFile()` reads `autoMapCommandsFromVoid` from the `deviceServer`
root, `processDeviceClassNode()` reads it from `deviceClass` and
`processDeviceInstanceNode()` from `deviceInstance`.
- The most specific level which carries the attribute wins: the device instance
value, else the device class value, else the server value, else the default
`true`.
- The auto-mapping branch of `import()` reads the value of the device instance it
maps into.

Aspect: the Tango command object.

- `TangoAdapter::Command` derives from `Tango::Command` and holds its
`CommandProperty`. It is stateless: the process variables and the wait state
belong to the device instance it is called on.
- `execute()` and `is_allowed()` find the device instance through the
`Tango::DeviceImpl*` argument, like the attribute templates do.
- `AdapterDeviceClass::command_factory()` creates one such command per entry of
the command map and pushes it into `get_command_list()`, next to the attributes
of `attribute_factory()`.
- The Tango in and out types and the two descriptions come from the
`CommandProperty` and are passed to the `Tango::Command` constructor and to
`set_in_type_desc()`/`set_out_type_desc()`.
- `is_allowed()` returns whether the device instance declares the command, so a
call on another device instance of the class fails with
`API_CommandNotAllowed`.
- The two conversions are shared, not repeated: the input path calls the new
`convertFromTangoType<AdapterType, TangoType>`, added next to the existing
`convertToTangoType<TangoType, AdapterType>` (`include/TangoPropertyHelper.h:160`),
and the return path calls
`convertToTangoType<TangoType, AdapterType>` itself, the helper the attribute read
path (`include/ScalarAttribTempl.h:90,110`, `include/SpectrumAttribTempl.h:96`)
and the change-event push (`AdapterDeviceImpl::pushChangeEvent`) use. The
per-Tango-type dispatch of the two paths only selects the template arguments of
those helpers and carries no conversion of its own. `convertFromTangoType()`
takes the adapter type explicitly and deduces the Tango type from the sample, so
the const-qualified buffer element type of a Tango string sequence binds as
well.

Aspect: device instance.

- `AdapterDeviceImpl` gains a map from command name to the three accessors and
to a wait state, which is a mutex and a condition variable; the version number
the wait compares is the write version `execute()` passes (see the execution
aspect below), not a value the wait state holds.
- `attachToClassCommands()` runs in `AdapterDeviceClass::device_factory()` next
to `attachToClassAttributes()`. It skips the commands the device instance does
not declare, and for the others it takes the accessors of its sources from the
CS PV manager, checks that they exist and are readable or writeable as their
role requires, and stores them.
- For a command with a return value it registers the return accessor with
`TangoUpdater::addVariable()` under `get_name() + "/" + name + "/returnValue"`
and passes a callback which notifies the wait state. The return accessor is the
only registered accessor of a command: the trigger and input argument accessors
are stored without registration, because the updater ignores a write-only source
(`TangoUpdater::addVariable()` acts only on a readable accessor, and the trigger
is a write-only `ChimeraTK::Void`) and the call writes the argument itself, so
nothing monitors them.
- The callback only notifies the wait state and takes no Tango lock, but it runs
on the shared updater thread on which the state and status callbacks of the same
device (CR-001) also run and take the device monitor
(`src/AdapterDeviceImpl.cpp:346-358`). The updater thread runs all callbacks of
one updated element before it returns to `waitAny()`
(`src/TangoUpdater.cpp:52-76`).
- Therefore, when the application publishes a new state or status while a
command waits, the updater thread blocks on the device monitor held by the
waiting command, the return-value notification is never delivered and the call
never returns. This deadlock is an accepted limitation of the blocking design;
the 'Releasing the device monitor while the command waits' alternative below is
rejected because it would let two concurrent calls write the same accessors.

Aspect: execution.

- `execute()` extracts the input value from the incoming `CORBA::Any` with the
`Tango::Command::extract()` overload matching the input type and converts each
element into the adapter value type with
`convertFromTangoType<AdapterType, TangoType>`. Only a string element needs a
conversion, and the helper takes care of it.
- It copies the converted values element by element into the input accessor, as
the attribute write path does.
- It sets the data validity of the input accessor to ok and writes it.
- A command with a `trigger` writes it with the accessor's `write()` call which
the void attribute write uses as well.
- Without a return value `execute()` returns an allocated `CORBA::Any` which
carries no value, after the writes. A client must not call `is_empty()` or
`get_type()` on the `Tango::DeviceData` which wraps it: both throw
`API_EmptyDeviceData` in this cppTango build.
- Otherwise it remembers the version number which the argument write and the
trigger write received, the newest of them, waits on the wait state until the
return accessor carries a version number newer than that write, and copies the
accessor's data into the Tango return type through `convertToTangoType()` as
well.
- The write version comes from the accessor the command wrote, through its
`getVersionNumber()` after the write. A trigger-only command takes the version
of its void trigger write; without a trigger the argument write is the
reference.
- The wait predicate is that the return accessor's version number is newer than
the write version, so an update of the return-value process variable which
pre-dates the command's own write, for example the application refreshing it
from an unrelated source, does not end the wait. The wait state stays the
per-command mutex, condition variable and notification callback;
`waitForCommandReturnValue()` (`src/AdapterDeviceImpl.cpp:515-520`) takes the
write version instead of the pre-write snapshot and `execute()` passes it
(`src/Command.cpp:54-58`, `:86`).
- The wait state is the adapter's own per-command mutex and condition variable,
and the command waits on that condition variable
(`AdapterDeviceImpl::waitForCommandReturnValue()`, `src/AdapterDeviceImpl.cpp:515`).
It is notified by the callback which `attachToClassCommands()` passes to
`TangoUpdater::addVariable()` for the return-value accessor
(`src/AdapterDeviceImpl.cpp:489`), not by the return-value process variable's own
queue.
- The wait is a wait on the condition variable with the newer-than-the-write
version number comparison as its predicate, so an unrelated update which
pre-dates the command's own write does not end it (see the accepted limitation
above).
- A return value whose accessor carries a data validity other than ok after the
wait makes `execute()` throw a `Tango::DevFailed` naming the process variable
and carrying a descriptive reason which names the faulty return value, not
`API_CommandNotAllowed`: the command is allowed and the failure is the invalid
data, and a client must be able to tell the two apart. A Tango command has no
quality channel for the invalid data.
- `API_CommandNotAllowed` stays reserved for the rejections of the command
itself, the reason `is_allowed()` relies on.
- The waiting call holds the device monitor like any other command, so the
device answers no other request until the application has answered, and two
calls of one command cannot interleave their writes.
- The adapter itself sets no timeout. A client which waits longer than its own
CORBA timeout has to raise that timeout.

Aspect: schema and documentation.

- `xmlschema/tango_variable_tree.xsd` gains a `CommandType` with the four
attributes, the two description children and an `autoMapCommandsFromVoid`
boolean attribute on `DeviceInstanceType`, `DeviceClassType` and the
`deviceServer` root. The schema is an editor aid, not consumed by the build.
- `doc/mainpage.dox` gains a subsection with the element, the auto-mapping, the
type restrictions and the blocking behaviour. It documents the three command
forms: a `trigger` only, a `trigger` with an argument, and an argument only,
whose write is the trigger. For a command with a trigger and an argument it
states that the argument is written before the trigger.
- `doc/example-AttributeMapper.xml` shows the three forms: `moveToPosition`
carries a `trigger` (`/Motors/move`), an `inputArgument` and a `returnValue`,
`stop` a `trigger` only, and a command without a `trigger` which its
`inputArgument` triggers.

Aspect: coding style.

- The change leaves the shared repository configuration untouched: `.clang-tidy`
and `.gitignore` keep the content the user maintains, so no include-header or
build-output ignore is added to them.
- An include which `misc-include-cleaner` flags gets a per-line
`// NOLINTNEXTLINE(misc-include-cleaner)` with a one-line justification, as
`tests/src_lib/TangoTestServer.cc` already does for `<sys/wait.h>`.

### Alternatives considered

- Mapping a command in addition to the boolean attribute of a write-only
`ChimeraTK::Void` process variable: rejected. The application reads the version
number of that process variable exactly once, so attribute write and command
would consume each other's writes.
- Non-blocking commands: rejected. A client which calls a command has to know
whether the action succeeded, and the adapter has no other way to report the
result.
- A wait timeout: rejected. The time the application needs is unknown to the
adapter, so a timeout would report a failure for an application which works.
- Taking the version number of the return value after the writes: rejected. An
application which answers before the snapshot is taken would make the call block
until the next, unrelated update.
- Releasing the device monitor while the command waits, so that other clients
are served meanwhile: rejected. Two concurrent calls of one command would write
the same accessors at once, and nothing could tell their answers apart.
- Waiting on the continuation of the return-value process variable's queue
instead of the adapter's own condition variable: rejected. `PVManager::getProcessVariable()`
hands out one accessor per process variable, so the return-value element is the
one the shared updater's single `ReadAnyGroup` polls with `waitAny()` and accepts,
and a second wait on its queue would race the updater for the one notification the
element carries: it therefore interferes with the update mechanism. The
notification would still have to be delivered by the shared updater thread, which
the state and status callbacks can block on the device monitor of the waiting
call, so the alternative would not remove the accepted limitation either.
- Returning a value whose data validity is faulty, or waiting for a valid one
instead: rejected. A Tango command has no quality channel, so a client could not
tell a valid answer from an invalid one, and waiting on would hide an
application which reports a problem behind a call which never returns.
- Registering the commands per device instance in
`AdapterDeviceImpl::add_dynamic_commands()`: rejected. The commands of a class
are the same set for every device instance, only the sources differ, and the
class-level registration with a per-instance `is_allowed()` keeps the
client-visible interface in one place.
- Gathering the command sources of the whole file in a pre-pass over the
document (`collectCommandSources()`): rejected. It needs a second tree walk, and
the DOOCS adapter keeps the same first-come behaviour with the instance-local
state of the single document-order pass.
- The `ProxyCommand` indirection with a shared owner and a queue per command of
an earlier draft on `jgeorg/wip/commands`: rejected. It serves no purpose for a
stateless command whose data lives in the device instance.
- Naming a command after its `inputArgument` when it has a trigger as well:
rejected. The trigger is the action, the argument is its parameter, and a
trigger-only command has no other source to take its name from.

## Test plan

- New module `testTangoServerCommand` with the mapper
`tests/serverTestCommand_ds-AttributeMapper.xml`, run with the automatic main
loop of the reference application, because a call blocks until the application
has answered. The mapper declares its `<command>` elements before its
`<import>/</import>`, so the import skips every command source the instance has
already mapped as a command and every other process variable of the household
becomes an attribute of the instance, while `VOID/TO_DEVICE_SCALAR` stays
unmapped although the import covers it. The fixture adds one described process
variable (`INT/DESCRIBED`, see `testCommandDescriptions`), so the household has
88 process variables and the eight command sources of the file
(`INT/TO_DEVICE_SCALAR`, `INT/FROM_DEVICE_SCALAR`, `INT/TO_DEVICE_ARRAY`,
`INT/FROM_DEVICE_ARRAY`, `BOOLEAN/TO_DEVICE_ARRAY`,
`BOOLEAN/FROM_DEVICE_ARRAY`, `VOID/TO_DEVICE_SCALAR` and `INT/DESCRIBED`) are
skipped, so the instance carries 80 imported attributes plus the built-in
`State` and `Status`, 82 in all, not the 89 of `testTangoAttributeMapper` or the
88 of `testTangoServerImportAll`.
- The tests observe a write to a to-device process variable through its version
number, so a command test shows that the application consumed the write and not
only that the call did not throw. The fixture hands its reference application to
the test (`TangoTestFixtureImpl::getContents()`) and
`ApplicationBase::getPVManager()` is public, so a test takes the process array
from the PV manager and compares its `getVersionNumber()` before and after the
call.
- The reference application refreshes a from-device process variable of a
command from two sources: `TypedPVHolder::inputToOutput`
(sources/ChimeraTK-ControlSystemAdapter/tests/include/ChimeraTK/ControlSystemAdapter/Testing/ReferenceTestApplication.h:99-130)
writes `INT/FROM_DEVICE_SCALAR` from the command's argument
`INT/TO_DEVICE_SCALAR` and from the unrelated `INT/BIDIRECTIONAL`, and
`INT/FROM_DEVICE_ARRAY` alike. The wait condition ignores an update which
pre-dates the command's own write, so an unrelated refresh does not end it; a
refresh which post-dates the write does end it, and the reference application's
refresh carries the argument's value, so the assertion on the returned value is
the check for the condition. The tests keep their sources, so repeated full
`ctest -j16` runs probe it.
- The mapper declares one device class with the single device instance
`tango/test/serverTestCommand`. An in-process fixture module takes the
mixed-case instance name the fixture derives from `BOOST_TEST_MODULE` as
`tango/test/` plus the module name; only a module which runs against an offline
database names its instances in lower case. The instance carries the commands
of the module. The mapper leaves `autoMapCommandsFromVoid` at its
default, so `VOID/TO_DEVICE_SCALAR` is a command source of the instance and no
source of it is both a command source and an attribute.
- `testArgumentAndReturnValue`: a command without a trigger, so its argument is
the trigger, with `INT/TO_DEVICE_SCALAR` as argument and
`INT/FROM_DEVICE_SCALAR` as return value. The call returns the value it was
given and the process array of the application carries it. Neither source is
available as a Tango attribute, because the `<import>` skips the sources the
instance has already mapped as commands: `INT_FROM_DEVICE_SCALAR` in particular
does not exist, as it is the `returnValue` of this command and of two others.
The test therefore reads the process array through
`ApplicationBase::getPVManager()` and must not read the return-value source with
a Tango attribute. The assertion `returned == 17` is the check of the wait
condition and fails when the wait ends on an unrelated refresh of the
return-value process variable.
- `testCommandDescriptions`: the `in_type_desc` and `out_type_desc` of
`command_query()` are asserted. The command of `testArgumentAndReturnValue`
carries explicit `inputDescription` and `outputDescription` children and the
query returns their values. To exercise the default, the fixture adds a
described process variable `INT/DESCRIBED` to the household and the mapper
declares `DescribedArgumentAndReturnValue` with no description children and
`INT/DESCRIBED` as both its `inputArgument` and its `returnValue`; its two
descriptions default to the description of that process variable, read through
`ApplicationBase::getPVManager()`, the precedent of the attribute descriptions
of `testTangoServerFilterAttributes`. Because `INT/DESCRIBED` is a command
source it is skipped by the `<import>`, so the attribute count stays 82
although the household grows to 88 process variables and the file to eight
command sources.
- `testDefaultCommandName`: a command declared without a `name` attribute
answers to the name derived from its `trigger` source, and a command without a
trigger to the name derived from its `inputArgument` source; both names are
asserted through `command_query()`. The module mapper declares two further
commands without a `name`: one with only a `trigger` and one with only an
`inputArgument`. Both reuse a source the file already uses as a command source,
so the command sources of the instance and the count of 82 attributes stay
unchanged.

- `testTriggerAndArgument`: a command with a `ChimeraTK::Void` trigger and an
input argument. The application sees both writes, the argument first: the test
asserts a new version number of the process array of both
`INT/TO_DEVICE_SCALAR` and `VOID/TO_DEVICE_SCALAR`. The call returns the return
value.
- `testTriggerOnly`: a command with a write-only `ChimeraTK::Void` trigger. The
call is asserted not to throw, and the test asserts a new version number of the
application's process array of `VOID/TO_DEVICE_SCALAR`, so the trigger write
reached the application. The returned `Tango::DeviceData` is not inspected,
because `is_empty()` and `get_type()` throw `API_EmptyDeviceData` in this
cppTango build.
- `testImportSkipsCommandSource` asserts that the `<import>` element leaves
`VOID/TO_DEVICE_SCALAR` unmapped as an attribute although the import covers it,
and that the command is registered: the instance has already mapped the source
as a command when the import runs, because the mapper declares its `<command>`
elements before its `<import>/</import>`. It asserts the resulting attribute
count of 82: the 88
process variables of the household less the eight command sources, plus the
built-in `State` and `Status`. The count is not the 88 of
`testTangoServerImportAll`, whose mapper imports every process variable.

- `testArrayArgumentAndReturnValue`: an array argument and an array return
value, asserted element by element.
- `testNoReturnValue`: the call returns without waiting for a return value,
asserted against the argument process variable of the application. The argument
is a command source of the instance, so it is not a Tango attribute and the test
reads it through `ApplicationBase::getPVManager()`.
- `testInvalidReturnValue`: with the data validity of the reference application
set to faulty, a call with a return value fails with a Tango exception which
names the return value process variable and carries a descriptive reason, not
`API_CommandNotAllowed`.
- `testAutoMappedVoid`: the module `testTangoServerCommandOptionServer` with
the mapper `tests/serverTestCommandOptionServer_ds-AttributeMapper.xml` sets
`autoMapCommandsFromVoid="true"` on the `deviceServer` root, and its class and
its instances carry no override, so the value of the `deviceServer` level is the
one which applies. The mapper imports with `<import>/</import>`, the only import
form whose derived command name is `VOID_TO_DEVICE_SCALAR`:
`getCsVariableNames()` returns names with a leading slash, so a prefixed
`<import>VOID</import>` matches no process variable, and `import()` strips a
matching prefix from the derived name. The device answers to the derived command
name `VOID_TO_DEVICE_SCALAR`, and the process variable is not an attribute.
- `testAutoMapCommandsFromVoidFalse`: the module
`testTangoServerCommandOption` with the mapper
`tests/serverTestCommandOption_ds-AttributeMapper.xml` asserts what
`autoMapCommandsFromVoid="false"` does: it keeps the write-only
`ChimeraTK::Void` process variable the boolean attribute of today instead of a
command. The file sets the option on the two more specific levels of the chain
and none of its commands uses `VOID/TO_DEVICE_SCALAR`, so every instance really
maps the boolean attribute:
- `tango/test/servertestcommandoption` takes the `false` of its device class
over the `true` of the `deviceServer` root (deviceClass over deviceServer);
- `tango/test/servertestcommandoptionother` takes its own `false` over the
`true` of its device class (deviceInstance over deviceClass).
The two instances belong to two device classes, so the module runs against the
offline database `tests/testCommandOption_template.db`: it declares both device
names in lower case and names the `VOID_TO_DEVICE_SCALAR` attribute with its
`DevBoolean` type in its attribute-list property.
- `testAutoMapCommandsFromVoidFalseOnServer`: the module
`testTangoServerCommandOptionRoot` with the mapper
`tests/serverTestCommandOptionRoot_ds-AttributeMapper.xml` sets
`autoMapCommandsFromVoid="false"` on the `deviceServer` root, while its device
class and its single device instance `tango/test/serverTestCommandOptionRoot`
carry no override, so the value of the root level is the one which applies and
the write-only `ChimeraTK::Void` process variable stays the boolean attribute of
today. The mapper declares no command, so no source is a command source. The `true` which `testAutoMappedVoid` sets on the root equals the
default and therefore does not verify the root level by itself.
- The rejection of a command which a device instance does not declare
(`is_allowed()` returning false, `API_CommandNotAllowed`) stays untested. It
needs two live device instances of one class whose command sets differ, and the
in-process fixture exports one device name only, so the module would have to run
against an offline database which declares the second, otherwise empty instance.
Setting up that instance plus a new offline database declaring it (with its own
`file(COPY ...)` entry) is outside this change request.
- `testBooleanArrayArgumentAndReturnValue`: a Boolean array argument and return
value, asserted element by element.
- `CMakeLists.txt` copies the new mapper assets and
`tests/testCommandOption_template.db` into the build directory with
`file(COPY ... DESTINATION ${CMAKE_BINARY_DIR})` entries, next to the existing
entries, so the tests and the subprocess find them. The existing
`tests/serverTestCommand_ds-AttributeMapper.xml` is already the single-instance
form the module needs; its `<import>/</import>` element moves after the
`<command>` elements, so the import skips the sources the instance has already
mapped as commands, and the mapper comment is reworded from the file-wide
"a command source of this file" rationale to the instance-local, first-come
one. `tests/testCommandTwoDevices_template.db` and
`tests/rejectCommandOnlyReturnValue` and their `file(COPY ...)` entries were
already removed, so only the new assets need `file(COPY ...)` entries.
- Rejections at start-up, each with its own mapper file, asserted by running
`testTangoServer` as a subprocess as described in CR-002:
- a command without `trigger` and without `inputArgument` (for example one
with only a `returnValue`), so the `name` derivation has no source;
- an unsupported value type (`uint8_t` scalar, `int8_t` scalar, `int8_t` array);
an image cannot occur, because the ControlSystemAdapter delivers single-channel
process arrays, so it stays untested like the image rejection of CR-002.
- an explicit `<attribute>` naming a write-only `ChimeraTK::Void` process
variable;
- a `returnValue` naming a `ChimeraTK::Void` process variable;
- an `inputArgument` naming a `ChimeraTK::Void` process variable;
- a `trigger` naming a process variable which is not a write-only
`ChimeraTK::Void`;
- a source which is not in the variable household of the application;
- an `inputArgument` naming a process variable which is not writeable;
- a `returnValue` naming a process variable which is not readable;
- a process variable which a command source and an attribute share, with the
`<command>` element first: the `<attribute>` which names the source second is
rejected;
- a process variable which a command source and an attribute share, with the
`<attribute>` element first: the `<command>` which names the source second is
rejected;
each with its own mapper file, the existing
`tests/rejectCommandSourceAsAttribute` staying the attribute-first fixture with
its order-independence comment removed;
- the same command name with different types in two device instances;
- a command named `State`, `Status` or `Init`, each with its own mapper file.
- The existing tests which rely on the boolean attribute of a write-only
`ChimeraTK::Void` process variable have to follow the new mapping:
`testTangoServerImportAll` (its attribute count and the void process variables),
`testTangoAttributeMapper` (its attribute count, which drops from 90 to 89) and
`testTangoServerReadWrite` (its `Void` instantiation of the scalar test).
- `testTangoServerMapTwice` is unaffected: two attributes which map one source
stay supported, verified by its five attributes and both renames.
The mapper of `testTangoServerEventing`,
`tests/serverTestEventing_ds-AttributeMapper.xml`, maps
`VOID/TO_DEVICE_SCALAR` with an explicit `<attribute>`, so it needs
`autoMapCommandsFromVoid="false"` on that device instance; otherwise the whole
mapper fails at start-up and every CR-002 eventing test fails.
- The full `ctest` run of the project, including the `coding_style` test.

## Deferred issue

- DI-1 [DONE] Missing dependency: CR-003's Test plan (line 276) says the start-up rejections are asserted by running `testTangoServer` as a subprocess "as described in CR-002", and both CRs edit the same files (src/AttributMapper.cpp, src/AdapterDeviceImpl.cpp, include/AttributeMapper.h, include/AdapterDeviceImpl.h, xmlschema/tango_variable_tree.xsd, doc/mainpage.dox, doc/example-AttributeMapper.xml, CMakeLists.txt), but CR-003 has no `Depends on:` line while CR-002 does (`Depends on: CR-...
- DI-2 [DONE] Factually wrong rejection rationale: Requirements (lines 51-53) reject an array of `ChimeraTK::Boolean` "because Tango does not offer a command type for them", but cppTango offers `DEVVAR_BOOLEANARRAY` (install-debug/include/tango/common/tango_const.h:771) with `Command::extract(..., DevVarBooleanArray)` (tango/server/Command.h:553) and `insert(DevVarBooleanArray*)` (Command.h:1013), and the reference application already provides Boolean arrays (TypedPVHolder<ChimeraTK::Bool...
- DI-3 [DONE] Test-asset wiring missing: CR-003 adds `tests/serverTestCommand_ds-AttributeMapper.xml` and one rejection mapper file/directory per rejection case (Test plan line 275-280), but `CMakeLists.txt` is not listed among the affected components (Specifications line 104-111) and the required `file(COPY ... DESTINATION ${CMAKE_BINARY_DIR})` entries are never mentioned. Every existing mapper asset and the CR-002 reject directories need such an entry (CMakeLists.txt:103-107), so withou...
- DI-4 [DONE] Internal inconsistency in the type mapping: Specifications (lines 115-119) say `deriveCommandType()` "shares the value type mapping with `util::deriveType()`", but `util::deriveType()` maps `uint8_t` to `DEV_UCHAR` and `int8_t` to `DEV_ENUM` (src/AttributMapper.cpp:134-140), while the Requirements reject both as command types (lines 51-53). A shared mapping cannot yield DATA_TYPE_UNKNOWN for those scalars; the text must state the overrides, and the Requirements' "1D array of...
- DI-5 [DONE] Order-dependent mutual exclusion of attribute and command source: Requirements (lines 90-92) demand that an explicit `<attribute>` naming a command source is rejected when the mapper file is read, but the Specifications (lines 143-144) only reach this through `_usedInputVariables`, which `addCommand()` fills while the command element is processed. `processDeviceInstanceNode()` (src/AttributMapper.cpp:315-329) walks the children in document order in one pass, so an `<attribut...
- DI-6 [DONE] The compatibility option `autoMapCommandsFromVoid` is specified nowhere: the Requirements (lines 96-100) demand it on `deviceServer`, `deviceClass` and `deviceInstance` with 'the value set on the most specific level wins', but the Specifications (lines 121-151, 213-218) never state which code reads the attribute on the three levels, where the parsed value is stored, how the levels are combined or which value the auto-mapping branch of `import()` (line 145-147) consults. The ...
- DI-7 [DONE] Overlooked side effect on CR-002 (DONE): `tests/serverTestEventing_ds-AttributeMapper.xml:29` maps `VOID/TO_DEVICE_SCALAR` (write-only `ChimeraTK::Void`) with an explicit `<attribute>` (used by tests/src/testTangoServerEventing.cc:292). CR-003 rejects exactly that while `autoMapCommandsFromVoid` is true (Specifications lines 147-149), so the whole `serverTestEventing` mapper fails at start-up and every CR-002 eventing test fails, not just one assertion. The Test plan (lines ...
- DI-8 [DONE] A `ChimeraTK::Void` process variable is only defined for the `trigger` role although it is also accepted as `inputArgument` (a writeable source, Requirements lines 37-39) and as `returnValue` (a readable source, e.g. `VOID/FROM_DEVICE_SCALAR`), and 'void' is in the accepted command-type list (line 53). For such a source the Specifications leave `execute()` undefined: 'extracts the input value from the incoming `CORBA::Any` with the `Tango::Command::extract()` overload matchi...
- DI-9 [DONE] The `trigger` role's value type is not enforced: Requirements (line 35) say `trigger` names a write-only `ChimeraTK::Void` process variable, but the invalid-configuration rules (lines 81-89: in the household, readable/writeable as the role requires, a Tango command type) accept a writeable source of any supported type as `trigger`, and the Specifications contain no check for it. Either the rejection has to be specified or the requirement has to drop the restriction; as it st...
- DI-10 [DONE] Blocking-command deadlock not handled: the Specifications ('Aspect: device instance', lines 206-210) register the return-value accessor with TangoUpdater::addVariable() and claim the notifying callback 'takes no Tango lock, so it cannot deadlock with the device monitor which the calling client thread may hold'. That claim is wrong. All callbacks run on the single shared updater thread (src/TangoUpdater.cpp:52-76, it runs every callback of one updated element before returnin...
- DI-11 [DONE] Overlooked side effect: tests/src/testTangoAttributeMapper.cc:36 (executable testTangoAttributeMapper, BOOST_TEST_MODULE serverTestImportAll, auto-import mapper) asserts attributes->size() == 90. CR-003 turns the write-only ChimeraTK::Void variable VOID/TO_DEVICE_SCALAR into a command, so the count becomes 89, exactly as for testTangoServerImportAll (89 -> 88), but the Test plan (CR-003 lines 349-352) lists only testTangoServerImportAll and testTangoServerReadWrite, so this...
- DI-12 [DONE] Internal inconsistency with a concrete failure: Specifications lines 160-164 make addAttribute() throw for any source in _usedInputVariables to enforce the attribute/command exclusion, but _usedInputVariables also holds attribute sources (src/AttributMapper.cpp:256) and mapping one source to two attributes is a supported, tested feature (tests/serverTestMapTwice_ds-AttributeMapper.xml maps INT/FROM_DEVICE_SCALAR to PlainRename1/PlainRename2; tests/src/testTangoServerMapTwic...
- DI-13 [REPLACED] Order-dependent interaction of `<import>` and `<command>` contradicts the Requirements: Requirements (lines 102-104) promise that an `<import>` covering a command source "skips it", and Specifications (lines 163-167) claim the attribute/command exclusion "is independent of the element order in the mapper file". But `processDeviceInstanceNode()` walks the instance children in document order in a single pass (src/AttributMapper.cpp:315-329) and `import()` only skips sources a...
- DI-14 [DONE] The Specifications never specify the reserved command-name rejection that the Requirements demand (doc:101). No bullet of 'Aspect: mapper data model' or 'Aspect: the Tango command object' (spec:133-204) mentions rejecting a command named `State`, `Status` or `Init`, although the Test plan tests it (doc:356) and the analogous attribute check exists in the code (src/AttributMapper.cpp:200-203). The implementation phase cannot place or word the check from the plan.
- DI-15 [DONE] The Test plan's rejection list (doc:339-356) does not cover every invalid configuration the Requirements demand. Missing are a source which is not in the variable household of the application and a source which is not readable or not writeable as its role requires (req:88-91), as well as the reserved names `State` and `Init` (only `Status` is listed, doc:356). Each is a start-up `ChimeraTK::logic_error` per the Requirements and needs its own reject mapper per the plan's own...
- DI-16 [DONE] All 15 existing items (DI-1 to DI-15) are correctly folded; the remaining problems below are new. Specification gap: no bullet states the rejection of a command source whose value type or data format yields DATA_TYPE_UNKNOWN. The Requirements demand a start-up ChimeraTK::logic_error naming the process variable and the command (doc:88-91) and the Test plan asserts it (doc:345), but 'Aspect: mapper data model' only defines deriveCommandType() (doc:135-142) and lists the addCo...
- DI-17 [DONE] Test plan gap: the rejection list covers an unsupported value type with a uint8_t scalar and an int8_t array only (doc:345), while the Requirements reject a scalar int8_t as well (doc:58-60, 'A scalar uint8_t or int8_t process variable, an array of int8_t, and an image are rejected'), and the plan's own rule is one reject mapper per rejection (doc:342). The scalar int8_t rejection is therefore untested; the image case cannot occur (single-channel process arrays) and needs n...
- DI-18 [REPLACED] Order- and instance-dependent command/attribute exclusion: the Specifications implement the exclusion with a command-source set on DeviceInstance (doc:147-149) plus the file-wide _usedInputVariables (include/AttributeMapper.h:79) checked by addCommand() (doc:164-165), while import() and addAttribute() consult the instance-local set. A source which a later device instance declares as a command and an earlier one maps with an explicit <attribute> is therefore accepted, althou...
- DI-19 [DONE] Specification gap: the mapper syntax/validation section has no rule rejecting a `<command>` that names neither `trigger` nor `inputArgument`, although the requirements demand at least one and the test plan (`doc/change-requests/CR-003-tango-commands.md:346`) only covers "a command with only a `returnValue`". The `name` derivation rule (doc:40-41) also has no defined source for such a command. Add the rejection rule (and its name/description consequences) to the mapper secti...
- DI-20 [DONE] Specification contradicts the requirements and the test plan for an explicit `<attribute>` naming a write-only `ChimeraTK::Void`: doc:177-179 says such an attribute "never maps" the source while `autoMapCommandsFromVoid` is `true`, but the requirements (doc:26-28) and the test plan (doc:350-351) say it is rejected when the mapper file is read. Reword doc:177-179 so the rejection is stated as the behaviour, independent of the option value.
- DI-21 [DONE] `testNotAllowed` requires two live device instances of the same device class to prove that a source which is a command of one instance and an attribute of another is rejected in either order (doc:170-174), but the in-process fixture exports exactly one device name via `-dlist` (`tests/src_lib/TangoTestServer.cc:51`), and `-dlist` applies only to the last class created in `class_factory()`. The plan names no fixture change and no device-list asset, so the test as planned can...
- DI-22 [DONE] `testAutoMappedVoid` claims to cover the `deviceClass` and `deviceServer` levels (doc:185-186), but one mapper file with one `deviceClass` can exercise at most two links of the deviceInstance/deviceClass/deviceServer "most specific level wins" chain, so `deviceServer` coverage is unreachable in the planned test setup. State exactly which option/level combinations are verified and name the mapper assets for each, or restrict the claim to the levels actually exercised.
- DI-23 [REPLACED] Contradiction between the folded-in Specifications and the Test plan for the command/attribute exclusion of a source shared between two device instances. The Specifications (CR-003 lines 173-179; implemented in src/AttributMapper.cpp:321 via the file-wide _commandSources set) require that a source used by a command anywhere in the mapper is rejected as an explicit <attribute> independent of the element order AND of the order of the device instances: 'a source which is a com...
- DI-24 [DONE] Consequential test-asset problem arising from the same contradiction: tests/testCommandTwoDevices_template.db carries a stale AttributeList property ('tango/test/servertestcommand->AttributeList: VOID_TO_DEVICE_SCALAR;VOID/TO_DEVICE_SCALAR;SCALAR;DevBoolean;;') that declares VOID_TO_DEVICE_SCALAR as a DevBoolean attribute of the instance which the Test plan wants to carry the auto-mapped command, so the asset also has to be reworked together with the mapper once the option ...
- DI-25 [DONE] Device instance names do not match the devices the offline database declares: the Test plan (lines 345-361) and the asset tests/serverTestCommand_ds-AttributeMapper.xml use the mixed-case names tango/test/serverTestcommand and tango/test/serverTestcommandother, while tests/testCommandTwoDevices_template.db:9-10 declares the lower-case names tango/test/servertestcommand and tango/test/servertestcommandother, and every existing mapper and DB template uses lower-case names. Ad...
- DI-26 [DONE] testTangoServerCommand.cpp:89 and :132 (and the analogous assertion in testTangoServerCommandOptionServer.cpp) use BOOST_TEST(out.is_empty()) on the DeviceData returned by a void-return command. In this cppTango 10.3 build both out.is_empty() and out.get_type() throw a non-std Tango error (API_EmptyDeviceData / 'Cannot extract, no data in DeviceData object'), which Boost.Test reports as 'fatal error: ... unknown type'. Void returns therefore cannot be asserted with is_empty...
- DI-27 [DONE] The optionServer test plan (CR-003 lines 358-360) expects the mapper to import VOID/TO_DEVICE_SCALAR and derive the command name VOID_TO_DEVICE_SCALAR, but import() strips the import source prefix from the derived name: <import>VOID</import> matches nothing because getCsVariableNames() returns names with a leading slash ('/VOID/TO_DEVICE_SCALAR'), and even a matching source would derive 'TO_DEVICE_SCALAR'. Only <import>/</import> (import-everything) yields 'VOID_TO_DEVICE_S...
- DI-28 [DONE] Ordering of remaining work is blocked on the two decisions above; the Command and CommandOption test modules cannot be made to pass until the void-return assertion and the import-source semantics are settled.
- DI-29 [DONE] testNotAllowed (CR-003 Test plan, doc/change-requests/CR-003-tango-commands.md:381-397) cannot be implemented as planned. The plan requires one device class with two live device instances where only the instance tango/test/servertestcommandother declares the attribute VOID_FROM_DEVICE_SCALAR. Start-up crashes before any test runs.
- DI-30 [DONE] Root cause is pre-existing (not CR-003): AdapterDeviceImpl::attachToClassAttributes (src/AdapterDeviceImpl.cpp:278-290) iterates the class-level attribute list for every instance and performs 'auto& source = device->attributeToSource[attr.name]' (line 287) on the per-instance map (include/AttributeMapper.h:37). For an instance that does not declare that class attribute, std::map::operator[] default-constructs "", RegisterPath("") makes it "/", and csPvManager->getProcessVaria...
- DI-31 [DONE] AdapterDeviceClass::device_factory (src/AdapterDeviceClass.cpp:140-168) calls attachToClassAttributes once per device of the class, so the crash is unavoidable for this instance layout. addAttribute also appends to the class attribute list unconditionally (src/AttributMapper.cpp:344), so declaring the attribute on both instances instead yields duplicate class attributes rather than a valid setup.
- DI-32 [DONE] The plan itself (doc/change-requests/CR-003-tango-commands.md:377-380, testAutoMapCommandsFromVoidFalse) rules out the alternative of combining an auto-mapped command source with an attribute source of the same name in one file, so there is no in-plan way to realise testNotAllowed without either changing the plan or fixing the pre-existing attachToClassAttributes defect. This is a plan/specification problem: the plan assumes a capability (per-instance attribute declaration wi...
- DI-33 [DONE] DI-26 is not correctly folded: it is marked FOLDED-IN, but the last two document commits (16699db has a single hunk, 5c13aba two hunks - Status line plus the Deferred issue section) changed nothing in the Requirements, Specifications or Test plan. The Test plan still words the void return as 'returns an empty value' (doc:346) and nowhere states the decided assertion idiom, so the test-write phase cannot know that the returned Tango::DeviceData must not be inspected with is_em...
- DI-34 [DONE] DI-27 is not correctly folded: it is marked FOLDED-IN although no body change records the decision. The Test plan (doc:354-360) still only says the optionServer mapper 'imports the write-only ChimeraTK::Void process variable VOID/TO_DEVICE_SCALAR' and derives VOID_TO_DEVICE_SCALAR, without naming the import element: import() (src/AttributMapper.cpp:767-800) strips the import prefix from the derived name and getCsVariableNames() returns names with a leading slash, so only <imp...
- DI-35 [DONE] Test plan, 'The rejection of a command which a device instance does not declare ... stays untested' (doc:384-392): the rationale is factually wrong. The test does not need two instances of one class with different attribute sets; it needs their command sets to differ. An instance which declares no `<attribute>` and no `<command>` leaves the class attribute list empty, so `AdapterDeviceImpl::attachToClassAttributes` (src/AdapterDeviceImpl.cpp:286-290) performs no lookup and ca...
- DI-36 [DONE] Option chain: the top link 'deviceServer, else the default true' (req:114, spec:204-206) is unverified and the reason given is wrong. `testAutoMappedVoid` (doc:354-360) sets `autoMapCommandsFromVoid="true"` on the root, a value indistinguishable from the default, and the Test plan (doc:380-383) claims the root's `false` 'is not exercised, because a file cannot combine an instance which auto-maps the source into a command with an instance which maps it as an attribute'. A mapp...
- DI-37 [REPLACED] Test plan (doc:421-423): the bullet 'an `<import>` covering a command source placed before the `<command>` element: the source stays unmapped as an attribute and the command is registered' is an internal inconsistency. It is listed among the start-up rejections, which are 'each with its own mapper file, asserted by running `testTangoServer` as a subprocess' expecting a non-zero exit, but the case is not a rejection and a non-zero exit cannot assert that the command got regi...
- DI-38 [DONE] Test plan rejections list (doc/change-requests/CR-003-tango-commands.md:417-419) lists one and the same invalid configuration twice: 'a command with only a `returnValue`' is exactly 'a command without `trigger` and without `inputArgument`', the single rule the Specifications define (doc:162-165). Under the plan's own rule 'each with its own mapper file' (doc:415) this yields two identical reject fixtures - the assets created for them, tests/rejectCommandOnlyReturnValue/test...
- DI-39 [DONE] Internal inconsistency in the Test plan (CR-003 lines 400-406): the bullet explaining why the 'device instance does not declare the command' rejection stays untested says 'reworking `tests/testCommandTwoDevices_template.db` is outside this change request', but the same Test plan (lines 412-414) removes that asset and its `file(COPY ...)` entry (`CMakeLists.txt:109`). A file cannot both be reworked and removed; reword the bullet to say the test would need a new offline datab...
- DI-40 [DONE] Overlooked side-effect of the revised test plan: the plan now describes `tests/serverTestCommand_ds-AttributeMapper.xml` as a single-instance mapper that uses `<import>/</import>` and leaves `autoMapCommandsFromVoid` at its default (lines 135-136, 333-341) and merges the two duplicate 'no trigger/inputArgument' rejections into one bullet (DI-38, lines 417-419), but it does not state that the existing assets which contradict this must be reworked or removed: the on-disk mapp...
- DI-41 [DONE] Implementation phase incomplete: HEAD (36c5a94) and the working tree lack the option test sources tests/src/testTangoServerCommandOption*.cpp, the option mappers tests/serverTestCommandOption_ds-AttributeMapper.xml, tests/serverTestCommandOptionServer_ds-AttributeMapper.xml and tests/serverTestCommandOptionRoot_ds-AttributeMapper.xml, and tests/testCommandOption_template.db, all of which the CR Specifications (CR-003-tango-commands.md:131-141) and Test plan (lines 364-399) ...
- DI-42 [DONE] CMakeLists.txt in HEAD does not match the CR: no file(COPY ... testCommandOption_template.db) entry (required by Test plan lines 409-411), and the required removals of tests/testCommandTwoDevices_template.db and its COPY entry and of tests/rejectCommandOnlyReturnValue and its COPY entry (Test plan lines 413-421) are absent.
- DI-43 [DONE] doc/mainpage.dox and doc/example-AttributeMapper.xml command documentation (Test plan/Specifications lines 284-293) are absent from HEAD, so the implementation phase did not land all non-test deliverables.
- DI-44 [DONE] The recorded session summary (9 passing tests, 'implementation phase already added option assets') is not reproducible: the option sources never existed in any commit (git log --all is empty for them); only stale build artifacts remain whose embedded debug paths point to now-deleted, never-tracked files.
- DI-45 [DONE] The CR's Deferred issue section (DI-26/27/33/34) states the void-return assertion idiom and the import-source semantics as FOLDED-IN but no Requirements/Specifications/Test plan body change records them, leaving the tests-write phase without a decided assertion for void returns.
- DI-46 [DONE] CR Deferred issues DI-29 to DI-37 remain unresolved: testNotAllowed and the option-level tests depend on a device layout that aborts start-up because of a pre-existing AdapterDeviceImpl::attachToClassAttributes defect, and the deviceServer/deviceClass option-chain coverage is unverified; these are plan/specification problems, not test problems.
- DI-47 [DONE] The build directory is not a valid separate build tree: CMAKE_HOME_DIRECTORY points at the source root and cmake . regenerated build files in the source directory, so stale artifacts in the build dir cannot be used as verification evidence.
- DI-48 [DONE] Device-instance casing conflict between the CR and the working code: the Test plan (CR-003 lines 338, 345, 348) and folded DI-25/DI-40 mandated the lower-case mapper instance `tango/test/servertestcommand`, but the in-process fixture passes `-dlist tango/test/<testName>` (mixed case, derived from BOOST_TEST_MODULE) in tests/src_lib/TangoTestServer.cc:47-56, so a lower-case mapper instance is not registered and the client cannot connect. The CR text was the wrong side of the c...
- DI-49 [DONE] testTangoServerCommandOption (testAutoMapCommandsFromVoidFalse) fails because the write-only Void attribute of the second device class is not registered: 'MultiAttribute::get_attr_by_name throwing exception' on get_attribute_config_5 for tango/test/servertestcommandoptionother. This reproduces the pre-existing AdapterDeviceImpl::attachToClassAttributes defect (src/AdapterDeviceImpl.cpp:287, per-instance device->attributeToSource[attr.name]) that the CR itself defers as DI-3...
- DI-50 [DONE] coding_style fails only because of the in-source build pollution (generated/VersionInfo.h, CMakeFiles/3.28.3/CompilerIdCXX/CMakeCXXCompilerId.cpp) described by DI-47, not because of the new files. The source directory is polluted by an in-source CMake build, so ctest cannot pass until that is cleaned or the build is re-created as a proper out-of-source tree.
- DI-51 [DONE] The CR's Deferred issue section cannot be updated by this phase (it must stay untouched), and its DI-41/DI-42/DI-43 entries are now partially addressed by the new files but the two blockers above prevent reporting the change request as implemented.
- DI-52 [DONE] Duplicated code in tests: the new helper `runTestTangoServer` in tests/src/testTangoServerCommand.cpp:195-208 duplicates the helper of the same name in tests/src/testTangoServerEventing.cc:142-155 (identical except for the exec'd sub-process name, `CommandReject` vs `TestEventingReject`). Factor it into one shared test helper (e.g. under tests/src_lib) instead of copying it.
- DI-53 [DONE] src/Command.cpp: clang-tidy misc-include-cleaner reports unused includes at line 7 (`TangoPropertyHelper.h`), line 9 (`<ChimeraTK/Utilities.h>`) and line 13 (`<vector>`).
- DI-54 [DONE] src/Command.cpp: clang-tidy readability-redundant-typename reports redundant `typename` at lines 39, 44, 213, 225 and 314.
- DI-55 [DONE] src/Command.cpp: clang-tidy cppcoreguidelines-owning-memory at line 113 (function returns a newly created `CORBA::Any*` through a non-owner return type) and line 318 (non-owner `auto*` initialised from a `gsl::owner<>`).
- DI-56 [DONE] src/AdapterDeviceImpl.cpp: clang-tidy readability-convert-member-functions-to-static at line 443 (`attachToClassCommands`), line 505 (`getCommandState`) and line 515 (`waitForCommandReturnValue`).
- DI-57 [REJECTED] src/AttributMapper.cpp:436: clang-tidy readability-convert-member-functions-to-static (`addCommand` can be made static).
- DI-58 [DONE] tests/src/testTangoServerCommand.cpp: clang-tidy misc-include-cleaner reports unused includes at lines 6 (`TangoTestServer.h`), 8 (`tango.h`), 10 (`unit_test.hpp`), 18 (`<memory>`) and 21 (`<vector>`).
- DI-59 [DONE] tests/src/testTangoServerCommandOption.cpp: clang-tidy misc-include-cleaner reports unused includes at lines 8 (`tango.h`), 10 (`unit_test.hpp`) and 13 (`<memory>`).
- DI-60 [DONE] tests/src/testTangoServerCommandOptionRoot.cpp: clang-tidy misc-include-cleaner reports unused includes at lines 8 (`tango.h`), 10 (`unit_test.hpp`) and 12 (`<string>`).
- DI-61 [DONE] tests/src/testTangoServerCommandOptionServer.cpp: clang-tidy misc-include-cleaner reports unused includes at lines 8 (`tango.h`), 10 (`unit_test.hpp`), 12 (`<memory>`), 13 (`<string>`) and 14 (`<vector>`).
- DI-62 [DONE] src/Command.cpp:17: the file-local helpers in the anonymous namespace (scalarFromTango at :22, scalarToTango at :36) are the only anonymous-namespace functions in src/ and trip the enabled clang-tidy check llvm-prefer-static-over-anonymous-namespace ('function scalarFromTango/scalarToTango is declared in an anonymous namespace; prefer using static'). Give them internal linkage the way the rest of the project does (static, or the util namespace used by AttributMapper.cpp).
- DI-63 [DONE] src/Command.cpp:50: 'Tango::string_dup(value.c_str())' trips readability-redundant-string-cstr. Tango provides the std::string overload (install-debug/include/tango/common/tango_const.h:23, 'inline char *string_dup(const std::string &s)'), so the c_str() call is redundant: pass the std::string directly.
- DI-64 [DONE] src/AdapterDeviceClass.cpp:223: 'get_command_list().push_back(new Command(commandEntry.second))' trips cppcoreguidelines-owning-memory ('initializing non-owner argument ... with a newly created gsl::owner<>'). Tango takes ownership of the pointer, as in src/Command.cpp:112 and :319, so it needs the same NOLINTNEXTLINE(cppcoreguidelines-owning-memory) with a one-line justification that the list takes ownership (cf. resolved DI-55).
- DI-65 [DONE] include/Command.h:5 and :11: the header includes AttributeMapper.h, which no declaration in it uses, while the CommandProperty it does use (lines 95 and 101) comes from CommandProperty.h, which it does not include directly; clang-tidy misc-include-cleaner reports 'included header AttributeMapper.h is not used directly'. Include CommandProperty.h (the header the file uses) instead, and drop the <type_traits> include at line 11, which is likewise unused.
- DI-66 [DONE] include/AdapterDeviceImpl.h:9: the added '#include "CommandProperty.h"' is unused; the header stores no CommandProperty (AdapterDeviceImpl::CommandState holds the accessors, and the command map with CommandProperty lives in include/AttributeMapper.h:56, which this header already includes at line 5). clang-tidy misc-include-cleaner reports 'included header CommandProperty.h is not used directly'. Remove the include.
- DI-67 [DONE] Style-review housekeeping for the go-back: src/AttributMapper.cpp:437-438 and tests/src/testTangoServerCommand.cpp:200 are accepted as they are (performance-unnecessary-value-param and modernize-use-ranges fire on identical pre-existing code in the same files, e.g. src/AttributMapper.cpp:296/:312 and src/AdapterDeviceClass.cpp:61/:232/:253), and are deliberately not reported as problems.
- DI-68 [DONE] Device-instance casing: the CR text mandates lower case, the working code uses mixed case. CR-003 Specifications (doc/change-requests/CR-003-tango-commands.md:135-136) and Test plan (:343-344 for the main module, :400 for testAutoMapCommandsFromVoidFalseOnServer) name the mapper instances tango/test/servertestcommand and tango/test/servertestcommandoptionroot, and the Test plan adds 'written in lower case' (:344). The on-disk mappers declared by the same CR use the mixed-case...
- DI-69 [DONE] The lower case of the CR text is only satisfiable by the offline-DB option module, so the CR's blanket 'in lower case' wording is misleading: tests/testCommandOption_template.db:9-10 and tests/serverTestCommandOption_ds-AttributeMapper.xml:4,11 use tango/test/servertestcommandoption and tango/test/servertestcommandoptionother because that module is DB-driven (the test hardcodes the names, tests/src/testTangoServerCommandOption.cpp:52-53). The CR should state that in-process f...
- DI-70 [DONE] Trigger-write verification missing (test gap, go back to IMPLEMENTED). The Test plan expects the trigger write to be verified by the application, but no command test observes it. testTriggerOnly (tests/src/testTangoServerCommand.cpp:96-102) only does BOOST_CHECK_NO_THROW; the plan (doc/change-requests/CR-003-tango-commands.md:358-360) also requires asserting that the application's VOID/TO_DEVICE_SCALAR process array carries a new version number, and testTriggerAndArgument (...
- DI-71 [DONE] Image-format command source is not rejected. src/AttributMapper.cpp:209 tests `dataFormat == AttrDataFormat::IMAGE` inside the `dataFormat == AttrDataFormat::SCALAR` branch, so the condition is always false and an image-format source falls through to the 1D array mapping (src/AttributMapper.cpp:215-247) instead of returning DATA_TYPE_UNKNOWN. Requirements (doc/change-requests/CR-003-tango-commands.md:58-60: 'A scalar uint8_t or int8_t process variable, an array of int8_t, a...
- DI-72 [DONE] Faulty return value is reported with a misleading Tango error reason. src/Command.cpp:119 raises the 'return value carries no valid return value' failure with the reason `API_CommandNotAllowed`, the same reason used for five unrelated situations (src/Command.cpp:72, 78, 101, 202, 295), so a client of a command whose returnValue process variable has an invalid data validity is told the command is not allowed on the device. The Specification (CR-003:281-283) only requires a T...
- DI-73 [DONE] Test coverage gap: the command descriptions are untested. Requirements (CR-003:42-43) and Specifications (:157, :231) define `inputDescription`/`outputDescription` - explicit values and the default from the description of the argument/return-value process variable - and the module mapper already declares them (tests/serverTestCommand_ds-AttributeMapper.xml, command `ArgumentAndReturnValue`), but no test asserts them. tests/src/testTangoServerCommand.cpp:175 calls command_qu...
- DI-74 [DONE] Test coverage gap: no test covers the `name` default. Requirements (:40-41) say the name defaults to the name derived from the `trigger` source, or from the `inputArgument` source when there is no trigger (src/AttributMapper.cpp:534-537), but every `<command>` of the module mapper carries an explicit `name`, and the only derived-name coverage is the auto-map path of import() (testAutoMappedVoid). A regression in the name default of processCommandNode() and its `inputArgumen...
- DI-75 [DONE] Test plan vs tests mismatch: the Test plan (:381-385) says testTriggerOnly also asserts that `<import>/</import>` leaves `VOID/TO_DEVICE_SCALAR` unmapped and that the command is registered, but the code has those assertions in a separate test case `testImportSkipsCommandSource` (tests/src/testTangoServerCommand.cpp:162-177), which the plan does not name; testTriggerOnly (:145-157) only asserts the no-throw call and the trigger version number.
- DI-76 [DONE] Implementation defect (blocks DI-73 and DI-74): the command-source exclusion never fires for the mapper source convention this repository uses. Every mapper file in the repo writes <command>/<attribute> source attributes WITHOUT a leading slash (e.g. tests/serverTestCommand_ds-AttributeMapper.xml uses inputArgument="INT/TO_DEVICE_SCALAR", trigger="VOID/TO_DEVICE_SCALAR"; likewise tests/serverTestEventing_ds-AttributeMapper.xml, tests/serverTestFilterAttributes_ds-AttributeM...
- DI-77 [DONE] Consequence for this phase: DI-73 (command descriptions) and DI-74 (name default) cannot be satisfied as long as the mapper uses the repository's no-slash convention. Adding the two commands described by the Test plan (:370-381) with no-slash sources leaves them unusable, and writing their sources with a leading slash (the only form the current matching accepts, and the form doc/example-AttributeMapper.xml uses) makes the pre-existing tests testArgumentAndReturnValue and te...
- DI-78 [DONE] Where to fix (implementation choice for the READY TO IMPLEMENT phase): either normalise source names to the PV-name form (leading slash) before they enter _commandSources/_usedInputVariables and the addAttribute/addCommand comparisons, so the documented no-slash convention works; or convert every mapper's source attribute (and the documentation/example) to the leading-slash form. Either way the resulting attribute set changes from the figure the Test plan/tests currently as...
- DI-79 [DONE] Additional genuine defect found while working (folded in during the attempt but reverted to leave the tree clean for the go-back): src/AttributMapper.cpp:536, name = util::deriveAttributeName(sources.trigger ? *sources.trigger : *sources.inputArgument), dereferences *sources.inputArgument even when trigger is engaged; for a trigger-only <command> without an explicit name it dereferences an empty std::optional (assertion/UB/SIGABRT at server start-up). Correct form: sources....
- DI-80 [REPLACED] Test plan vs implementation: the Test plan (CR-003 test plan, testArgumentAndReturnValue / testNoReturnValue bullets) does not state which attribute count the command mapper's `<import>/</import>` yields once the command-source exclusion fires. Fixing DI-76/DI-78 per the specification excludes every command source of tests/serverTestCommand_ds-AttributeMapper.xml from the import (INT/TO_DEVICE_SCALAR, INT/FROM_DEVICE_SCALAR, INT/TO_DEVICE_ARRAY, INT/FROM_DEVICE_ARRAY, BOOLEAN/...
- DI-81 [REPLACED] Test plan vs tests: tests/src/testTangoServerCommand.cpp:34 readIntFromDeviceScalar() and :48 pollIntFromDeviceScalar() read the Tango attribute INT_FROM_DEVICE_SCALAR, but the specification excludes INT/FROM_DEVICE_SCALAR from the import because it is a command returnValue of ArgumentAndReturnValue / TriggerAndArgument / InvalidReturnValue. This makes testArgumentAndReturnValue (:109) and testNoReturnValue (:229) fail with a Tango DevFailed. The Test plan already prescribes t...
- DI-82 [REPLACED] DI-80 (Test plan gap): the Test plan never states the attribute count of tests/serverTestCommand_ds-AttributeMapper.xml once the command-source exclusion fires. Its <import>/</import> maps the household's 87 process variables except the seven command sources of the file (INT/TO_DEVICE_SCALAR, INT/FROM_DEVICE_SCALAR, INT/TO_DEVICE_ARRAY, INT/FROM_DEVICE_ARRAY, BOOLEAN/TO_DEVICE_ARRAY, BOOLEAN/FROM_DEVICE_ARRAY, VOID/TO_DEVICE_SCALAR), so 80 process variables plus the built-in ....
- DI-83 [DONE] DI-81 (test-side deviation from a correct plan): tests/src/testTangoServerCommand.cpp:34 readIntFromDeviceScalar and :48 pollIntFromDeviceScalar read the Tango attribute INT_FROM_DEVICE_SCALAR, but per the Requirements (CR-003:102-104) that source is skipped by the import because INT/FROM_DEVICE_SCALAR is a returnValue of ArgumentAndReturnValue, TriggerAndArgument and InvalidReturnValue (tests/serverTestCommand_ds-AttributeMapper.xml:11,17,30), so the attribute no longer ex...
- DI-84 [DONE] Specification gap: the Specifications never state which name form a mapper file uses for a source, nor that a source name is normalised to the process variable name form before it enters `_commandSources`, `_usedInputVariables`, `attributeToSource` and the source comparisons. The Requirements' attribute/command exclusion and the derived command name both depend on that comparison. The repository uses both forms - `tests/serverTestCommand_ds-AttributeMapper.xml:11,17`, `tests/...
- DI-85 [DONE] Specification states the wrong mechanism for a requirement: the bullet on the auto-mapping branch of `import()` says an explicit `<attribute>` naming a write-only `ChimeraTK::Void` process variable is rejected 'because such a source is a command source and `addAttribute()` throws for a command source'. `_commandSources` only holds the sources of the file's `<command>` elements, so for `tests/rejectCommandVoidAttribute/testTangoServer-AttributeMapper.xml` (an explicit `<attrib...
- DI-86 [DONE] Test plan gap in the folded DI-74: `testDefaultCommandName` requires commands declared without a `name`, but the module mapper the plan describes declares all seven commands with an explicit `name` (tests/serverTestCommand_ds-AttributeMapper.xml:11-31) and the plan does not say which name-less commands the mapper gains, nor that they must reuse one of the seven command sources on which the stated count of 82 attributes depends. The mapper bullet of the Test plan has to carr...
- DI-87 [DONE] include/Command.h:8 and include/CommandProperty.h:5: clang-tidy-22 reports misc-include-cleaner 'included header tango.h is not used directly'. The two new headers pull in the whole Tango header although every declaration they use comes in through another of their direct includes (CommandProperty.h for Command.h). Either include the specific Tango header the declarations need, or keep the include and silence the finding on that line with a NOLINT comment plus a one-line rea...
- DI-88 [DONE] tests/src/testTangoServerCommand.cpp:13 and :15: clang-tidy-22 reports misc-include-cleaner for <tango/tango.h> and for <boost/test/included/unit_test.hpp>. The two includes are the repository's test-file convention (every other test file has them) and the boost test header is the one the project ignores globally, but the 'included/' variant escapes the pattern misc-include-cleaner.IgnoreHeaders: 'boost/test/unit_test.hpp;' in .clang-tidy. Fix the file (or extend the ignore...
- DI-89 [DONE] tests/src/testTangoServerCommandOption.cpp:8 and :10: same two misc-include-cleaner findings as the module above.
- DI-90 [DONE] tests/src/testTangoServerCommandOptionRoot.cpp:8 and :10: same two misc-include-cleaner findings as the module above.
- DI-91 [DONE] tests/src/testTangoServerCommandOptionServer.cpp:13 and :15: same two misc-include-cleaner findings as the module above.
- DI-92 [DONE] tests/src_lib/TangoTestServer.cc:8: clang-tidy-22 reports misc-include-cleaner 'included header wait.h is not used directly' for the <sys/wait.h> include added with runTestTangoServer(). The line 189 WIFEXITED/WEXITSTATUS use makes this a false positive of the tool (the macros are attributed to another direct include), which the no-warnings policy still requires to be silenced on that line or by a global configuration entry.
- DI-93 [DONE] The deferred items DI-58 to DI-61 are recorded DONE, but their findings are only partly fixed: the removable includes (<memory>, <vector>, <string>, TangoTestServer.h) are gone, while the 'tango/tango.h' and 'boost/test/included/unit_test.hpp' findings they list are still produced by clang-tidy-22 on the same files.
- DI-94 [DONE] tests/src/testTangoServerCommand.cpp:41 and :43: a stray duplicated chrome bar (a bar, a blank line, a second bar) with no code between them, a leftover of an edit (a helper was evidently removed from there). grep over the whole repository shows this bar/blank/bar pattern in no other file; cpp-style reserves consecutive bars for big separations and puts no blank line between them, so one of the two bars has to go and the section separator becomes the single bar the file uses ...
- DI-95 [DONE] src/AttributMapper.cpp:58 is 121 columns, over the 120-column limit of the comment-wrapping rule (cpp-style 'Wrap comments at the same column limit as code, 120'). clang-format-19 does not rewrap it (clang-format-19 --dry-run -Werror passes and formatting the file leaves the line unchanged), so the comment has to be rewrapped by hand.
- DI-96 [DONE] src/AttributMapper.cpp:561 is 121 columns, over the 120-column limit of the comment-wrapping rule; same as the line above, clang-format-19 does not rewrap it and it needs a manual rewrap.
- DI-97 [DONE] src/AttributMapper.cpp:562 is 121 columns, over the 120-column limit of the comment-wrapping rule; same as the lines above, clang-format-19 does not rewrap it and it needs a manual rewrap.
- DI-98 [DONE] CR-003 must not change .clang-tidy or .gitignore - the user makes those changes themselves. The branch currently changes .clang-tidy (it appends 'boost/test/included/unit_test.hpp;tango/tango.h;' to misc-include-cleaner.IgnoreHeaders, .clang-tidy:345-348) and .gitignore (it appends 'Testing/', .gitignore:34-35). Revert both changes out of CR-003. Because the IgnoreHeaders entry is what silenced the include-header findings, the findings it covered have to be resolved per fil...
- DI-99 [DONE] Rebase the whole CR-003 branch on top of the CR-002 branch. The branch does not sit on CR-002: its first commit fd0ee6b (the CR document) is based on CR-001's head e18bc0a, and the branch later carries the merge commit d616721 'fixup! CR-003: Map process variables to Tango commands' whose second parent is CR-002's head 428b1ab (= origin/jgeorg-agent/CR-002-tango-eventing). That merge drags CR-002's artefacts (e.g. tests/src/testTangoServerEventing.cc, tests/serverTestEventing...
- DI-100 [DONE] Note for the re-run (factual, no extra instruction): after the rebase CR-003's diff relative to the CR-002 tip still modifies three CR-002 artefacts, which DI-1 records as the expected 'both change requests edit the same files': tests/src/testTangoServerEventing.cc (its local runTestTangoServer helper was replaced by the shared one), tests/include_lib/TangoTestServer.h (gains the shared runTestTangoServer declaration, + <filesystem>) and tests/serverTestEventing_ds-Attribute...
- DI-101 [DONE] src/Command.cpp: clang-tidy-22 reports clang-analyzer-security.ArrayBound 'Out of bound access to memory after the end of the heap area' in install-debug/include/omniORB4/seqTemplatedecls.h:75 for the new Command::insertArray() path, reached from src/Command.cpp:249 -> :276 -> insertArray -> data->length() at src/Command.cpp:323 -> the write (*data)[i] at src/Command.cpp:325. It is a false positive (the _CORBA_Sequence(max) constructor leaves pd_len 0, and data->length(n) ...
- DI-102 [DONE] Working tree confirms no auto-applied changes and no leftover experimental edits: the two edits tried while diagnosing the ArrayBound finding (dropping data->length() and the NOLINTNEXTLINE) were reverted; git status is clean.
- DI-103 [DONE] Documentation gap (user point 1): `doc/example-AttributeMapper.xml` and `doc/mainpage.dox` show only a trigger-plus-argument command (`moveToPosition`) and a trigger-only command (`stop`); they lack an example of a command with no explicit `trigger` which is triggered through its `inputArgument`, although the Requirements ('a command may omit it', 'without a `trigger` the input argument write is the trigger', CR-003 Requirements 'Aspect: configuration of a command' and 'As...
- DI-104 [DONE] Design question to settle (user point 2): whether the return value should be waited for on the continuation of the return-value process variable's queue instead of the adapter's own per-command condition variable (the wait state registered via `TangoUpdater::addVariable()` at `src/AdapterDeviceImpl.cpp:489`, notified in the callback and waited on in `waitForCommandReturnValue()` at `src/AdapterDeviceImpl.cpp:515`), and whether that interferes with the update mechanism. The S...
- DI-105 [DONE] Code duplication (user point 3): the Tango<->adapter type conversion in the new `src/Command.cpp` repeats the read/write/update path. `src/Command.cpp:21-48` defines the anonymous-namespace `scalarFromTango`/`scalarToTango` helpers and `src/Command.cpp:135` dispatches per Tango type, while the attribute, state and status paths share a single helper `convertToTangoType<TangoType, AdapterType>` (`include/TangoPropertyHelper.h:160`, used by `include/ScalarAttribTempl.h:90,110...
- DI-106 [DONE] Affected-components list incomplete after the DI-105 fold. 'Aspect: the Tango command object' (CR-003:254-261) adds a from-Tango conversion helper next to the existing `convertToTangoType<TangoType, AdapterType>` (`include/TangoPropertyHelper.h:160`), so the change edits `include/TangoPropertyHelper.h`; but the affected-components list (CR-003:130-133) names only `include/Command.h`, `src/Command.cpp`, the mapper, the device class, the device instance, `xmlschema/tango_varia...
- DI-107 [DONE] Affected-components list still incomplete: it does not name `include/CommandProperty.h`, the new header this change adds (git: added by commit 5784bde, absent from `master` and from the CR-002 tip; DI-65 and DI-87 call it the header `include/Command.h` uses). DI-106 fixed exactly this class of omission for `include/TangoPropertyHelper.h` (CR-003:131-133) but the equally new `include/CommandProperty.h` was left out.
- DI-108 [DONE] Overlooked side effect: the change modifies the shared test application helper `tests/include_lib/ExtendedReferenceTestApplication.h` and `tests/src_lib/ExtendedReferenceTestApplication.cc` (the `VariableHolder` tuple gains a `description` element, commit 221ac29, consumed by `tests/src/testTangoServerCommand.cpp:35`), yet neither file is named anywhere in the CR - not in the affected components (CR-003:131-137) and not in the test-asset list (CR-003:138-148). The ripple als...
- DI-109 [DONE] Rework the commits of the CR-003 branch into logical, easier to review units: replace the current history (the CR-document commit fd0ee6b plus a long chain of 20+ `fixup! CR-003: Map process variables to Tango commands` commits) with self-contained commits that each carry one coherent part of the change (for example: mapper/command data model and type mapping, the Tango command object and the conversion helper, device class and device instance wiring, schema and documentation, ne...
- DI-110 [DONE] User decision after three clarifying questions: make the Tango mapper's command/attribute exclusion behave like the DOOCS adapter and drop the extra tree walk. Concretely: remove AttributeMapper::collectCommandSources() (src/AttributMapper.cpp:710-733) and its call in readMapperFile (src/AttributMapper.cpp:761) together with the file-wide set AttributeMapper::_commandSources (include/AttributeMapper.h:108), and decide the exclusion in the single document-order pass with fi...
- DI-111 [DONE] Test plan asset bullet (doc/change-requests/CR-003-tango-commands.md:554-566) is stale after the DI-110 fold and omits the edit the new first-come semantics require. It says tests/serverTestCommand_ds-AttributeMapper.xml 'drops its second device instance, the autoMapCommandsFromVoid overrides and the explicit <attribute>, and gains <import>/</import>' and that tests/testCommandTwoDevices_template.db (CMakeLists.txt:109) and tests/rejectCommandOnlyReturnValue (CMakeLists.txt:...
- DI-112 [DONE] Rejection-order coverage gap in the Test plan (doc:583-585). The Requirements (doc:107-111) make the attribute/command exclusion symmetric ('whichever of <attribute> and <command> names the source first wins and the other is rejected when the mapper file is read') and the Specifications (doc:200-202) define two distinct checks (addAttribute() rejects a source an earlier command of the instance declared; addCommand() rejects a source an earlier attribute of the instance mappe...
- DI-113 [DONE] include/AttributeMapper.h:39-40: the comment above the member 'bool autoMapCommandsFromVoid{true}' still reads 'The write-only ChimeraTK::Void sources this instance auto-maps to commands, so import() can tell whether the option is still in force for the instance it maps into.' DI-110 removed that per-instance source set, so the comment describes a set that no longer exists next to a bool. Reword it the way the sibling members do (include/AttributeMapper.h:51 for deviceClas...
- DI-114 [DONE] tests/src/testTangoServerCommand.cpp:186: the docstring 'The <import> element before the <command> elements leaves a command source unmapped as an attribute' states the opposite element order of the mapper it runs against. tests/serverTestCommand_ds-AttributeMapper.xml now declares the <command> elements (lines 10-40) before <import>/</import> (line 41), and its own header comment says so; the docstring must be reworded to the commands-before-import, first-come order.
- DI-115 [DONE] A command source is still called a source 'of the file' in tests/serverTestCommand_ds-AttributeMapper.xml:37 and tests/src/testTangoServerCommand.cpp:97, :190, :195 and :258 although DI-110 made the attribute/command exclusion instance-local, and the CR Test plan (doc/change-requests/CR-003-tango-commands.md:561-562) explicitly requires the mapper comment to be reworded from the file-wide wording to the instance-local one. Reword all five comments to the device instance.
- DI-116 [REPLACED] Flaky test with a specification-level root cause: testTangoServerCommand / testArgumentAndReturnValue fails intermittently (about 1 in 10 to 1 in 20 full parallel `ctest -j16` runs; reproduced repeatedly) with `testTangoServerCommand.cpp(94): error: in "testArgumentAndReturnValue": check returned == 17 has failed [0 != 17]`. The call returns the stale initial value of the return-value process variable instead of the value it sent.
- DI-117 [REPLACED] Root cause (measured by temporary instrumentation, since reverted): the command waits for the return-value process variable to carry a version number DIFFERENT from the snapshot taken before its writes (src/Command.cpp:56-58 snapshot, src/AdapterDeviceImpl.cpp:519-520 predicate). The reference test application writes INT/FROM_DEVICE_SCALAR from TWO independent sources in ReferenceTestApplication::TypedPVHolder::inputToOutput: the command's argument INT/TO_DEVICE_SCALAR and th...
- DI-118 [REPLACED] This is a specification problem, not an implementation deviation: Requirements (CR-003 line 72, 'The call blocks until the process variable of `returnValue` carries a new version number, and returns its value') and Specifications (CR-003 lines 316-318, 'takes the version number of the return accessor before the writes, waits on the wait state until the version number differs') define exactly the condition that src/Command.cpp implements. A correct condition has to require the...
- DI-119 [REPLACED] Same latent race affects every other command test that waits for a return value driven through the reference application's FROM_DEVICE_* coupling (testTriggerAndArgument, testArrayArgumentAndReturnValue, testBooleanArrayArgumentAndReturnValue, testInvalidReturnValue): all use INT/FROM_DEVICE_SCALAR / INT/FROM_DEVICE_ARRAY, which the application also refreshes from INT/BIDIRECTIONAL, so any of them can return or validate a stale value under load. The Test plan does not mention...
- DI-120 [DONE] Timeout in this environment: a full `ctest -j16` run plus repeated stress runs exceeded the 1200-1500 s tool budget several times, so the flakiness measurement was done in batches of 20 in 20-25 runs each; the suite itself needs roughly 16 s per full run and the other 14 tests plus coding_style pass.
- DI-121 [DONE] Wait condition defect confirmed, specification-level (DI-116, DI-117): Requirements (doc/change-requests/CR-003-tango-commands.md:72, 'The call blocks until the process variable of `returnValue` carries a new version number, and returns its value') and Specifications (doc:315-320, 'takes the version number of the return accessor before the writes, waits on the wait state until the version number differs') prescribe exactly the condition src/Command.cpp:54-58 and src/Adapte...
- DI-122 [DONE] Correction to fold (DI-118, user decision: 'Require the answer to post-date the command's own write'): the call has to wait for the return-value process variable to carry a version number newer than the write the command itself performed. Requirements, Specifications and the execute()/wait path (src/Command.cpp:54-58 and :86, src/AdapterDeviceImpl.cpp:515-520, include/AdapterDeviceImpl.h:59) have to carry that condition instead of the pre-write snapshot and the 'differs' p...
- DI-123 [DONE] Test plan gap (DI-119): the plan never mentions that the reference application drives INT/FROM_DEVICE_SCALAR and INT/FROM_DEVICE_ARRAY from the command argument and from the unrelated INT/BIDIRECTIONAL, so testTriggerAndArgument, testArrayArgumentAndReturnValue, testBooleanArrayArgumentAndReturnValue and testInvalidReturnValue are exposed to the same race. The plan has to state the coupling, why the corrected condition makes the command tests insensitive to it, and which a...
- DI-124 [DONE] New requirement statement needed for the corrected condition: the comparison is between version numbers of the application and of the adapter, and ChimeraTK version numbers are per-process counters (install-debug/include/ChimeraTK/VersionNumber.h:81-93), so the ordering holds only while the application runs in the adapter's process (as in the in-process fixture); for a separate application process the answers are stamped by the application's own counter. The plan has to ca...
- DI-125 [DONE] Specifications still describe the wait state as carrying the return value's version number, contradicting the corrected wait condition of DI-122: 'Aspect: device instance' (CR-003:284-286, 'a wait state, which is a mutex, a condition variable and the version number of the return value') and 'Aspect: execution' (CR-003:341-343, 'The wait state is the adapter's own per-command mutex, condition variable and version number') both let the wait state hold a version number, while...
- DI-126 [DONE] Requirements (CR-003:74-76) and Specifications (CR-003:348-350) overstate what the corrected condition achieves: 'an unrelated update of the return-value process variable, for example a refresh from another source of the application, does not end the wait' / 'a notification of an unrelated update does not end it'. Any update whose version post-dates the command's own write does end the wait, whatever its source, so an application refreshing the return-value process variabl...
- DI-127 [FOLDED-IN] Extra behaviour in `AdapterDeviceImpl::attachToClassCommands()` (src/AdapterDeviceImpl.cpp:477-482): it registers the `trigger` and `inputArgument` accessors with `TangoUpdater::addVariable()` (`get_name()+"/"+commandName+"/trigger"` and `.../inputArgument`), in addition to the return-value registration. The Specifications ('Aspect: device instance', CR-003:292-299) describe only the return-value registration (`.../returnValue` plus the notifying callback); the trigger and inputAr...
- DI-128 [FOLDED-IN] Documentation does not show the 'trigger with an argument' command form that the Specifications claim. 'Aspect: schema and documentation' (CR-003:381-384) says `doc/example-AttributeMapper.xml` shows 'a command with a `trigger` and an argument whose argument is written before the trigger', but the file (doc/example-AttributeMapper.xml:21-26) has `moveToPosition` (argument + return value, no trigger), `stop` (trigger only) and an argument-only command; no command carries both a `tr...

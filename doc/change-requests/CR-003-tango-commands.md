# CR-003: Map process variables to Tango commands

Synopsis: Let the AttributeMapper map process variables to Tango commands with
one optional input argument and one optional return value. A write-only
`ChimeraTK::Void` process variable which the mapper imports becomes an
argument-less command without a return value by default, instead of the boolean
attribute of today.

Depends on: CR-002, which is merged into master; CR-003 sits on master. Its
test plan uses the mapper-read test infrastructure CR-002 introduces: the
`withMapperFileInCaseDirectory` helper of `tests/include_lib/TangoTestServer.h`
and the mapper-text helpers of `tests/src/testTangoAttributeMapper.cc`. Both
change requests edit the same files. CR-003's change set carries no CR-002
artefact of its own.

Status: TESTS REVIEWED (from DONE)

## Requirements

Aspect: commands.

- A device instance declares commands in the mapper file with a new `<command>`
element.
- A command may take one input argument and may return one value.
- A write-only `ChimeraTK::Void` process variable which the mapper imports
becomes an argument-less command without a return value.
- A readable `ChimeraTK::Void` process variable still becomes an attribute.
- An explicit `<attribute>` naming a write-only `ChimeraTK::Void` process
variable keeps mapping the boolean attribute of today, whatever
`autoMapCommandsFromVoid` is. Only the element-order exclusion below rejects it,
when a `<command>` of the same device instance claimed the source first.

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
write-only `ChimeraTK::Void` process variable which an `<import>` maps becomes a
boolean attribute again instead of a command. The option does not affect an
explicit `<attribute>`.
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
`tests/testCommandOption_template.db`. The shared test server helper
`tests/include_lib/TangoTestServer.h` gains the `mapperFileBody()` and
`withMapperFileBodyInCaseDirectory()` overloads the rejection cases with more
than one device instance use. The shared test application helper
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
boolean attribute of today when `autoMapCommandsFromVoid` is `false`. The option
gates that branch alone: an explicit `<attribute>` naming a write-only
`ChimeraTK::Void` process variable still maps the boolean attribute of today,
whatever the option is, and only the element-order exclusion rejects it when an
earlier `<command>` of the same device instance declared the source.
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
`convertToTangoType<TangoType, AdapterType>` (`include/TangoPropertyHelper.h:158`),
and the return path calls
`convertToTangoType<TangoType, AdapterType>` itself, the helper the attribute read
path (`include/ScalarAttribTempl.h:105`, `include/SpectrumAttribTempl.h:96`)
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
(`src/AdapterDeviceImpl.cpp:315` for state and `:357` for status). The updater thread runs all callbacks of
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
`waitForCommandReturnValue()` (`src/AdapterDeviceImpl.cpp:502`) takes the
write version instead of the pre-write snapshot and `execute()` passes it
(`src/Command.cpp:54-58`, `:86`).
- The wait state is the adapter's own per-command mutex and condition variable,
and the command waits on that condition variable
(`AdapterDeviceImpl::waitForCommandReturnValue()`, `src/AdapterDeviceImpl.cpp:502`, body `:503-508`).
It is notified by the callback which `attachToClassCommands()` passes to
`TangoUpdater::addVariable()` for the return-value accessor
(`src/AdapterDeviceImpl.cpp:476`), not by the return-value process variable's own
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
type restrictions and the blocking behaviour. It states that an explicit
`<attribute>` naming a write-only `ChimeraTK::Void` process variable is allowed
whatever the option is. It documents the three command
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
entries, so the tests find them. The existing
`tests/serverTestCommand_ds-AttributeMapper.xml` is already the single-instance
form the module needs; its `<import>/</import>` element moves after the
`<command>` elements, so the import skips the sources the instance has already
mapped as commands, and the mapper comment is reworded from the file-wide
"a command source of this file" rationale to the instance-local, first-come
one.
- Rejections when the mapper file is read, each with its own mapper file,
asserted by the in-process mapper test CR-002 uses: build an
`AttributeMapper`, set the control-system process-variable manager of the
running in-process server on it, read the mapper file written for the case with
`withMapperFileInCaseDirectory` (or `withMapperFileBodyInCaseDirectory` for the
case with more than one device instance) and assert that the read throws
`ChimeraTK::logic_error`. No server and no subprocess is started, because the
`TangoAdapter::run` of a server would abort the test binary on the escaping
error. The cases are:
- a command without `trigger` and without `inputArgument` (for example one
with only a `returnValue`), so the `name` derivation has no source;
- an unsupported value type (`uint8_t` scalar, `int8_t` scalar, `int8_t` array);
an image cannot occur, because the ControlSystemAdapter delivers single-channel
process arrays, so it stays untested like the image rejection of CR-002.
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
rejected.
- the same command name with different types in two device instances;
- a command named `State`, `Status` or `Init`.
- The positive counterpart of those rejections: an explicit `<attribute>`
naming the write-only `ChimeraTK::Void` process variable `VOID/TO_DEVICE_SCALAR`
is accepted whatever `autoMapCommandsFromVoid` is, and maps the boolean
attribute of today. The test writes the mapper file for a single device instance
which carries the `<attribute>` and sets the option to `true` and to `false`
with `withMapperFileBodyInCaseDirectory`, and asserts the read succeeds and the
attribute's Tango type is `DevBoolean` (the type `getDataType()` derives from the
raw `DevVoid` property).
- The change set carries no `runTestTangoServer` helper, no mapper assets under
`tests/rejectCommand*` and no `file(COPY ...)` entries for such assets: the
mapper-read rejection cases above need neither a separate process nor a
directory of their own, because the mapper test writes and reads the mapper file
itself.
- The existing tests which rely on the boolean attribute of a write-only
`ChimeraTK::Void` process variable have to follow the new mapping:
`testTangoServerImportAll` (its attribute count and the void process variables),
`testTangoAttributeMapper` (its attribute count, which drops from 90 to 89) and
`testTangoServerReadWrite` (its `Void` instantiation of the scalar test). The
void-attribute case of the mapper-read rejection tests goes away, because the
read accepts the mapper file it writes now.
- `testTangoServerMapTwice` is unaffected: two attributes which map one source
stay supported, verified by its five attributes and both renames.
The mapper of `testTangoServerEventing`,
`tests/serverTestEventing_ds-AttributeMapper.xml`, maps
`VOID/TO_DEVICE_SCALAR` with an explicit `<attribute>` and keeps no
`autoMapCommandsFromVoid`, so the option is at its default: the mapper read
accepts the `<attribute>` and the CR-002 eventing tests, which write it as a
`Tango::DevBoolean`, cover the allowance.
- The full `ctest` run of the project, including the `coding_style` test.

## Deferred issue

- DI-1 [DONE] CR-002 was reworked and merged into master after CR-003 was created; CR-003 was rebased onto master and its change set carries no CR-002 artefact of its own.
- DI-2 [DONE] Specifications line-number citations for src/AdapterDeviceImpl.cpp are stale relative to the current code base (they predate the rebase that moved the file's lines up by ~13): '(:515-520)' and '(:515)' name the wait on waitForCommandReturnValue(), which is now at :502 (body :503-508); '(:489)' names the return-value TangoUpdater::addVariable() registration, now at :476 (a blank/'}' line sits at :489).
- DI-3 [DONE] include/ScalarAttribTempl.h:90,110 is cited for the convertToTangoType() read-path call, but in the current code :90 is an assert() and :110 is a set_quality() call; the only remaining convertToTangoType() call in that file is at :105.
- DI-4 [DONE] Minor: include/TangoPropertyHelper.h:160 is cited for the convertToTangoType<TangoType,AdapterType> declaration, which is now at :158; src/AdapterDeviceImpl.cpp:346-358 is cited for the state and status callbacks, which are now registered at :315 (state) and :357 (status).
- DI-5 [DONE] An explicit `<attribute>` naming a write-only `ChimeraTK::Void` process variable keeps mapping the boolean attribute of today, whatever `autoMapCommandsFromVoid` is; only the element-order exclusion rejects it.
- DI-6 [DONE] The CR-003 document's 'Deferred issue' section is truncated mid-sentence: DI-1 (line 658) ends with '...merged into master after CR-003 was' and DI-5 (line 662) ends with '...An explicit `<attribute>` naming a write-only', which is also the last line of the file (verified with od -c, file ends at 'write-only\n'). DI-5's entry is therefore incomplete and cannot be verified as written, so it cannot be confirmed resolved.

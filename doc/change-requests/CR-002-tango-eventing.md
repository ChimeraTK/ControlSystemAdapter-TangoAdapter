# CR-002: Tango events for mapped attributes

Synopsis: Let the AttributeMapper enable Tango event generation per attribute.
Support DATA_READY and CHANGE events; the default stays off.

Depends on: CR-001

Status: DONE

## Requirements

Attributes currently only answer client reads. Event generation is not offered.

- An `<attribute>` may carry a new child element `<eventing>`.
- The element text is one of `dataReady`, `change`, `off`.
- An absent `<eventing>` element means `off`, so existing mappers keep their
current behaviour.
- `dataReady` makes the attribute emit Tango DATA_READY events.
- `change` makes the attribute emit Tango CHANGE events.
- The two event kinds are mutually exclusive per attribute.
- Any other element text is rejected when the mapper file is read; the server
fails to start.
- Eventing applies to readable scalar and spectrum attributes.
- Eventing on a write-only attribute is rejected when the mapper file is read.
- `change` on a readable `ChimeraTK::Void` process variable is accepted. The
readout time stamp of such an event is the only way to tell a client when the
variable was produced, because a Tango DATA_READY event carries a counter and no
time stamp. Tango has no attribute type for a Void register, so the event's value
is the constant `true` that the attribute's read path reports.
- Void attributes accept all three values.
- Image attributes are not implemented and stay unsupported by this change.
- Image attributes cannot occur today: the ControlSystemAdapter delivers
single-channel process arrays, so a mapped attribute is always scalar or
spectrum.
- Once image attributes exist, DATA_READY eventing must be supported for them.
- Both kinds emit on every new version of the attribute's process variable,
even when its value and quality are unchanged: a repeat is an update as well.
- The two kinds differ in their payload, not in when they fire: `change` carries
the attribute's value and quality, `dataReady` the DATA_READY counter.

## Specifications

- `AttributeProperty` gains an `EventingMode` enum with values `Off`,
`DataReady` and `Change`, stored in a new member defaulting to `Off`.
- `EventingMode` gets an `operator<<` next to the existing `AttrDataFormat`
printer in `AttributeProperty.h`, so the mapper tests can compare the parsed
mode with `BOOST_TEST` and print both values on failure.
- `AttributeMapper::processAttributeNode` reads the `<eventing>` child content
and maps it to the enum. An unknown or empty value throws
`ChimeraTK::logic_error` naming the attribute and the offending value.
- `AttributeMapper::addAttribute` rejects `DataReady`/`Change` when the process
variable is not readable and rejects both when the data format is image. The image
check stays as a defensive guard: it cannot trigger today, because the
ControlSystemAdapter delivers single-channel process arrays, so the data format
is never image.
- `AttributeMapper::processAttributeNode` passes the parsed eventing mode to
`addAttribute`, which stores it in the `AttributeProperty` it creates (the
`AttributeProperty` constructor gains the mode, or `addAttribute` sets the
member after construction). The `ScalarAttribTempl`/`SpectrumAttribTempl`
constructors therefore read the requested mode from `AttributeProperty::eventing`,
and the `addAttribute` rejection check uses the passed mode.
- `ScalarAttribTempl` and `SpectrumAttribTempl` constructors configure the Tango
attribute from `AttributeProperty::eventing`: `set_change_event(true, false)`
for `Change`, `set_data_ready_event(true)` for `DataReady`.
- `AdapterDeviceImpl::attachToClassAttributes` passes a callback to
`TangoUpdater::addVariable` for every attribute whose `eventing` is not
`Off`.
- The callback runs on the TangoUpdater thread, which already delivers the
state/status callbacks.
- `Change` pushes a fresh read, because `push_change_event(attrName)` alone
pushes only the value stored in the attribute object, not a new read.
- The callback reads the attribute's process variable through
`AdapterDeviceImpl::getPvForAttribute` and switches on the attribute's
`AttributeProperty::dataType` (the raw `Tango::CmdArgType`), because
`DeviceImpl::push_change_event` is a type template while the callback is not.
The switch keeps `DEV_ENUM` and `DEV_VOID` distinct from `DEV_SHORT` and
`DEV_BOOLEAN`; `AttributeProperty::getDataType` is not used, as it collapses
`DEV_ENUM` to `DEV_SHORT` and `DEV_VOID` to `DEV_BOOLEAN`.
- Each branch builds the Tango-typed buffer the matching `push_change_event`
overload expects, mirroring the read path of
`ScalarAttribTempl`/`SpectrumAttribTempl`.
- The `DEV_VOID` case uses the common helper with the Tango type `Tango::DevBoolean`
and the adapter type `ChimeraTK::Void`, because a Void attribute is registered as
`DEV_BOOLEAN` (`AttributeProperty::getDataType`) and its read path reports the
constant `true`. A Void register carries no sample, so the branch of the helper
that does not read a value pushes that constant; the quality and the readout time
stamp are produced for it exactly as for every other type. The helper asserts at
compile time that a Void attribute pushes a `Tango::DevBoolean`.
- The Void case is an `if constexpr` branch of the common helper instead of a
helper of its own, so the quality, the time stamp and the push stay in one place.
- Every other `dataType` case instantiates the same Tango/AdapterType pair as
`util::toScalarTangoAttribute`/`util::toSpectrumTangoAttribute` and casts the
accessor with `dynamic_pointer_cast`, so that the check for a failed cast can
trigger: only a dynamic cast yields an empty pointer for a type mismatch. No
runtime `callForTypeNoVoid` is used, because `push_change_event` needs the type
at compile time.
`AttributeProperty::getDataType` is not used, as it collapses `DEV_ENUM` to
`DEV_SHORT`.
- The elements are converted individually instead of reinterpreted: `int8_t` to
`Tango::DevShort`, `std::string` to `Tango::DevString` through
`Tango::string_dup`, and every other type copied as is.
- This element conversion is shared with the read path instead of being
duplicated per path: one helper function template converts a single
`AdapterType` sample to its `TangoType`. `ScalarAttribTempl::read`,
`SpectrumAttribTempl::read` and `detail::pushChangeEventForAttribute` all call
it. It lives in a shared header, so both the attribute templates and
`AdapterDeviceImpl.cpp` can include it.
- A `Change` push allocates that buffer with one element for a scalar and one
element per sample for a spectrum, fills it, and passes it with the release
flag set so Tango takes ownership.
- The pushed `Change` event mirrors the read path's quality through the
quality-carrying `DeviceImpl::push_change_event` overload: `ATTR_INVALID` when
the process variable's data validity is not `ChimeraTK::DataValidity::ok` and
`ATTR_VALID` otherwise. That overload also needs a readout time stamp, taken
from the process variable. DeviceAccess exposes no other time stamp.
DATA_READY events carry no quality.
- `DataReady` pushes with `DeviceImpl::push_data_ready_event(attrName,
counter)`.
- Each attribute carries its own DATA_READY counter in `AdapterDeviceImpl`,
incremented before each push, so the first event of an attribute carries 1. The
counter advances only when the process variable's version
number differs from the version number of the last push for that attribute; a
push for a version number already pushed carries the same counter.
`ChimeraTK::VersionNumber` exposes no numeric value, so `AdapterDeviceImpl`
remembers the last version number pushed per attribute.
- The `Change` callback depends on the attribute's data format: it pushes a
scalar value as one element and a spectrum value as an array of the
attribute's length. The `DataReady` callback takes the attribute name, looks
the process variable up with `AdapterDeviceImpl::getPvForAttribute` to obtain
its version number, and needs no data-format knowledge, so a future image
attribute implementation needs only a DATA_READY counter and the same callback
registration to support DATA_READY events.
- `xmlschema/tango_variable_tree.xsd` gets the `<eventing>` element and
`doc/example-AttributeMapper.xml` shows all three values: `change` on an
attribute whose source value changes (the position source `/Motors/position`),
`dataReady` on an attribute the client must be told about whether or not its
value changed (a status word source), `off` on `numberOfMotors`, which never
changes, and `change` on a `ChimeraTK::Void` source (the new-data trigger
`/Motors/newDataTrigger`), whose event carries the time the trigger was produced.
- `doc/mainpage.dox` describes the keyword, the three values, the difference
between the two kinds (their payload; both emit on every update) and the rejection
of an unknown value and of a write-only attribute; it keeps the statement that
image attributes are not implemented. It no longer restricts an eventing mode to a
value type of the process variable, and it does not describe what a `ChimeraTK::Void`
CHANGE event carries. It must not claim that an image attribute is rejected when
the mapper file is read, because that case cannot occur.

### Alternatives considered

- `<eventing>` as an XML attribute of `<attribute>`: inconsistent with the
existing `<description>` and `<egu>` child elements.
- Pushing only when the value actually changed: needs per-attribute previous
value bookkeeping with unclear rules for spectra and quality.
- A device-class default eventing mode: not needed yet; per-attribute control
covers the use cases.

## Test plan

- Mapper with `dataReady`, `change` and `off` attributes, plus a write-only
attribute.
- Subscribe CHANGE and DATA_READY events through a `Tango::DeviceProxy`.
- The mapper of the first bullet also carries a spectrum attribute with `change`
and one with `dataReady`; assert the CHANGE event of the spectrum arrives with
all elements of the written array. For the spectrum `dataReady` attribute,
write a second array and assert the second DATA_READY counter is strictly
larger than the first, mirroring the scalar case in the bullet below.
- Write the process variables from the test application; assert CHANGE events
carry the written value and DATA_READY events an increasing counter. Every
write produces exactly one event: the helper writes the process variable once,
then repeats only a main loop iteration plus the condition check until the
event arrives. It never repeats the write, so an event cannot be paired with an
earlier write.
- The nested source location reaches the mapper and the event machinery: this is
proven by the CHANGE event carrying the value written to the mapped process
variable. The attribute name is not read back, because Tango sets
`DeviceAttribute::name` to the requested name, so such a read cannot fail and
verifies nothing.
- The attribute mapper is tested directly, without a server and without a
subprocess: build an `AttributeMapper` and give it the control-system process
variable manager of the running in-process server
(`TangoAdapter::TangoAdapter::getInstance().getCsPvManager()`) with
`setCSPVManager`, because the application itself exposes only a
`DevicePVManager` (`ApplicationBase::getPVManager`), and read a mapper file in
each case. Because `readMapperFile` derives the file name from the executable
name of `Tango::Util::instance()`, the test runs in the in-process fixture and
changes into a per-case directory which holds a mapper file carrying that name.
The per-case mapper files are written by the test code with the shared helper
`withMapperFileInCaseDirectory` of `TangoTestServer.h`, into a directory below
the working directory of the test executable, which the build system creates
fresh and which is therefore present in any checkout. Each case carries one
`<attribute>` element, written with the shared `attribute`/
`attributeWithEventing` helpers, so the test covers the attribute data
(description, egu, data format, data type, write type) together with the mode.
The rejection cases are: an unknown `<eventing>` value, an empty
`<eventing>` element and eventing on a write-only attribute, each expected to
throw `ChimeraTK::logic_error`. The
acceptance cases are the parsed mode for `dataReady`, `change`, `off` and an
absent `<eventing>` element (the last two both `Off`), `dataReady` and `change`
on a Void attribute, and the data-mapping cases: a scalar with
`<eventing>change</eventing>`, a spectrum with `<eventing>dataReady</eventing>`,
and a write-only attribute without `<eventing>`. Every case names the eventing
mode it carries in its doc comment, and compares the parsed mode with
`BOOST_TEST`, so a failure prints both values.
- The `runTestTangoServer` helper and the asset directories
`tests/rejectInvalidEventingValue` and `tests/rejectWriteOnlyEventing` are
removed, together with their CMake copies; the rejection cases are covered by
the mapper test bullet.
- The mapper also carries a readable `ChimeraTK::Void` attribute with
`dataReady`, and a second attribute on the same `ChimeraTK::Void` process
variable with `change`; write the matching to-device `ChimeraTK::Void` process
variable through the proxy and use the helper of the bullet above (one main loop
iteration plus the condition check, repeated until the event arrives), then
assert a DATA_READY event with an increasing counter arrives.
- The CHANGE event of that Void attribute carries the constant `true` and the
`ATTR_VALID` quality, and its time stamp is the readout time stamp of the
process variable: it lies between the time before the write and the time the
event arrived. A second write produces an event with a strictly later time
stamp, which a constant time stamp would not.
- Assert a CHANGE event of an attribute whose process variable carries faulty
data validity arrives with quality `ATTR_INVALID`.

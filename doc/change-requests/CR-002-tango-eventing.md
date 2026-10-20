# CR-002: Tango events for mapped attributes

Synopsis: Let the AttributeMapper enable Tango event generation per attribute.
Support DATA_READY and CHANGE events; the default stays off.

Depends on: CR-001

Status: PLANNED (from DONE)

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
- Image attributes are not implemented and stay unsupported by this change.
- Image attributes cannot occur today: the ControlSystemAdapter delivers
single-channel process arrays, so a mapped attribute is always scalar or
spectrum.
- Once image attributes exist, DATA_READY eventing must be supported for them.
- An event is emitted on each update of the attribute's process variable.

## Specifications

- `AttributeProperty` gains an `EventingMode` enum with values `Off`,
`DataReady` and `Change`, stored in a new member defaulting to `Off`.
- `AttributeMapper::processAttributeNode` reads the `<eventing>` child content
and maps it to the enum. An unknown or empty value throws
`ChimeraTK::logic_error` naming the attribute and the offending value.
- `AttributeMapper::addAttribute` rejects `DataReady`/`Change` when the process
variable is not readable, or when the data format is image. The image check
stays as a defensive guard: it cannot trigger today, because the
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
- `DEV_VOID` (a readable asynchronous `ChimeraTK::Void` register) has its own
case in this switch and a separate helper, which pushes a single constant
`Tango::DevBoolean true` with the accessor's data validity as quality and never
reads a value, as `ScalarAttribTempl::read` does. The generic branch cannot
handle it: a `ChimeraTK::Void` register carries no data to convert. The
matching comment in `src/AdapterDeviceImpl.cpp` should say the same instead of
referring to `ChimeraTK::callForTypeNoVoid`.
- Every other `dataType` case instantiates the same Tango/AdapterType pair as
`util::toScalarTangoAttribute`/`util::toSpectrumTangoAttribute` and casts the
accessor with `reinterpret_pointer_cast`. No runtime `callForTypeNoVoid` is
used, because `push_change_event` needs the type at compile time.
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
`ATTR_VALID` otherwise. That overload also needs a readout time stamp, filled
with the current time (`Tango::TangoTimestamp(std::chrono::system_clock::now())`).
DATA_READY events carry no quality.
- `DataReady` pushes with `DeviceImpl::push_data_ready_event(attrName,
counter)`.
- Each attribute carries its own DATA_READY counter in `AdapterDeviceImpl`,
incremented on every push and starting at 1.
- The `Change` callback depends on the attribute's data format: it pushes a
scalar value as one element and a spectrum value as an array of the
attribute's length. The `DataReady` callback still works on the attribute
name only, so a future image attribute implementation needs only a DATA_READY
counter and the same callback registration to support DATA_READY events.
- `xmlschema/tango_variable_tree.xsd` gets the `<eventing>` element and
`doc/example-AttributeMapper.xml` shows one. `doc/mainpage.dox` describes the
keyword, the three values, the rejection of an unknown value and of a
write-only attribute, and that image attributes are not implemented. It must
not claim that an image attribute is rejected when the mapper file is read,
because that case cannot occur.

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
carry the written value and DATA_READY events an increasing counter.
- Assert an `off` attribute produces no event.
- Assert an invalid `<eventing>` value and eventing on a write-only attribute
both fail at mapper read time. Assert each rejection with its own mapper file,
because the first error aborts the start-up: one mapper with an invalid
`<eventing>` value and one with `<eventing>` on a write-only attribute. Spawn
the `testTangoServer` executable as a subprocess with the server arguments the
in-process fixture uses (an instance name as `argv[1]`, `-nodb`, `-dlist
<device>` and an ORB end point), with the offending mapper file placed as
`testTangoServer-AttributeMapper.xml` in the subprocess working directory, and
assert a non-zero exit. Without the instance name `Tango::Util::init` aborts
before `readMapperFile`, so the assertion would not prove the rejection. The
mapper error escapes `TangoAdapter::run` uncaught, so an in-process server would
abort the test binary itself.
- The mapper also carries a readable `ChimeraTK::Void` attribute with `change`;
write it through the proxy, run one application main loop and assert the CHANGE
event arrives with the constant `true` and quality `ATTR_VALID`.
- Assert a CHANGE event of an attribute whose process variable carries faulty
data validity arrives with quality `ATTR_INVALID`.

## Deferred issue

- DI-1 [NEW] Void must only emit DATA_READY events: the `DEV_VOID` branch and `pushVoidChangeEvent` (src/AdapterDeviceImpl.cpp:96) push a CHANGE event carrying a constant `true`, which does not make sense.
- DI-2 [NEW] The specification gives `DEV_VOID` a case of its own in the CHANGE switch and a separate helper for it (doc/change-requests/CR-002-tango-eventing.md:70); Void must only be data_ready.
- DI-3 [NEW] An event for the same version number must carry the same counter; the specification's rule that the counter is incremented on every push (doc/change-requests/CR-002-tango-eventing.md:102) is wrong.
- DI-4 [NEW] `pumpUntilEvent` (tests/src/testTangoServerEventing.cc:126) repeats the write and one main loop iteration until any matching event arrives, so a random later write can be paired with a random earlier event. Every write must have exactly one event.
- DI-5 [NEW] `testOffAttributeProducesNoEvent` (tests/src/testTangoServerEventing.cc:269) tests Tango core functionality and is unnecessary.
- DI-6 [NEW] `testVoidChangeEventCarriesConstantTrue` (tests/src/testTangoServerEventing.cc:278) asserts a CHANGE event for a Void attribute; Void must only ever carry a DATA_READY event.
- DI-7 [NEW] `testInvalidEventingValueRejectedAtMapperRead` (tests/src/testTangoServerEventing.cc:330) is a full end-to-end subprocess test; the rejection of an invalid `<eventing>` value must be a unit test of the attribute mapper.
- DI-8 [NEW] `testWriteOnlyEventingRejectedAtMapperRead` (tests/src/testTangoServerEventing.cc:339) is a full end-to-end subprocess test; the rejection of eventing on a write-only attribute must be a unit test of the attribute mapper.
- DI-9 [NEW] `doc/example-AttributeMapper.xml:19` uses `numberOfMotors` for `change`, a really bad example for a changing value; the example must use a value that changes.

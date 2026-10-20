# CR-002: Tango events for mapped attributes

Synopsis: Let the AttributeMapper enable Tango event generation per attribute.
Support DATA_READY and CHANGE events; the default stays off.

Depends on: CR-001

Status: IMPLEMENTATION REVIEWED (from DONE)

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
- `change` on a readable `ChimeraTK::Void` process variable is rejected when the
mapper file is read, because a Void carries no value to compare. Void
attributes accept `dataReady` and `off` only.
- Image attributes are not implemented and stay unsupported by this change.
- Image attributes cannot occur today: the ControlSystemAdapter delivers
single-channel process arrays, so a mapped attribute is always scalar or
spectrum.
- Once image attributes exist, DATA_READY eventing must be supported for them.
- An event is emitted on each update of the attribute's process variable.

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
variable is not readable, rejects `Change` when its value type is
`ChimeraTK::Void`, and rejects both when the data format is image. The image
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
- The change switch has no `DEV_VOID` case: `Change` is rejected for a Void
process variable, so the generic branch and the `default:` error branch cover
every reachable type. `pushVoidChangeEvent` and its comment in
`src/AdapterDeviceImpl.cpp` are deleted.
- Every other `dataType` case instantiates the same Tango/AdapterType pair as
`util::toScalarTangoAttribute`/`util::toSpectrumTangoAttribute` and casts the
accessor with `reinterpret_pointer_cast`. No runtime `callForTypeNoVoid` is
used, because `push_change_event` needs the type at compile time.
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
`ATTR_VALID` otherwise. That overload also needs a readout time stamp, filled
with the current time (`Tango::TangoTimestamp(std::chrono::system_clock::now())`).
DATA_READY events carry no quality.
- `DataReady` pushes with `DeviceImpl::push_data_ready_event(attrName,
counter)`.
- Each attribute carries its own DATA_READY counter in `AdapterDeviceImpl`,
starting at 1. The counter advances only when the process variable's version
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
`doc/example-AttributeMapper.xml` shows one, on an attribute whose value
changes (for example a position source from `/Motors/position`), because
`numberOfMotors` never changes; the `numberOfMotors` attribute shows
`<eventing>off</eventing>` (or no `<eventing>` element).
- `doc/mainpage.dox` describes the keyword, the three values, the rejection of
an unknown value, of a write-only attribute and of `change` on a Void process
variable, and that image attributes are not implemented. It must not claim
that an image attribute is rejected when the mapper file is read, because that
case cannot occur.

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
`<eventing>` element, eventing on a write-only attribute, and `change` on a
Void attribute, each expected to throw `ChimeraTK::logic_error`. The
acceptance cases are the parsed mode for `dataReady`, `change`, `off` and an
absent `<eventing>` element (the last two both `Off`), `dataReady` on a Void
attribute, and the data-mapping cases: a scalar with
`<eventing>change</eventing>`, a spectrum with `<eventing>dataReady</eventing>`,
and a write-only attribute without `<eventing>`. Every case names the eventing
mode it carries in its doc comment, and compares the parsed mode with
`BOOST_TEST`, so a failure prints both values.
- The `runTestTangoServer` helper and the asset directories
`tests/rejectInvalidEventingValue` and `tests/rejectWriteOnlyEventing` are
removed, together with their CMake copies; the rejection cases are covered by
the mapper test bullet.
- The mapper also carries a readable `ChimeraTK::Void` attribute with
`dataReady`; write the matching to-device `ChimeraTK::Void` process variable
through the proxy and use the helper of the bullet above (one main loop
iteration plus the condition check, repeated until the event arrives), then
assert a DATA_READY event with an increasing counter arrives. `change` on a
Void attribute is asserted to be rejected, in the mapper test bullet.
- Assert a CHANGE event of an attribute whose process variable carries faulty
data validity arrives with quality `ATTR_INVALID`.

## Deferred issue

- DI-1 [DONE] Void must only emit DATA_READY events: the `DEV_VOID` branch and `pushVoidChangeEvent` (src/AdapterDeviceImpl.cpp:96) push a CHANGE event carrying a constant `true`, which does not make sense.
- DI-2 [DONE] The specification gives `DEV_VOID` a case of its own in the CHANGE switch and a separate helper for it (doc/change-requests/CR-002-tango-eventing.md:70); Void must only be data_ready.
- DI-3 [DONE] An event for the same version number must carry the same counter; the specification's rule that the counter is incremented on every push (doc/change-requests/CR-002-tango-eventing.md:102) is wrong.
- DI-4 [DONE] `pumpUntilEvent` (tests/src/testTangoServerEventing.cc:126) repeats the write and one main loop iteration until any matching event arrives, so a random later write can be paired with a random earlier event. Every write must have exactly one event.
- DI-5 [DONE] `testOffAttributeProducesNoEvent` (tests/src/testTangoServerEventing.cc:269) tests Tango core functionality and is unnecessary.
- DI-6 [DONE] `testVoidChangeEventCarriesConstantTrue` (tests/src/testTangoServerEventing.cc:278) asserts a CHANGE event for a Void attribute; Void must only ever carry a DATA_READY event.
- DI-7 [DONE] `testInvalidEventingValueRejectedAtMapperRead` (tests/src/testTangoServerEventing.cc:330) is a full end-to-end subprocess test; the rejection of an invalid `<eventing>` value must be a unit test of the attribute mapper.
- DI-8 [DONE] `testWriteOnlyEventingRejectedAtMapperRead` (tests/src/testTangoServerEventing.cc:339) is a full end-to-end subprocess test; the rejection of eventing on a write-only attribute must be a unit test of the attribute mapper.
- DI-9 [DONE] `doc/example-AttributeMapper.xml:19` uses `numberOfMotors` for `change`, a really bad example for a changing value; the example must use a value that changes.
- DI-10 [DONE] The plan leaves the behaviour of `<eventing>change</eventing>` on a readable `ChimeraTK::Void` process variable undefined, although DI-1/DI-2 forbid a CHANGE event for Void; the `addAttribute` rejection currently only covers unreadable and image (src/AttributMapper.cpp:232). User decision taken in this phase: reject `change` for a Void process variable when the mapper file is read.
- DI-11 [DONE] DI-7/DI-8 as worded are not implementable as a plain mapper unit test: `AttributeMapper` has no public entry point other than `readMapperFile` (include/AttributeMapper.h:65), which derives its file name from `Tango::Util::instance()->get_ds_exec_name()` (src/AttributMapper.cpp:394-397) and needs a `ControlSystemPVManager` (setCSPVManager). The plan must say how the mapper test reaches its offending mapper file and the process variable manager without starting a server.
- DI-12 [DONE] Removing `testOffAttributeProducesNoEvent` (DI-5) leaves the default (`off` and absent `<eventing>`) unasserted; the mapper test must also cover the parsed mode for `dataReady`, `change`, `off` and an absent element.
- DI-13 [DONE] Removing the two subprocess rejection tests (DI-7/DI-8) leaves `runTestTangoServer` (tests/src/testTangoServerEventing.cc:142) and the asset directories `tests/rejectInvalidEventingValue` and `tests/rejectWriteOnlyEventing` dead; the plan must state that they are removed (including their CMake copies).
- DI-14 [DONE] Removing the Void CHANGE test (DI-6) leaves Void eventing untested although DI-1 allows `dataReady` for a Void attribute; the plan needs a Void DATA_READY case in the test plan.
- DI-15 [DONE] DI-11 is not fully folded: the test plan still does not say how the mapper test obtains the process variable manager it passes to setCSPVManager. The application only exposes a DevicePVManager (ApplicationBase::getPVManager), not the ControlSystemPVManager that setCSPVManager takes; the reachable control-system manager is the running in-process server's, TangoAdapter::TangoAdapter::getInstance().getCsPvManager() (include/TangoAdapter.h:29). The plan also does not say where ...
- DI-16 [DONE] DI-14 is not fully folded: the Void DATA_READY bullet says to 'write it through the proxy' although the mapped Void attribute is read-based (VOID/FROM_DEVICE_SCALAR is not writeable), and it says to 'run one application main loop iteration' instead of the helper idiom the DI-4 correction introduced (one main loop iteration plus the condition check, repeated until the event arrives), which makes the assertion timing-dependent.
- DI-17 [DONE] clang-tidy-22 reports `modernize-raw-string-literal` at src/AttributMapper.cpp:244 (line added by this change request). That changed message carries escaped quotes and should use a raw string literal, as the neighbouring error message added in the same function at src/AttributMapper.cpp:313 already does. The warning appears despite `clang-format-19 --dry-run -Werror` passing, so it cannot be auto-applied.
- DI-18 [DONE] clang-tidy-22 reports `llvm-prefer-static-over-anonymous-namespace` for all five helpers in the anonymous namespace at tests/src/testTangoAttributeMapper.cc:50-95 (`attribute`, `attributeWithEventing`, `readMapperFor`, `parseAttributesOf`, `expectMapperRejects`). This namespace is newly added and is the only anonymous namespace in `src/`, `include/`, `tests/` and `examples/`; the project declares test helpers as `static` free functions (e.g. tests/src/testTangoServerEventin...
- DI-19 [DONE] `EventCollector::changeEventCount` (tests/src/testTangoServerEventing.cc:98) is never called anywhere in the new test file, so it is dead code introduced by this change request. `clang-tidy-22` cannot detect an unused public member function; the unused helper should be removed.
- DI-20 [DONE] tests/src/testTangoAttributeMapper.cc:128 - Doxygen comment contradicts the test: it says 'A scalar attribute without an ` <eventing>` element carries the mapped values, with eventing off.' but testMapperMapsScalarAttributeWithEventing passes `<eventing>change</eventing>` and asserts EventingMode::Change. It also deviates from the Test plan (doc/change-requests/CR-002-tango-eventing.md:172-175), which lists the scalar and spectrum data-mapping cases as *without* `<eventing>...
- DI-21 [DONE] tests/src/testTangoServerEventing.cc:145 - `BOOST_TEST(proxy.read_attribute(location).name == location)` is vacuous. Tango sets DeviceAttribute::name to the requested name, so the assertion holds for any name the read returns; it cannot fail for a successful read and does not verify the comment's claim ('a nested variable name has to reach the mapper and the event machinery'). The name used is the source path `INT/FROM_DEVICE_SCALAR`, which is not a Tango attribute of the d...
- DI-22 [DONE] Minor: the new eventing-mode comparisons use BOOST_CHECK (tests/src/testTangoAttributeMapper.cc:111,123,143,163,179) where cpp-testing prefers BOOST_TEST (BOOST_CHECK prints no values on failure).
- DI-23 [DONE] include/AttributeProperty.h:101 (line added by this change request): the AttributeProperty dumper prints the eventing mode with `static_cast<int>(prop.eventing)`, bypassing the `operator<<` that this same change request adds for EventingMode three lines above (include/AttributeProperty.h:71-89). `TANGO_LOG_DEBUG << attr` therefore shows a bare number while the new printer exists; a probe compiled with the project's own flags confirms that `os << prop.eventing` is found and ...
- DI-24 [DONE] tests/src/testTangoAttributeMapper.cc (14 sites, lines 53, 59, 67, 77, 89, 97, 117, 128, 148, 168, 184, 192, 200, 207) and tests/src/testTangoServerEventing.cc (5 sites, lines 31, 70, 78, 90, 97) document functions, members and test cases with `///` single-line comments. These are the only `///` comments in the whole project; the convention (cpp-documentation: "Use multi-line comments (`/** */`)", cpp-style chrome bar / `/**` rules) is `/** ... */` with `@brief`, as used by...
- DI-25 [DONE] tests/src/testTangoServerEventing.cc:125 calls the static member `ExtendedReferenceTestApplication::runMainLoopOnce()` through the instance `app`; clang-tidy-22 reports readability-static-accessed-through-instance on this line, which was added by this change request. The `app` parameter of `writeOnceAndRunMainLoopUntil` is used for nothing else, so it becomes unused once the call is qualified: call `ExtendedReferenceTestApplication::runMainLoopOnce()` and drop the parameter...
- DI-26 [DONE] tests/src/testTangoServerEventing.cc:14 includes <filesystem> (and line 18 <string>), neither of which is used anywhere in that file. clang-tidy-22 reports `misc-include-cleaner` (included header filesystem is not used directly) on this line, which was added by this change request; the project's no-warnings policy forbids linter findings, and clang-format-19 --dry-run passes, so the dead include cannot be removed automatically. <filesystem> is needed by tests/include_lib/Ta...
- DI-27 [DONE] include/ScalarAttribTempl.h:88-92 keeps a `std::is_same_v<TangoType, Tango::DevString>` branch in `ScalarAttribTempl<TangoType, AdapterType>::read` that is now equivalent to the generic branch below it (include/ScalarAttribTempl.h:106-112): both allocate one element with `std::make_unique<TangoType>`, fill it from `convertToTangoType<TangoType, AdapterType>(processScalar->accessData(0))` and call `att.set_value(value.release(), 1, 0, true)`. Because this change moved the st...
- DI-28 [DONE] include/TangoPropertyHelper.h:5 - clang-tidy-22 reports misc-include-cleaner: the header <ChimeraTK/SupportedUserTypes.h> is included but not used directly. This include line was added by this change request (git log -S and the diff against the merge-base confirm it as an added line). No symbol from SupportedUserTypes.h (no ChimeraTK:: or SupportedUserTypes usage) appears anywhere in the file; only int8_t and std::is_same_v are used, which are covered by <type_traits>. Fix:...
- DI-29 [DONE] tests/include_lib/TangoTestServer.h:193 - the Doxygen brief of `mapperFile` reads 'A mapper file with the given `<attribute>` children, written under the executable name of the running server.', but the helper only returns the mapper XML text: it writes no file and never looks at the executable name. That naming is done by `withMapperFileInCaseDirectory` (tests/include_lib/TangoTestServer.h:203-210), whose own comment already explains it, so the clause both misdescribes `ma...

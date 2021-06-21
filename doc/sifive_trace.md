# SiFive Trace Macro Library, Copyright 2021

## About
The sifive trace macro library allows the program running on the target to initiate and control trace at a finer level not supported by using Freedom Stuido`s trace capabilities. Freedom Studio can still be used to collect and view the trace data, or the user can perform those operations manually. When collecting CA (cycle accurate) traces, it is necessary to use the trace macro library due to the startup sequency when resuming from a breakpoint creating instruction and cycle accurate information that is difficult for the trace decoder to syncronize. Other trace functionallity can also be contolled at finer levels, that are not possible from Freedom Studio alone if the user has unique tracing requriemnts.


It is expected the user is familiar with the SiFive trace engine, its registers, and its functionallity. For more information, read the SiFive Trace Engine Design for the target processor.

## Memory Structures

### Intro
The three structures TraceRegMemMap, CaTraceRegMemMap, and TfTraceRegMemMap map the registers used for Tracing, Cycle Accurate (CA) Tracing, and Funnel Tracing to uint32_t variables that can be referenced.

### Usage
The trace encoder registers used to control tracing are memory mapped. The program under trace needs to declare three structures, `TraceRegMemMap`, `CaTraceRegMemMap`, and `TfTraceRegMemMap` which contain definiations for the trace control registers. The sifive_trace.h file contains an example of how to define them. Users will need to specify the correct addresses of the three register blocks. Addresses for the structures can be found in the dts file for the processor ip package. If the processor you are tracing does not have all trace components, the declaration for the missing components still needs to be present, but the address should be set to 0. If the trace funnel is not in use, `fmm` should be set to `0` in additon to `tfBaseAddress`. Likewise, if not using cycle accurate tracing, `cmm[]` should be set to `0` as well. For all memory structure declarations, the order of the addresses should match, i.e., `tmm[0]` should represent the same core as `cmm[0]`, etc.

For basic tracing include: 

`#include sifive_trace.h`

`struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)traceBaseAddr_Core0, ..., (struct TraceRegMemMap*)traceBaseAddr_CoreN};`

Where `traceBaseAddr_Core0`-`traceBaseAddr_CoreN` are the base addresses of each core's trace encoder. Only the addresses of the cores in which tracing is desired need to be included.

For the use of the trace funnel, include:

`define tfBaseAddress = 0`
`struct TfTraceRegMemMap volatile * const fmm = (struct TfTraceRegMemMap*)tfBaseAddress;`

Where `tfBaseAddress` is the address of the chip's funnel encoder, set to the address found in the dts file, or `0` if not present. 

For cycle accurate tracing include: 

`struct CaTraceRegMemMap volatile * const cmm[] = {(struct CaTraceRegMemMap*)caBaseAddr_Core0, ..., (struct CaTraceRegMemMap*)caBaseAddr_CoreN};`

Where `caBaseAddr_Core0`-`caBaseAddr_CoreN` are the base addresses of each core's cycle accurate (CA) trace encoder. Only the addresses of the cores in which CA tracing is desired need to be included.

The `CORES_COUNT` macro will be automatically set to be the number of cores being references by the `TraceRegMemMap` object.

## Available Macros

### Intro
  Most macros take a core aprgument when called to specify which of the cores setup in the memory structures to act upon, with the value `TRACE_CORES_ALL` working upon some macros causing them to act upon all coresbe . Register macros typicaly perform a single read or write to/from a specified register or field within a register, with the prefix `get` specifying a maco that will read a value from a register or field within, and the prefix `set` specifying a macro that will write a value register or field within. High level macros are typically comprised of multiple register macros. Register macros dont support the `TRACE_CORES_ALL` value for the core parameter, instead an existing core number needs to be passed if specified. Printing macros are a subset of high level macros, being comprised of multiple register macros. Each printing macro will write to `STDOUT` via `printf` provided by `<stdio.h>`for.

Below is a table discribing which registers/fields have macros, whether they are get and/or set, and what values can be passed to each get macro.

### All Macros

| Macro | Discription | Type |
|-|-|-|
| TEReset(core), TSReset(core), PCReset(core), CAReset(core), TFReset() | Resets the core's specified trace encoder unit: TE, TS, PC, CA, or TF. | High Level |
| enableTraceEncoder(core), disableTraceEncoder(core) | Enables or disables the trace encoder on the specified core. | High Level |
| enableTrace(core), disableTrace(core) | Enables or disables the tracing bit on the specified core. | High Level |
| Trace(core, opt) | Enable, disable, or reset a core's trace encoder. When enabled tracing can occure on the configured trace encoders: TE, CA, or funnel. |  High Level |
| setTraceMode(core, opt) | Sets the trace encoder intstruction bits to the specified option. | High Level |
| traceConfigDefaults(core), traceConfig(core, inst, instru, overflow, stall, sow, srcInhib, maxBtm, maxInst, teSink) | Configure all elements of a core's trace encoder. teSliceFormat is set to 6 MDo + 2 MSE0. | High Level |
| traceSetSinkAddr(core, addr, size) | Sets the sink address (teSinkHigh:teSinkBase) to the user specified address, and sets teSinkLimit to the highest address of the trace circular buffer based upon the size parameter. | High Level |
| setTraceEnable(core, opt) | Enable or disable a specified core's trace encoder. If the specified core is configured to use a funnel sink, the funnel will be enabled/disabled as well. | High Level |
| traceClear(core) | Clear the specified core's trace encoder buffer, if the core is using a SBA sink, will set RP and WP registers to the address of teSinkLimit. Otherwise sets the registers to zero. | High Level |
| tsConfigDefault(core), tsConfig(core, debug, precale, branch, instru, own) | Configure a specified core's timestamp register. | High Level |
| tfConfigDefault(), tfConfig(sow, sink) | Configure the trace funnel. | High Level |
| caTraceConfigDefaults(core), caTraceConfig(core, teInstruction, teInstrumentation, teStallEnable, teStopOnWrap, teInhibitSrc, teSynkMaxBtm, teSyncMaxInst, teSink, caStopOnWrap) | Configure cycle accurate (CA) trace on a specified core to passed parmeters. If `TRACE_CORES_ALL` is specified, the sink will be set to the funnel, regardless of passed parameter. | High Level |
| tfSetSinkAddr(addr, size) | Sets the sink address (tfSinkHigh:tfSinkBase) to the user specified address, and sets tfSinkLimit to the highest address of the trace circular buffer based upon the size parameter. | High Level |
| traceFlush() | Each core's will have its buffer flushed into its configured sink. If a funnel is present, it will be flushed after the cores, followed by the funnel`s ATB or PIB sink. Finally, the ATB and PIB sink for the cores will be flushed if in use. After each flush, the macro will block until the buffer is empty. | High Level |
| ItcEnableChannel(core, channel), ItcDisableChannel(core, channel) | Enables or disables a specified ITC channel on a specified core. | High Level |
| getTeControl(core), setTeControl(core, opt) | Get or set the 32 bit Te Control register | Register |
| setTeActive(core, opt), getTeActive(core) | Set or get bit 0 of the Te Control register | Register |
| setTeEnable(core, opt), getTeEnable(core) | Set or get bit 1 of the Te Control register | Register |
| setTeTracing(core, opt), getTeTracing(core | Set or get bit 2 of the Te Control register | Register |
| setTeEmpty(core, opt), getTeEmpty(core) | Set or get bit 3 of the Te Control register | Register |
| setTeInstruction(core, opt), getTeInstruction(core) | Set or get bits 6-4 of the Te Control register | Register |
| setTeInstruction(core, opt), getTeInstruction(core) | Set or get bits 8-7 of the Te Control register | Register | 
| setTeStallOrOverflow(core, opt), getTeStallOrOverflow(core) | Set or get bit 12 of the Te Control register | Register | 
| setTeStallEnable(core, opt), getTeStallEnable(core) | Set or get bit 13 of the Te Control register | Register | 
| setTeStopOnWrap(core, opt),  getTeStopOnWrap(core) | Set or get bit 14 of the Te Control register | Register |
| setTeInhibitSrc(core, opt), getTeInhibitSrc(core) | Set or get bit 15 of the Te Control register | Register |
| setTeSyncMaxBtm(core, opt), getTeSyncMaxBtm(core) | Set or get bits 19-16 of the Te Control register | Register |
| setTeSyncMaxInst(core, opt), getTeSyncMaxInst(core) | Set or get bits 23-20 of the Te Control register | Register |
| setTeSliceFormat(core, opt), getTeSliceFormat(core) | Set or get bits 26-24 of the Te Control register | Register |
| setTeSinkError(core, opt), getTeSinkError(core) | Set or get bit 27 of the Te Control register | Register |
| setTeSink(core, opt), getTeSink(core) | Set bits 31-28 of the Te Control register | Register |
| getTeFifo(core) | Get the 32 bit FIFO register | Register |
| getTeBtmCnt(core) | Get the 32 bit bottom count register | Register |
| getTeWordCnt(core) | Get the 32 bit word count register | Register |
| getTeWp(core) | Get the 32 bit WP register | Register |
| getTeImple(core) | Get the 32 bit Te Implementation register | Register |
| getTeImpleVersion(core) | Get bits 3-0 from the Te Implementation register | Register |
| getTeImpleHasSRAMSink(core) |Get bit 4 from the Te Implementation register | Register |
| getTeImpleHasATBSink(core) | Get bit 5 from the Te Implementation register | Register |
| getTeImpleHasPIBSink(core) | Get bit 6 from the Te Implementation register | Register |
| getTeImpleHasSBASink(core) | Get bit 7 from the Te Implementation register | Register |
| getTeImpleHasFunnelSink(core) | Get bit 8 from the Te Implementation register | Register |
| getTeImpleSinkyBytes(core) | Get bit 8 from the Te Implementation register | Register |
| getTeImpleCrossingType(core) | Get bits 19-18 from the Te Implementation register | Register |
| getTeImpleNSrcBits(core) | Get bits 26-24 from the Te Implementation register | Register |
| getTeImpleHartId(core) | Get bits 31-28 from the Te Implementation register | Register |
| getTeImpleSrcId(core) | Get bits 23-20 from the Te Implementation register | Register |
| setTeSinkBase(core, opt), getTeSinkBase(core) | Set or get the 32 bit Te SinkBase register | Register |
| setTeSinkBaseHigh(core, opt), getTeSinkBaseHigh(core) | Set or get the 32 bit Te SinkBase High register | Register |
| setTeSinkBaseLimit(core, opt), getTeSinkBaseLimit(core) | Set or get the 32 bit Te SinkBase Limit register | Register |
| setTeSinkWpReg(core, opt), getTeSinkWpReg(core) | Set or get the 32 bit Te SinkBase WP register | Register |
| setTeSinkWrap(core, opt), getTeSinkWrap(core) | Set or get the 32 bit Te SinkBase Wrap register | Register |
| setTeSinkRpReg(core, opt), getTeSinkRpReg(core) | Set or get the 32 bit Te SinkBase RP register | Register |
| setTeSinkData(core, opt), getTeSinkData(core) | Set or get the 32 bit Te SinkBase Data register | Register |
| setITCTraceEnable(core, opt), getITCTraceEnable(core) | Set or get the 32 bit ITC Trace Enable register | Register |
| setITCTrigEnable(core, opt), getITCTrigEnable(core) | Set or get the 32 bit ITC Trig Enable register | Register |
| getITCStimulus(core, regnum), setITCStimulus32Bit(core, regnum, value), setITCStimulus16Bit(core, regnum, value), setITCStimulus8Bit(core, regnum, value) | Get the 32 bit ITC Stimulus register or set 32, 16, or 8 bits of the register | Register |
| setITCMask(core, opt), getITCMask(core) | Set or get the 32 bit ITC  Mask register | Register |
| setITCTrigMask(core, mask), getITCTrigMask(core) | Set or get the 32 bit ITC Trig Mask register | Register |
| setTsControl(core, mask), getTsControl(core) | Set or get the 32 bit Ts Control register | Register |
| setTsActive(core, opt), getTsActive(core) | Set or get bit 0 of the Ts Control register | Register |
| setTsCount(core, opt), getTsCount(core) | Set or get bit 1 of the Ts Control register | Register |
| setTsReset(core, opt), getTsReset(core) | Set or get bit 2 of the Ts Control register | Register |
| setTsDebug(core, opt), getTsDebug(core) | Set or get or get bit 3 of the Ts Control register | Register |
| setTsType(core, opt), getTsType(core) | Set or get or get bits 6-4 of the Ts Control register | Register |
| setTsPrescale(core, opt), getTsPrescale(core) | Set or get or get bits 9-8 of the Ts Control register | Register |
| setTsEnable(core, opt), getTsEnable(core) | Set or get bit 15 of the Ts Control register | Register |
| setTsBranch(core, opt), getTsBranch(core) | Set or get bits 17-16 of the Ts Control register | Register |
| setTsInstrumentation(core, opt), getTsInstrumentation(core) | Set or get bit 18 of the Ts Control register | Register |
| setTsOwnership(core, opt), getTsOwnership(core) | Set or get bit 19 of the Ts Control register | Register |
| setTsWidth(core, opt), getTsWidth(core) | Set or get bits 31-24 of the Ts Control register | Register |
| setTsLower(core, opt), getTsLower(core) | Set or get the 32 bits of the Ts Lower register | Register |
| setTsUpper(core, opt), getTsUpper(core) | Set or get the 32 bits of the Ts Upper register | Register |
| getTsFull(core), setXtiReg(core, opt) | Get the 64 bits of the concatonated Ts upper and Ts lower registers | Register |
| getXtiReg(core) | Get the 32 bits of the XTI register | Register |
| setXtiAction(core, n, opt), getXtiAction(core, n) | Set or get 4 bits at the nth nibble from the XTI register | Register |
| setXtoReg(core, n, opt),  getXtoReg(core) | Set or get the 32 bits of the XTO register | Register |
| setXtoEvent(core, n, opt), getXtoEvent(core, n) | Set or get 4 bits at the nth nibble from the XTO register | Register |
| setWpReg(core, opt), getWpReg(core) | Set or get the 32 bits of the WP register | Register |
| setWp(core, n, opt), getWp(core, n) | Set or get 4 bits at the nth nibble from the WP register | Register |
| setPcControl(core, opt),  getPcControl(core) | Set or get the 32 bits of the PC Control register | Register |
| getPcCapture(core) | Get the 32 bits of the PC Capture register | Register |
| getPcCaptureValid(core) | Get bit 0 of the PC Capture register | Register
| getPcCaptureAddress(core) | Get bits 31-1 of the PC Capture register | Register |
| getPcCaptureHigh(core) | Get the 32 bits of the PC Capture High register | Register |
| getPcSampleReg(core) | Get the 32 bits of the PC Sample register | Register |
| getPcSampleValid(core) | Get bit 0 of the PC Sample register | Register |
| getPcSample(core) | Get bits 31-1 of the PC Sample register | Register |
| setTfControl(opt), getTfControl() | Set or get the 32 bits of the Tf Control register | Register |
| setTfActive(opt), getTfActive() | Set or get bit 0 of the Tf Control register | Register |
| setTfEnable(opt), getTfEnable() | Set or get bit 1 of the Tf Control register | Register |
| setTfEmpty(opt), getTfEmpty()  | Set or get bit 3 of the Tf Control register | Register |
| setTfStopOnWrap(opt), getTfStopOnWrap() | Set or get bit 14 of the Tf Control register | Register | 
| setTfSinkError(opt), getTfSinkError() | Set or get bit 27 of the Tf Control register | Register |
| setTfSink(opt), getTfSink() | Set or get bits 31-28 of the Tf Control register | Register |
| getTfImpl() | Get the 32 bits of the Tf Implementation register | Register |
| getTfVersion() | Get bits 3-0 of the Tf Implementation register | Register |
| getTfHasSRAMSink() | Get bit 4 of the Tf Implementation register | Register |
| getTfHasATBSink() | Get bit 5 of the Tf Implementation register | Register |
| getTfHasPIBSink() | Get bit 6 of the Tf Implementation register | Register |
| getTfHasSBASink() | Get bit 7 of the Tf Implementation register | Register |
| getTfHasFunnelSink() | Get bit 8 of the Tf Implementation register | Register |
| getTfSinkBytes() | Get bits 3-0 of the Tf Implementation register | Register |
| setTfSinkWp(opt), getTfSinkWp() | Set or get the 32 bits of the Tf Sink Wp register | Register |
| setTfSinkRp(opt), getTfSinkRp() | Set or get the 32 bits of the Tf Sink Rp register | Register |
| setTfSinkData(opt), getTfSinkData() | Set or get the 32 bits of the Tf Sink Data register | Register |
| setTeAtbControl(core, opt), getTeAtbControl(core) | Set or get the 32 bit Te ATB Control register | Register |
| setTeAtbActive(core, opt), getTeAtbActive(core) | Set or get bit 0 of the Te ATB Control register | Register |
| setTeAtbEnable(core, opt), getTeAtbEnable(core) | Set or get bit 1 of the Te ATB Control Register | Register |
| getTeAtbEmpty(core) | Get bit 3 of the Te ATB Control register | Register |
| setTePibControl(core, opt), getTePibControl(core) | Set or get the 32 bits of the Te PIB Control register | Register |
| setTePibActive(core, opt), getTePibActive(core) | Set or get bit 0 of the Te PIB Control register | Register |
| setTePibEnable(core, opt), getTePibEnable(core) | Set or get bit 1 of the Te PIB Control register | Register |
| getTePibEmpty(core) | Get bit 2 of the Te PIB Control Register | Register |
| setTfAtbControl(opt), getTfAtbControl() | Set or get the 32 bit Tf ATB Control register | Register |
| setTfAtbActive(opt), getTfAtbActive(opt) | Set or get bit 0 of the Tf ATB Control register | Register |
| setTfAtbEnable(opt), getTfAtbEnable() | Set or get bit 1 of the Tf ATB Control Register | Register |
| getTfAtbEmpty() | Get bit 3 of the Tf ATB Control register | Register |
| setTfPibControl(opt), getTfPibControl(opt) | Set or get the 32 bits of the Tf PIB Control register | Register |
| setTfPibActive(opt), getTfPibActive() | Set or get bit 0 of the Tf PIB Control register | Register |
| setTfPibEnable(opt), getTfPibEnable() | Set or get bit 1 of the Tf PIB Control register | Register |
| getTfPibEmpty() | Get bit 2 of the Tf PIB Control Register | Register |
| setTfSinkBase(opt), getTfSinkBase() | Set or get the 32 bits of the Tf Sinkbase register | Register |
| setTfSinkBaseHigh(opt), getTfSinkBaseHigh() | Set or get the 32 bits of the Tf Sink Base High register | Register |
| setTfSinkLimit(opt), getTfSinkLimit() | Set or get the 32 bits of the Tf Sink Limt register | Register |
| setCaControl(core, opt), getCaControl(core) | Set or get the 32 bits of the Ca Control register | Register |
| setCaActive(core, opt),  getCaActive(core) | Set or get bit 0 of the Ca Control register | Register |
| setCaEnable(core, opt), getCaEnable(core) | Set or get bit 1 of the Ca Control register | Register |
| getCaTracing(core) | Get bit 2 of the Ca Control register | Register |
| getCaEmpty(core) | Get bit 4 of the Ca Control register | Register |
| setCaStopOnWrap(core, opt) | Set or get bit 3 of the Ca Control register | Register |
| setCaSink(core, opt), getCaSink(core) | Set or get bits 31-28 of the Ca Control register | Register |
| getCaImpl(core) | Get the 32 bit Ca Implementation register | Register |
| getCaVersion(core) | Get bits 3-0 of the Ca Implementation register | Register |
| getCaSramSink(core) | get bit 4 of the Ca Implimentation register | Register |
| getCaAtbSink(core) | get bit 5 of the Ca Implimentation register | Register |
| getCaPibSink(core) | get bit 6 of the Ca Implimentation register | Register |
| getCaSbaSink(core) | get bit 7 of the Ca Implimentation register | Register |
| getCaFunnelSink(core) | get bit 8 of the Ca Implimentation register | Register |
| getCaSinkBytes(core) | Get bits 17-16 of the Ca Implementation register | Register |
| getCaSinkData(core) | Get the 32 bit Ca Sink Data register | Register |
| setCaSinkWpReg(core, opt), getCaSinkWpReg(core) | Set or get the 32 bit Ca Sink WP register | Register |
| setCaWrap(core, opt), getCaWrap(core) | Set or get the 32 bit Ca Wrap register | Register |
| setCaSinkRpReg(core, opt), getCaSinkRpReg(core) | Set or get the 32 bit Ca Sink RP register | Register |
| ItcWriteMarker(core, channel, marker) | Writes a 32-bit value to a specified ITC channel on a specified core. |  Printing |
| teRegDump(core) | Will print the address and value of all of the trace encoder (TE) registers: `CONTROL`, `IMPL`, `SINBASE`, `SINKBASE HIGH`, `SINKBASE LIMIT`, `SINK WP`, `SINK RP`, `SINK DATA`, `FIFO`, `BTM COUNT`, `WORD COUNT`, `TS CONTROL`, `TS LOWER`, `TS UPPER`, `XTI CONTROL`, `XTO CONTROL`, `WP CONTROL`, `ITC TRACE ENABLE`, and `ITC TRIG ENABLE`. | Printing |
| tfRegDump(core) | Will print the address and value of all of the trace funnel (TF) registers: `CONTROL`, `IMPL`, `SINBASE`, `SINKBASE HIGH`, `SINKBASE LIMIT`, `SINK WP`, `SINK RP`, and `SINK DATA`. | Printing |
| caRegDump(core) | Will print the address and value of all of the cycle accurate trace (CA) registers: `CONTROL`, `IMPL`,  `SINK WP`, `SINK RP`, and `SINK DATA`. | Printing | 
| teControlDump(core) | Will print both the value and the meaning of the value for each field in the teControl register. | Printing |
| tsControlDump(core) | Will print both the value and the meaning of the value for each field in the tsControl register. | Printing |
| implControlDump(core) | Will print both the value and the meaning of the value for each field in the implControl register. | Printing | 
| xtiControlDump(core) | Will print both the value and the meaning of the value for each field in the xtiControl register. | Printing |
| xtoControlDump(core) | Will print both the value and the meaning of the value for each field in the xtoControl register. | Printing |
| tfControlDump(core) | Will print both the value and the meaning of the value for each field in the tfControl register. | Printing |



### High Level Macros

The below macros are used to set up the Trace Enable, Cycle Accurate Trace, and Trace Funnel registers to enable the collection of various traces without the use of Freedom Studio to configure and control the trace. However, Freedom Studio can still be used to collect the resulting trace files automatically, otherwise a remote connection to OpenOCD is still required to collect the trace files from the proccessor using the `wtb` and `wcab` commands from `trace.tcl`.
### `TEReset(core)`
Resets a core's trace encoder.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.


### `TSReset(core)`
Resets a core's time stamp encoder.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `PCReset(core)`
Resets a core's PC sampling unit.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `CAReset(core)`
Resets a core's CA trace encoder.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `TFReset()`
Resets the trace funnel.

Parameters: none

### `enableTraceEncoder(core)`
Enables the trace encoder on the specified core.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.


Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `disableTraceEncoder(core)`
Disables the trace encoder on the specified core.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.


### `enableTrace(core)`
Enables the tracing bit on the specified core.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `disableTrace(core)`
Disables the tracing bit on the specified core.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.


### `Trace(core, opt)`
Enable, disable, or reset a core's trace encoder. When enabled tracing can occure on the configured trace encoders: TE, CA, or funnel.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `opt`
  * `TRACE_ON`: enable both the trace encoder and the tracing bit on the specified cores.
  * `TRACE_OFF`: disable the trace encoder on the specified core.
  * `TRACE_RESET`:  reset tracing on the specified core.


### `setTraceMode(core, opt)`
Sets the trace encoder intstruction bits to the specified option.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `opt`
  * `TEINSTRUCTION_NONE`: will case the trace encoder to not generate instruction trace messages.
  * `TEINSTRUCTION_BTMSYNC`: will generate both BTM and Sync trace messages.
  * `TEINSTRUCTION_HTM`: will generate HTM and Sync trace messages without HTM optimization.
  * `TEINSTRUCTION_HTMSYNC`: will generae both HTM and Sync trace messages with HTM optimization.

### `traceConfigDefaults(core)`
Clear the specified core's trace encoder buffer, if the core is using a SBA sink, will set RP and WP registers to the address of teSinkLimit. Otherwise sets the registers to zero.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `traceConfig(core, inst, instru, overflow, stall, sow, srcInhib, maxBtm, maxInst, teSink)`
Configure all elements of a core's trace encoder. teSliceFormat is set to 6 MDo + 2 MSE0.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `inst`
  * `TE_INSTRUCTION_NONE`: Dont generate BTM or Sync.
  * `TE_INSTRUCTION_BTMSYNC`: Generate BTM and Sync.
  * `TE_INSTRUCTION_HTM`: Generate HTM and Sync without HTM optimization.
  * `TE_INSTRUCTION_HTMSYNC`: Generate HTM and Sync with HTM optimization.
* `instru`
  * `TE_INSTRUMENTATION_NONE`: ITC trace diasabled.
  * `TE_INSTRUMENTATION_ITC`: Generate ITC messages for all itcStimmulus registers.
  * `TE_INSTRUMENTATION_OWNERSHIP`: Generate Ownership message for itcStimulus[15, 31], disable other ITC trace.
  * `TE_INSTRUMENTATION_OWNERSHIPALL`: Generate Ownership message for itcStimulus[15, 31] and ITC messages for all other itcStimulus registers.
* `overflow`
  * `CLEAR`: Clear the StallorOverflow bit, bit enabled when an overflow message is generated or core stalled.
* `stall`
  * `TE_STALL_ENABLE_OFF`: If TE cannot accept a message, an overflow is generated.
  * `TE_STALL_ENABLE_ON`: If TE cannot accept a message, the core is stalled until it can.
* `sow`
  * `TE_STOPONWRAP_OFF`: Don`t set teEnable to 0 when curcular buffer fills.
  * `TE_STOPONWRAP_ON`: Set teEnable to 0 when circular buffer fills, present only in systems with SRAM or system memory sinks.
* `srcInhib`
  * `TE_INHIBITSRC_OFF`: SRC field of teImpl.nSrcBits is added to every Nexus message to indicate which TE generated each message.  Present only in systems with a Funnel sink.
  * `TE_INHIBITSRC_ON`: Disable SRC field in Nexus messages.
* `maxBtm`
  * `TE_SYNCMAXBTM_32`: Generate sync after 32 BTMs.
  * `TE_SYNCMAXBTM_64`: Generate sync after 64 BTMs.
  * `TE_SYNCMAXBTM_128`: Generate sync after 128 BTMs.
  * `TE_SYNCMAXBTM_256`: Generate sync after 256 BTMs.
  * `TE_SYNCMAXBTM_512`: Generate sync after 512 BTMs.
  * `TE_SYNCMAXBTM_1024`: Generate sync after 1024 BTMs.
  * `TE_SYNCMAXBTM_2048`: Generate sync after 2048 BTMs.
  * `TE_SYNCMAXBTM_4096`: Generate sync after 4096 BTMs.
  * `TE_SYNCMAXBTM_8192`: Generate sync after 8192 BTMs.
  * `TE_SYNCMAXBTM_16348`: Generate sync after 16348 BTMs.
  * `TE_SYNCMAXBTM_32768`: Generate sync after 32768 BTMs.
  * `TE_SYNCMAXBTM_65536`: Generate sync after 65536 BTMs.
* `maxInst`
  * `TE_SYNCMAXINST_32`: Generate Sync when running I-CNT reaches 32 bytes.
  * `TE_SYNCMAXINST_64`: Generate Sync when running I-CNT reaches 64 bytes.
  * `TE_SYNCMAXINST_128`: Generate Sync when running I-CNT reaches 128 bytes.
  * `TE_SYNCMAXINST_256`: Generate Sync when running I-CNT reaches 256 bytes.
  * `TE_SYNCMAXINST_512`: Generate Sync when running I-CNT reaches 512 bytes.
  * `TE_SYNCMAXINST_1024`: Generate Sync when running I-CNT reaches 1024 bytes.
* `teSink`
  * `TE_SINK_DEFAULT`: Send trace to the one sink connected.
  * `TE_SINK_SRAM`: Use SRAM sink.
  * `TE_SINK_ATB`: Use ATB sink.
  * `TE_SINK_PIB`: Use PIB sink.
  * `TE_SINK_SBA`: Use SBA sink.
  * `TE_SINK_FUNNEL`: Use Funnel sink.

### `traceSetSinkAddr(core, addr, size)`
Sets the sink address (teSinkHigh:teSinkBase) to the user specified address, and sets teSinkLimit to the highest address of the trace circular buffer based upon the size parameter.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `addr`
  * Base address (64-bit) of the circular buffer used for trace messages.
* `size`
  * Number of words in the circular buffer, used to set the teSinkLimit register.


### `setTraceEnable(core, opt)`
Enable or disable a specified core's trace encoder. If the specified core is configured to use a funnel sink, the funnel will be enabled/disabled as well.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `opt`
  * `ENABLE`: Enable tracing on the specified core.
  * `DISABLE`: Disable tracing on the specified core.

### `traceClear(core)`
Clear the specified core's trace encoder buffer, if the core is using a SBA sink, will set RP and WP registers to the address of teSinkLimit. Otherwise sets the registers to zero.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.



### `tsConfig(core, debug, precale, branch, instru, own)`
Configure a specified core's timestamp register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `debug`
  * `Enable`: Counter runs in debug mode.
  * `Disable`: Counter doesn`t run in debug mode.
* `prescale`
  * `TS_PRESCL_1`: Prescale timestamp clock by 1.
  * `TS_PRESCL_4`: Prescale timestamp clock by 4.
  * `TS_PRESCL_16`: Prescale timestamp clock by 16.
  * `TS_PRESCL_64`: Prescale timestamp clock by 64.
* `branch`
  * `BRNCH_off`: No timestamp on branch messages.
  * `BRNCH_indt`: Timestamp on all indirect branch and exception messages.
  * `BRNCH_excp`: Timestamp on all indirect branch and exception messages.
  * `BRNCH_resv`: Reserved.
  * `BRNCH_all`: Timestamp on all branches except PTCm and error messages.
* `isntru`
  * `ENABLE`: Add timestamp field to instrumentation messages if tsEnable=1. Fixed at zero if ITC unit is not present.
  * `DISABLE`: Dont add timestamp field to instrumentation messages.
* `own`
  * `ENABLE`: Add timestamp field to ownership messages if tsEnable=1. Fixed at zero if ITC unit is not present.
  * `DISABLE`: Dont add timestamp field to ownership messages.

### `tsConfigDefault(core)`
Configure a specified core's timestamp register using default settings.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.

### `tfConfigDefault()`
Configure the trace funnel to default parameters: StopOnWrap disabled, TfSink set to default sink.

### `tfConfig(sow, sink)`
Configure the trace funnel.

Parameters:
* `sow`
  * `Enable`: Set tfEnable to 0 when the trace buffer fills.
  * `Disable`: Overwrite data when the trace buffer fills.
* `sink`
  * `TE_SINK_DEFAULT`: Send trace to the one sink connected.
  * `TE_SINK_SRAM`: Use SRAM sink.
  * `TE_SINK_ATB`: Use ATB 
  * `TE_SINK_PIB`: Use PIB sink.
  * `TE_SINK_SBA`: Use SBA sink.

### `caTraceConfigDefaults(core)`
Configure cycle accurate (CA) trace on a specified core to default values: HTM or BTM instruction messages (selected based on which is supported by each core), SRAM sink (if TRACE_CORES_ALL is specified, sink will be set to funnel), ITC trace disabled, XTI set to record program trace sink messages, XTO set to cause external trigger 0 to fire when stopping the trace, StopOnWrap enabled, tracing enabled.

* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.


### `caTraceConfig(core, teInstruction, teInstrumentation, teStallEnable, teStopOnWrap, teInhibitSrc, teSynkMaxBtm, teSyncMaxInst, teSink, caStopOnWrap)`
Configure cycle accurate (CA) trace on a specified core to passed parmeters. If `TRACE_CORES_ALL` is specified, the sink will be set to the funnel, regardless of passed parameter.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `teInstruction`
  * `TE_INSTRUCTION_NONE`: Dont generate BTM or Sync.
  * `TE_INSTRUCTION_BTMSYNC`: Generate BTM and Sync.
  * `TE_INSTRUCTION_HTM`: Generate HTM and Sync without HTM optimization.
  * `TE_INSTRUCTION_HTMSYNC`: Generate HTM and Sync with HTM optimization.
* `teInstrumentation`
  * `TE_INSTRUMENTATION_NONE`: ITC trace diasabled.
  * `TE_INSTRUMENTATION_ITC`: Generate ITC messages for all itcStimmulus registers.
  * `TE_INSTRUMENTATION_OWNERSHIP`: Generate Ownership message for itcStimulus[15, 31], disable other ITC trace.
  * `TE_INSTRUMENTATION_OWNERSHIPALL`: Generate Ownership message for itcStimulus[15, 31] and ITC messages for all other itcStimulus registers.
* `teStallEnable`
  * `TE_STALL_ENABLE_OFF`: If TE cannot accept a message, an overflow is generated.
  * `TE_STALL_ENABLE_ON`: If TE cannot accept a message, the core is stalled until it can.
* `teStopOnWrap`
  * `TE_STOPONWRAP_OFF`: Don`t set teEnable to 0 when curcular buffer fills.
  * `TE_STOPONWRAP_ON`: Set teEnable to 0 when circular buffer fills, present only in systems with SRAM or system memory sinks.
* `teInhibitSrc`
  * `TE_INHIBITSRC_OFF`: SRC field of teImpl.nSrcBits is added to every Nexus message to indicate which TE generated each message.  Present only in systems with a Funnel sink.
  * `TE_INHIBITSRC_ON`: Disable SRC field in Nexus messages.
* `teSyncMaxBTM`
  * `TE_SYNCMAXBTM_32`: Generate sync after 32 BTMs.
  * `TE_SYNCMAXBTM_64`: Generate sync after 64 BTMs.
  * `TE_SYNCMAXBTM_128`: Generate sync after 128 BTMs.
  * `TE_SYNCMAXBTM_256`: Generate sync after 256 BTMs.
  * `TE_SYNCMAXBTM_512`: Generate sync after 512 BTMs.
  * `TE_SYNCMAXBTM_1024`: Generate sync after 1024 BTMs.
  * `TE_SYNCMAXBTM_2048`: Generate sync after 2048 BTMs.
  * `TE_SYNCMAXBTM_4096`: Generate sync after 4096 BTMs.
  * `TE_SYNCMAXBTM_8192`: Generate sync after 8192 BTMs.
  * `TE_SYNCMAXBTM_16348`: Generate sync after 16348 BTMs.
  * `TE_SYNCMAXBTM_32768`: Generate sync after 32768 BTMs.
  * `TE_SYNCMAXBTM_65536`: Generate sync after 65536 BTMs.
* `teSyncMaxInst`
  * `TE_SYNCMAXINST_32`: Generate Sync when running I-CNT reaches 32 bytes.
  * `TE_SYNCMAXINST_64`: Generate Sync when running I-CNT reaches 64 bytes.
  * `TE_SYNCMAXINST_128`: Generate Sync when running I-CNT reaches 128 bytes.
  * `TE_SYNCMAXINST_256`: Generate Sync when running I-CNT reaches 256 bytes.
  * `TE_SYNCMAXINST_512`: Generate Sync when running I-CNT reaches 512 bytes.
  * `TE_SYNCMAXINST_1024`: Generate Sync when running I-CNT reaches 1024 bytes.
* `teSink`
  * `TE_SINK_DEFAULT`: Send trace to the one sink connected.
  * `TE_SINK_SRAM`: Use SRAM sink.
  * `TE_SINK_ATB`: Use ATB sink.
  * `TE_SINK_PIB`: Use PIB sink.
  * `TE_SINK_SBA`: Use SBA sink.
  * `TE_SINK_FUNNEL`: Use Funnel sink.
* `caStopOnWrap`
  * `TE_STOPONWRAP_OFF`: Don`t set caEnable to 0 when curcular buffer fills.
  * `TE_STOPONWRAP_ON`: Set caEnable to 0 when circular buffer fills, present only in systems with SRAM

### `tfSetSinkAddr(addr, size)`
Sets the sink address (tfSinkHigh:tfSinkBase) to the user specified address, and sets tfSinkLimit to the highest address of the trace circular buffer based upon the size parameter.

Parameters:
* `addr`
  * Base address (64-bit) of the circular buffer used for trace messages.
* `size`
  * Number of words in the circular buffer, used to set the tfSinkLimit register.


### `traceFlush()`
Each core's will have its buffer flushed into its configured sink. If a funnel is present, it will be flushed after the cores, followed by the funnel`s ATB or PIB sink. Finally, the ATB and PIB sink for the cores will be flushed if in use. After each flush, the macro will block until the buffer is empty.




### `ItcEnableChannel(core, channel)`
Enables a specified ITC channel on a specified core.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `channel`
  * Value between 1 and 32 coresponding to which channel to enable.

### `ItcDisableChannel(core, channel)`
Disables a specified ITC channel on a specified core.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `channel`
  * Value between 1 and 32 coresponding to which channel to disable.

### `ItcWriteMarker(core, channel, marker)`
Writes a 32-bit value to a specified ITC channel on a specified core.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
  * Passing `TRACE_CORES_ALL` will act upon all cores.
* `channel`
  * Value between 1 and 32 coresponding to which channel to write to.
* `marker`
  * 32-bit value to write to the specified core's ITC channel.

### Register Macros
These macros are used to to directly read/write to individual trace registers or fields within trace registers.


### ` getTeControl(core), setTeControl(core, value) `
Reads or writes the 32-bit Te Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit Value 


### ` getTeActive(core), setTeActive(core, opt) `
Reads or writes the TE Active field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeEnable(core), setTeEnable(core, opt) `
Reads or writes the TE Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeEmpty(core), setTeEmpty(core, opt) `
Reads or writes the TE Empty field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeInstruction(core), getTeInstruction(core, opt) `
Reads or writes the TE Intruction field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_INSTRUCTION_NONE`
  *  `TE_INSTRUCTION_BTMSYNC`
  *  `TE_INSTRUCTION_HTM`
  *  `TE_INSTRUCTION_HTMSYNC` 


### ` getTeInstrumentation(core), setTeInstrumentation(core, opt) `
Reads or writes the TE Intrumentation field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_INSTRUMENTATION_NONE`
  *  `TE_INSTRUMENTATION_ITC`
  *  `TE_INSTRUMENTATION_OWNERSHIP`
  *  `TE_INSTRUMENTATION_OWNERSHIPALL` 


### ` getTeStallOrOverflow(core), setTeStallOrOverflow(core, opt)  `
Reads or writes the TE Stall or Overflow field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeStallEnable(core), setTeStallEnable(core, opt)  `
Reads or writes the TE Stall Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeStopOnWrap(core), setTeStopOnWrap(core, opt) `
Reads or writes the TE Stop on Wrap field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeInhibitSrc(core),  setTeInhibitSrc(core, opt)  `
Reads or writes the TE Inhibit Source field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeSyncMaxBtm(core), setTeSyncMaxBtm(core, opt) `
Reads or writes the TE Sync Max BTM field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_SYNCMAXBTM_X` {X=32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16348, 32768, 65536} 


### ` getTeSyncMaxInst(core), setTeSyncMaxInst(core, opt)  `
Reads or writes the TE Syn Max Instruction field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_SYNCMAXINST_X` {X=32, 64, 128, 256, 512, 1024} 


### ` getTeSliceFormat(core), setTeSliceFormat(core, opt) `
Reads or writes the TE Slice Format field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  * `SLICE_FORMAT_6MDO_2MSEO`


### ` getTeSinkError(core), setTeSinkError(core, set) `
Reads or writes the TE Sink Error field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_CLEAR` 


### ` getTeSink(core), getTeSink(core, opt) `
Reads or writes the TE Sink field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_SINK_DEFAULT`
  *  `TE_SINK_SRAM`
  *  `TE_SINK_ATB`
  *  `TE_SINK_PIB`
  *  `TE_SINK_SBA`
  *  `TE_SINK_FUNNEL` 


### ` getTeImple() `
Reads the Te Implementation register.

Parameters: none


### ` getTeImpleVersion() `
Reads the Te Implementation Version field.

Parameters: none


### ` getTeImpleHasSRAMSink(core) `
Reads the Te Implementation Has SRAM field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleHasATBSink(core) `
Reads the Te Implementation Has ATB field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleHasPIBSink(core) `
Reads the Te Implementation Has PIB field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleHasSBASink(core)  `
Reads the Te Implementation Has SBA field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleHasFunnelSink(core)  `
Reads the Te Implementation Has Funnel field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleSinkyBytes(core) `
Reads the Te Implementation Sink Bytes field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleCrossingType(core) `
Reads the Te Implementation Crossing Type field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleNSrcBits(core) `
Reads the Te Implementation N Source Bits field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleHartId(core)  `
Reads the Te Implementation Hart ID field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeImpleSrcId(core)  `
Reads the Te Implementation Source ID field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTeSinkBase(core), setTeSinkBase(core, value) `
Reads or writes the Te Sink Base register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit Address 


### ` getTeSinkBaseHigh(core), setTeSinkBaseHigh(core, value) `
Reads or writes the Te Sink Base High register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit Address 


### ` getTeSinkBaseLimit(core), setTeSinkBaseLimit(core, value) `
Reads or writes the Te Sink Base Limit register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit Address 


### ` getTeSinkWpReg(core), setTeSinkWpReg(core, value) `
Reads or writes the Te Sink WP register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value` 
  *  32-bit number 


### ` getTeSinkWrap(core), setTeSinkWrap(core, opt) `
Reads or writes the Te Sink Wrap field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeSinkRpReg(core), setTeSinkRpReg(core, opt) `
Reads or writes the Te Sink RP register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  32-bit number 


### ` getTeSinkData(core), setTeSinkData(core, value) `
Reads or writes the Te Sink Data register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  32-bit number 


### ` getITCTraceEnable(core), setITCTraceEnable(core, value) `
Reads or writes the ITC Trace Enable register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  32-bit number 


### ` getITCTrigEnable(core), setITCTrigEnable(core, value) `
Reads or writes the ITC Trig Enable register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit number 


### ` getITCStimulus(core, regnum) `
Reads one ITC Stimulus register. There are 32 ITC Stimulus registers, one for each channel. The `regnum` paremeter specifies which register to read from.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `regnum`
  *  Register number (0-31) 


### ` setITCStimulus32Bit(core, regnum, value) `
Writes a 32-bit value to the ITC Stimulus register specified by `regnum`.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `regnum`
  *  Register number (0-31)
* `value`
  * 32-bit value 


### ` setITCStimulus16Bit(core, regnum, value) `
Writes a 16-bit value to the upper 16 bits of a ITC Stimulus register specified by `regnum`. Doesn't write the the lower 16 bits.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `regnum`
  *  Register number (0-31)
* `value`
  * 16-bit value


### ` setITCStimulus8Bit(core, regnum, value) `
Writes a 8-bit value to the upper 8 bits of a ITC Stimulus register specified by `regnum`. Doesn't write to the lower 24 bits.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `regnum`
  *  Register number (0-31)
* `value`
  * 8-bit value


### ` getITCMask(core), setITCMask(core, mask) `
Reads or writes to the ITC Mask register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `mask` 
  *  32-bit mask 


### ` getITCTrigMask(core), setITCTrigMask(core, mask) `
Reads or writes to the ITC Trig Mask register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `mask`
  *  32-bit mask 


### ` setITC(core, bmask, tmask) `
Writes both the ITC bit-mask and trig-mask registers.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `bmask`
  *  32-bit mask
* `tmask`
  * 32-bit mask 


### ` getTsControl(core), setTsControl(core, value) `
Reads or writes to the 32-bit TS Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getTsActive(core), setTsActive(core, opt) `
Reads or writes to the TS Active field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsCount(core), setTsCount(core, opt) `
Reads or writes to the TS Count field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsReset(core), setTsReset(core, opt) `
Reads or writes to the TS Reset field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsDebug(core), setTsDebug(core, opt)  `
Reads or writes to the TS Debug field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsPrescale(core), setTsPrescale(core, opt) `
Reads or writes to the TS Prescale field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TS_PRESCL_X` {X=1, 4, 16, 64} 


### ` getTsEnable(core), setTsEnable(core, opt) `
Reads or writes to the TS Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsBranch(core), setTsBranch(core, opt) `
Reads or writes to the TS Branch field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `BRNCH_OFF`
  *  `BRNCH_INDT`
  *  `BRNCH_EXPT`
  *  `BRNCH_ALL` 


### ` getTsInstrumentation(core), setTsInstrumentation(core, opt) `
Reads or writes to the TS Intrumentation field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsOwnership(core), setTsOwnership(core, opt) `
Reads or writes to the TS Ownership field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTsWidth(core), setTsWidth(core, value) `
Reads or writes to the TS Width field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  8-bit value 


### ` getTsLower(core), setTsLower(core, value) `
Reads or writes to the 32-bit TS Lower register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getTsUpper(core), setTsUpper(core, value) `
Reads or writes to the 32-bit TS Upper register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getTsFull(core) `
Reads both the TS Lower and TS Upper registers, and returns the 64-bit concatenation. 

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getXtiReg(core), setXtiReg(core, value) `
Reads or writes to the 32-bit XTI register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getXtiAction(core, n), setXtiAction(core, n, value) `
Reads or writes to 4 bits of XTI register. The parameter `n` specifies which nibble to read or write to, with `n=0` writing to the lowest nibble (bits 3-0), `n=1` writing to the lowest nibble (bits 7-4), etc.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `n`
  *  value between 0-7
* `value`
  * 4-bit value 


### ` getXtoReg(core), setXtoReg(core, value) `
Reads or writes to the 32-bit XTO register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getXtoEvent(core, n), setXtoEvent(core, n, value) `
Reads or writes to 4 bits of XTO register. The parameter `n` specifies which nibble to read or write to, with `n=0` writing to the lowest nibble (bits 3-0), `n=1` writing to the lowest nibble (bits 7-4), etc.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `n`
  *  value between 0-7
* `opt`
  * 4-bit value 


### ` getWpReg(core), getWpReg(core, value) `
Reads or writes to the 32-bit WP register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getWp(core, n), setWp(core, n, value) `
Reads or writes to 4 bits of WP register. The parameter `n` specifies which nibble to read or write to, with `n=0` writing to the lowest nibble (bits 3-0), `n=1` writing to the lowest nibble (bits 7-4), etc.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `n`
  *  value between 0-7
* `value`
  * 4-bit value 


### ` getPcControl(core), setPcControl(core, value) `
Reads or writes to the 32-bit PC Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getPcCapture(core) `
Reads the 32-bit PC Capture register.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcCaptureValid(core) `
Reads the 32-bit PC Capture Valid field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcCaptureAddress(core) `
Reads the 32-bit PC Capture Address register.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcCaptureHigh(core) `
Reads the 32-bit PC Capture High register.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcSampleReg(core) `
Reads the 32-bit PC Sample register.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcSampleValid(core) `
Reads the PC Sample Valid field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getPcSample(core) `
Reads the PC Sample field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTfControl(), setTfControl(value) `
Reads or writes to the 32-bit Tf Control register.

Parameters:
* `value`
  *  32-bit value 


### ` getTfActive(), setTfActive(opt) `
Reads or writes to the Tf Active field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfEnable(), setTfEnable(opt) `
Reads or writes to the Tf Enable field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfEmpty(), setTfEmpty(opt) `
Reads or writes to the Tf Empty field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfStopOnWrap(), setTfStopOnWrap(opt) `
Reads or writes to the Tf Stop on Wrap field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfSinkError(), setTfSinkError(opt) `
Reads or writes to the Tf Sink Error field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfSink(), setTfSink(opt) `
Reads or writes to the Tf Sink field.

Parameters:
* `opt`
  *  `TE_SINK_DEFAULT`
  *  `TE_SINK_SRAM`
  *  `TE_SINK_ATB`
  *  `TE_SINK_PIB`
  *  `TE_SINK_SBA` 


### ` getTfImpl() `
Reads to the 32-bit Tf Implementation register.

Parameters: none


### ` getTfVersion() `
Reads to the Tf Implementation Version field.

Parameters: none


### ` getTfHasSRAMSink() `
Reads to the Tf Implementation Has SRAM field.

Parameters: none


### ` getTfHasATBSink() `
Reads to the Tf Implementation Has ATB field.

Parameters: none 


### ` getTfHasPIBSink() `
Reads the Tf Implementation Has PIB field.

Parameters: none


### ` getTfHasFunnelSink() `
Reads the Tf Implementation Has Funnel field. Note, the trace funnel cannot have a trace funnel, and thus this macro will always return `0`.

Parameters: none


### ` getTfSinkBytes() `
Reads the 32-bit Tf Sink Bytes register.

Parameters: none


### ` getTfSinkWp(), setTfSinkWp(opt) `
Reads or writes to the 32-bit Tf Sink WP register.

Parameters:
* `value`
  *  32-bit value 


### ` getTfSinkRp(), setTfSinkRp(opt) `
Reads or writes to the 32-bit Tf Sink RP register.

Parameters:
* `value`
  *  32-bit value 


### ` getTfSinkData(), setTfSinkData(value) `
Reads or writes to the 32-bit Tf Sink Data register.

Parameters:
* `value`
  *  32-bit value 


### ` getTeAtbControl(core), setTeAtbControl(core, value) `
Reads or writes to the 32-bit Te ATB Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getTeAtbActive(core), setTeAtbActive(core, opt) `
Reads or writes to the Te ATB Active field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeAtbEnable(core), setTeAtbEnable(core, opt) `
Reads or writes to the Te ATB Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTeAtbEmpty(core) `
Reads or writes to the Te ATB Empty field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTePibControl(core), setTePibControl(core, value) `
Reads or writes to the 32-bit Te PIB Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit number 


### ` getTePibActive(core), setTePibActive(core, opt) `
Reads or writes to the Te PIB Active field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTePibEnable(core), setTePibEnable(core, opt) `
Reads or writes to the Te PIB Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTePibEmpty(core) `
Reads to the Te PIB Empty field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getTfAtbControl(), setTfAtbControl(value) `
Reads or writes to the 32-bit Tf ATB Control register.

Parameters:
* `value`
  *  32-bit value 


### ` getTfAtbActive(), setTfAtbActive(opt) `
Reads or writes to the Tf ATB Active field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfAtbEnable(), setTfAtbEnable(opt) `
Reads or writes to the Tf ATB Enable field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfAtbEmpty() `
Reads to the Tf ATB Empty field.

Parameters: none


### ` getTfPibControl(), setTfPibControl(value) `
Reads or writes to the 32-bit Tf PIB Control register.

Parameters:
* `value`
  *  32-bit number 


### ` getTfPibActive(), setTfPibActive(opt) `
Reads or writes to the Tf PIB Active field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getTfPibEnable(), setTfPibEnable(opt) `
Reads or writes to the Tf PIB Enable field.

Parameters:
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 

### ` getTfPibEmpty() `
Reads to the Tf PIB Empty field.

Parameters: none


### ` getTfSinkBase(), setTfSinkBase(value) `
Reads or writes to the 32-bit Tf Sink Base register.

Parameters:
* `value`
  *  32-bit Address 


### ` getTfSinkBaseHigh(), setTfSinkBaseHigh(value) `
Reads or writes to the 32-bit Tf Sink Base High register.

Parameters:
* `value`
  *  32-bit Address 


### ` getTfSinkBaseLimit(), setTfSinkBaseLimit(value) `
Reads or writes to the 32-bit Tf Sink Base Limit register.

Parameters:
* `value`
  *  32-bit Address 


### ` getCaControl(core), setCaControl(core, value)  `
Reads or writes to the 32-bit Ca Control register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  * 32-bit Value



### ` getCaActive(core), setCaActive(core, opt) `
Reads or writes to the Ca Active field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getCaEnable(core), setCaEnable(core, opt) `
Reads or writes to the Ca Enable field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getCaTracing(core) `
Reads the Ca Tracing field.

Parameters: 
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.


### ` getCaEmpty(core) `
Reads the Ca Empty field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores. 


### ` getCaStopOnWrap(core), setCaStopOnWrap(core, opt) `
Reads or writes to the Ca Stop on Wrap field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getCaSink(core), setCaSink(core, value) `
Reads or writes to the Ca Sink field.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit Value 


### ` getCaImpl() `
Reads the 32-bit Ca Implementation register.

Parameters: none


### ` getCaVersion() `
Reads the Ca Implementation Version field.

Parameters: none


### ` getCaSramSink() `
Reads the Ca Implementation SRAM Sink field.

Parameters: none


### ` getCaAtbSink() `
Reads the Ca Implementation ATB Sink field.

Parameters: none


### ` getCaPibSink() `
Reads the Ca Implementation PIB Sink field.

Parameters: none


### ` getCaSbaSink() `
Reads the Ca Implementation SBA Sink field.

Parameters: none


### ` getCaFunnelSink() `
Reads the Ca Implementation Funnel Sink field.

Parameters: none


### ` getCaSinkBytes() `
Reads the Ca Implementation Sink Bytes Sink field.

Parameters: none


### ` getCaSinkData() `
Reads the 32-bit Ca Sink Data register.

Parameters: none


### ` getCaSinkWpReg(core), setCaSinkWpReg(core, value) `
Reads the 32-bit Ca Sink WP register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 


### ` getCaWrap(core), setCaWrap(core, opt) `
Reads the 32-bit Ca Sink Wrap register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `opt`
  *  `TE_ENABLE`
  *  `TE_DISABLE` 


### ` getCaSinkRpReg(core), setCaSinkRpReg(core, value) `
Reads the 32-bit Ca Sink RP register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.
* `value`
  *  32-bit value 

### Printing Macros
The register dump macros print the values of a register, or set of registers, to the console. Single register dumps will print the meaning of each field's value along with the field`d value. These macros are meant as debug only. 

### `teRegDump(core)`
Will print the address and value of all of the trace encoder (TE) registers: `CONTROL`, `IMPL`, `SINBASE`, `SINKBASE HIGH`, `SINKBASE LIMIT`, `SINK WP`, `SINK RP`, `SINK DATA`, `FIFO`, `BTM COUNT`, `WORD COUNT`, `TS CONTROL`, `TS LOWER`, `TS UPPER`, `XTI CONTROL`, `XTO CONTROL`, `WP CONTROL`, `ITC TRACE ENABLE`, and `ITC TRIG ENABLE`.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `tfRegDump(core)`
Will print the address and value of all of the trace funnel (TF) registers: `CONTROL`, `IMPL`, `SINBASE`, `SINKBASE HIGH`, `SINKBASE LIMIT`, `SINK WP`, `SINK RP`, and `SINK DATA`.

Parameters: none

### `caRegDump(core)`
Will print the address and value of all of the cycle accurate trace (CA) registers: `CONTROL`, `IMPL`,  `SINK WP`, `SINK RP`, and `SINK DATA`.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `teControlDump(core)`
Will print both the value and the meaning of the value for each field in the teControl register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `tsControlDump(core)`
Will print both the value and the meaning of the value for each field in the tsControl register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `implControlDump(core)`
Will print both the value and the meaning of the value for each field in the implControl register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `xtiControlDump(core)`
Will print both the value and the meaning of the value for each field in the xtiControl register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `xtoControlDump(core)`
Will print both the value and the meaning of the value for each field in the xtoControl register.

Parameters:
* `core`
  * Integer number of the core to act upon, 0...N-1, where N is the number of cores.

### `tfControlDump(core)`
Will print both the value and the meaning of the value for each field in the tfControl register.

Parameters: none


## Examples
To collect trace files either an OpenOCD telnet session can be used to interface with the board in which the `wtb` and `wcab` commands can be used to write the trace and cycle accurate trace files to your local machine, or if debugging through Freedom Studio, the "Collet trace data when target halts" feature can be used to collect the trace files automaticly (TE and CA). If using Freedom Studio durring a debug session its advised to disable tracing under "Trace Control" so that the trace control registers set by the program are not overwritten.

Addresses for each trace encoder core, cycle accurate trace encoder core, and funnel can be found in design.dts for your particular IP package.

### Collecting a trace for a section of code
The below example traces the calculation of the 10th Fibonacci number, all other code in this example will not be traced.
```
#include <stdio.h>
#include "sifive_trace.h"

int fib(int a)
{
  if (a == 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a-2) + fib(a-1);
}

int main() {

  // Create the register map objects.
  // set traceBaseAddress (address differes by IP)

  #define traceBaseAddress 0x10000000

  // create the trace memory map object, with N cores

  struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)traceBaseAddress};

  // set tfBaseAddress (address differes by IP)

  #define tfBaseAddress 0

  // create the trace funnel memory map object

  struct TfTraceRegMemMap volatile * const fmm = 0;

  // configure all cores` trace encoder to default parameters

  traceConfigDefaults(TRACE_CORES_ALL);

  // Enable tracing on all cores

  Trace(TRACE_CORES_ALL, TRACE_ON);
		
  // Tracing starts here

  int f = fib(10);

  // Disable tracing on all cores

  Trace(TRACE_CORES_ALL, TRACE_OFF);
		
  // Tracing is now off

}

```

### Collecting a cycle-accurate trace for a section of code
The below example traces the calculation of the 10th Fibonacci number, all other code in this example will not be traced. The inclusion of the Fibonacci before tracing starts is used to warm up the instruction cache. If the code being traced is running from serial flash, most of the cycle accurate trace information will be for empty cycles while the processor is stalled waiting for instructions to be fetched. Warming the cache beforehand eliminates the stalls. If a breakpoint is set between the warmup code and the code desired to be traced, the breakpoint will flush the instruction cache. 
```
#include <stdio.h>
#include "sifive_trace.h"

int fib(int a)
{
  if (a == 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a-2) + fib(a-1);
}

int main() {

  // Create the register map objects.
  // set traceBaseAddress (address differes by IP)

  #define traceBaseAddress 0x10000000

  // create the trace memory map object, with one core

  struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)  traceBaseAddress};

  // set caBaseAddress (address differes by IP)

  #define caBaseAddress 0x1000f000

  // create the cycle accurate trace memory map object

  struct CaTraceRegMemMap volatile * const cmm[] = {(struct CaTraceRegMemMap*)caBaseAddress};

  // set tfBaseAddress to zero to indicate that the funnel is not in use

  #define tfBaseAddress 0

  // create the trace funnel memory map object

  struct TfTraceRegMemMap volatile * const fmm = (struct TfTraceRegMemMap*)tfBaseAddress;

  // configure core 0`s TE and CA trace encoders to default parameters

  caTraceConfigDefaults(0);

  // Added instruction to warm up the instruction cache

  for (int i = 0; i < 10; i++) {
    int f = fib(10);
  }

  // Enable tracing on core 0

  Trace(0, TRACE_ON);
	
  // Tracing starts

  int f = fib(10);

  // Disable tracing on core 0

  Trace(0, TRACE_OFF);

  // Tracing is now off

}

```

### Collecting a multi-core trace for a section of code using the funnel
The below example traces the calculation of the 10th Fibonacci number, all other code in this example will not be traced.
```
#include <stdio.h>
#include "sifive_trace.h"

int fib(int a)
{
  if (a == 0) {
    return 0;
  }
  if (a == 1) {
    return 1;
  }
  return fib(a-2) + fib(a-1);
}

int main() {
	
  // Create the register map objects.
  // set traceBaseAddress (address differes by IP)

  #define traceBaseAddress0 0x10000000
  
  // Repeat for all cores

  #define traceBaseAddressN 0x1F000000

  // create the trace memory map object, with N cores

  struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)traceBaseAddress0, ..., (struct TraceRegMemMap*)traceBaseAddressN};

  // set tfBaseAddress (address differes by IP)

  #define tfBaseAddress 0x20000000

  // create the trace funnel memory map object

  struct TfTraceRegMemMap volatile * const fmm = (struct TfTraceRegMemMap*)tfBaseAddress;
	
  // configure all cores` trace encoder to default parameters

  traceConfigDefaults(TRACE_CORES_ALL);
  tfConfigDefault();

  // Enable tracing on all cores

  Trace(TRACE_CORES_ALL, TRACE_ON);

  // Tracing starts

  int f = fib(10);
	
  // Disable tracing on all cores

  Trace(TRACE_CORES_ALL, TRACE_OFF);

  // Tracing has stoped

}
```

### Generate ITC Trace Messages Through the Stimulus Register
This example will generate an ITC trace message with the value `0x31415926` through the ITC stimulus register using channel 0.
```
#include "sifive_trace.h"

int main() {

  // Create the register map objects.

  #define traceBaseAddress 0x10000000

  // create the trace memory map object, with one core

  struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)traceBaseAddress};

  #define caBaseAddress 0x1000f000

  // create the cycle accurate trace memory map object

  struct CaTraceRegMemMap volatile * const cmm[] = {(struct CaTraceRegMemMap*)caBaseAddress};

  // set tfBaseAddress to zero to indicate that the funnel is not in use

  #define tfBaseAddress 0

  // create the trace funnel memory map object

  struct TfTraceRegMemMap volatile * const fmm = (struct TfTraceRegMemMap*)tfBaseAddress;

  // Enable ITC proccessing on core 0

	setTeInstrumentation(0, TE_INSTRUMENTATION_ITC);

  // Enable a specific ITC channel to write to

  ItcEnableChannel(0, 0x1);

  // Code 
	
  // Write 0x31415926 to a specified ITC channel

  ItcWrite(0, 0x1, 0x31415926);

  // Code 

}
```

### Print ITC string using ITC-Util's itc_printf
This example will print the ITC trace message `Hello World!` through the ITC stimulus register using channel 0.
```
#include "sifive_trace.h"
#include "itc_utils.h"

int main() {

  // Create the register map objects.

  #define traceBaseAddress 0x10000000

  // create the trace memory map object, with one core

  struct TraceRegMemMap volatile * const tmm[] = {(struct TraceRegMemMap*)traceBaseAddress};

  #define caBaseAddress 0x1000f000

  // create the cycle accurate trace memory map object

  struct CaTraceRegMemMap volatile * const cmm[] = {(struct CaTraceRegMemMap*)caBaseAddress};

  // set tfBaseAddress to zero to indicate that the funnel is not in use

  #define tfBaseAddress 0

  // create the trace funnel memory map object

  struct TfTraceRegMemMap volatile * const fmm = (struct TfTraceRegMemMap*)tfBaseAddress;

  // Enable ITC proccessing on core 0

	setTeInstrumentation(0, TE_INSTRUMENTATION_ITC);

  // Enable a specific ITC channel to write to

  ItcEnableChannel(0, 0x0);

  // Code 
	
  // Write 'Hello World!' to a specified ITC channel using ITC_Utils

  itc_printf("Hello World!");

  // Code 

}
```
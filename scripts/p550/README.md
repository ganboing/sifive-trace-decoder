***Hifive P550 N-Trace How To***
---

### Prepare the board
* Reserve the trace buffer in Linux. Patch the Linux device tree with the following:
```
--- /dev/fd/63	2024-11-21 23:46:52.706572872 +0000
+++ /dev/fd/62	2024-11-21 23:46:52.710572842 +0000
@@ -360,6 +360,11 @@
 			phandle = <0x21>;
 			reg = <0x00 0x59000000 0x00 0x400000>;
 		};
+
+		trace_4g {
+			no-map;
+			reg = <0x01 0x00 0x01 0x00>;
+		};
 	};
 
 	soc {
@@ -4677,6 +4682,12 @@
 			sifive,perfmon-counters = <0x06>;
 		};
 
+		pmem_trace@100000000 {
+			compatible = "pmem-region";
+			reg = <0x01 0x00 0x01 0x00>;
+			volatile;
+		};
+
 		pmu@50c02000 {
 			compatible = "arm,smmu-v3-pmcg";
 			eswin,syscfg = <0x16 0x3fc>;
```
  This will reserve a 4GB buffer at `0x100000000 (4GB)`. We'll use this buffer to hold trace data. It'll also add a pmem device that exposes the buffer as `/dev/pmem0`

* We use the address at 4GB boundary because it should be free at all boot stages: opensbi/u-boot. \
  The stock u-boot scripts will not load files into >= 4GB addresses, and the u-boot UEFI runtime services are located \
  at just below the end of main memory. Given we have 16GB DDR memory starting at 0x80000000 (2G), addresses between `[4GB, 8GB)` are safe.

### Tracing with OpenOCD
* clone this repo and run `openocd -f openocd_mcpu.cfg` to attach to the jtag of the board. (Given you have already connected the USB cable to the debug port of p550).
* Init tracing and verify that you observe the same command output:
```
> source trace.tcl ; # Init tracing
TeControlReg 100000 -> 81a3107b
TeControlReg 100000 <- 81a31079
TeControlReg 101000 -> 81a3107b
TeControlReg 101000 <- 81a31079
TeControlReg 102000 -> 81a3107b
TeControlReg 102000 <- 81a31079
TeControlReg 103000 -> 81a3107b
TeControlReg 103000 <- 81a31079
TfControlReg 18000 -> 70000003
TfControlReg 18000 <- 70000001

> tsallstart ; # Start timestamping
tsEnable[0]: tsctl=280f8041
tsEnable[1]: tsctl=280f8041
tsEnable[2]: tsctl=280f8041
tsEnable[3]: tsctl=280f8041
tsEnable[funnel]: tsctl=28000023
tsStart[0]: tsctl=280f8041
tsStart[1]: tsctl=280f8041
tsStart[2]: tsctl=280f8041
tsStart[3]: tsctl=280f8041
tsStart[funnel]: tsctl=28000023

> dts ; # Show timestamping
dts: core[0]: ts_control=0x280f8041 ts_width=40 ts=0x00000000130f01c0 <- Observe ts is chaning for hart 0
dts: core[1]: ts_control=0x280f8041 ts_width=40 ts=0x00000000184e6eff <- Observe ts is chaning for hart 1
dts: core[2]: ts_control=0x280f8041 ts_width=40 ts=0x000000001d918172 <- Observe ts is chaning for hart 2
dts: core[3]: ts_control=0x280f8041 ts_width=40 ts=0x0000000022c74974 <- Observe ts is chaning for hart 3
dts: master funnel @0x00018000: ts_control=0x28000023 ts_width=40 ts=0x0000000026cdf412 <- Observe ts is changing for funnel

> tracemode all htm ; # Set trace mode to htm (History Trace Messaging) mode

> tracedst SBA 0x100000000 0xfffffff8 ; # Set trace buffer to [4G, 8G - 8)
txSetSink(100000): sink=8 impl=2040100
txSetSink(100000): enabling sink FUNNEL
txSetSink(101000): sink=8 impl=12140100
txSetSink(101000): enabling sink FUNNEL
txSetSink(102000): sink=8 impl=22240100
txSetSink(102000): enabling sink FUNNEL
txSetSink(103000): sink=8 impl=32340100
txSetSink(103000): enabling sink FUNNEL
txSetSink(18000): sink=7 impl=10080
txSetSink(18000): enabling sink SBA
txSetSink[funnel]: setting SBA sink to 0x100000000+0xfffffff8
_txProbeMemBaseAlignBits(18000): align=3
_txProbeMemBaseMaxBits(18000): maxbit=41
_txProbeMemBaseAlignBits(18000): align=3
_txProbeMemLimitAlignBits(18000): align=3
_txProbeMemLimitMaxBits(18000): maxbit=32
_txProbeMemLimitAlignBits(18000): align=3

> trace all settings ; # dump trace settings
ts: core 0: running; core 1: running; core 2: running; core 3: running; core funnel: intrunning
tsitc: core 0: on; core 1: on; core 2: on; core 3: on; core funnel: off
tsclock: core 0: slave; core 1: slave; core 2: slave; core 3: slave; core funnel: internal
tsowner: core 0: on; core 1: on; core 2: on; core 3: on; core funnel: off
tsbranch: core 0: all; core 1: all; core 2: all; core 3: all; core funnel: off
tsnonstop: core 0: off; core 1: off; core 2: off; core 3: off; core funnel: off
tracemode: core 0: htm+sync; core 1: htm+sync; core 2: htm+sync; core 3: htm+sync
tsprescale: core 0: 1; core 1: 1; core 2: 1; core 3: 1; core funnel: 1
stoponwrap: core 0: off; core 1: off; core 2: off; core 3: off; core funnel: off
itc: core 0: none; core 1: none; core 2: none; core 3: none; core funnel: none
maxbtm: core 0: 8; core 1: 8; core 2: 8; core 3: 8
maxicnt: core 0: 14; core 1: 14; core 2: 14; core 3: 14
```
* At this point tracing is successfully initialized. Now we can start tracing
```
> resume ; # Resume the Harts
Disabling abstract command writes to CSRs.
Disabling abstract command writes to CSRs.
Disabling abstract command writes to CSRs.
Disabling abstract command writes to CSRs.

> trace all reset ; # Reset the write pointer
> trace all start ; # Start tracing
> dtr ; # Dump trace registers
dtr: core[0] ctrl=89a3107f impl=02040100 base=000000000 limit=0 wp=0 rp=0 xti=0 xto=0 wpctrl=0 itctrig=0 itctrace=0
dtr: core[1] ctrl=89a3107f impl=12140100 base=000000000 limit=0 wp=0 rp=0 xti=0 xto=0 wpctrl=0 itctrig=0 itctrace=0
dtr: core[2] ctrl=81a3107f impl=22240100 base=000000000 limit=0 wp=0 rp=0 xti=0 xto=0 wpctrl=0 itctrig=0 itctrace=0
dtr: core[3] ctrl=81a3107f impl=32340100 base=000000000 limit=0 wp=0 rp=0 xti=0 xto=0 wpctrl=0 itctrig=0 itctrace=0
dtr: core[funnel] ctrl=70000003 impl=00010080 base=100000000 limit=fffffff8 wp=375670 rp=0
```
* You should be able to observe the `wp` (write pointer) incrementing. You can safely ignore all the `Failed to read priv register.` messages, as the hart registers are not available when not halted, but the trace registers can be directly accessed by the debug module, so we can still control trace while the hart is running.
* After some time, stop the trace:
```
> trace all stop ; # Stop tracing
> dtr ; # Dump trace registers
...
dtr: core[funnel] ctrl=70000003 impl=00010080 base=100000000 limit=fffffff8 wp=656c78 rp=0
```
* The final value of `wp` will be the length of the trace emitted into the buffer (when not wrapped). If the buffer is wrapped around, the LSB of `wp` will be set to 1.

### Dump trace buffer
* Dumping using dd from within Linux, notice the `wp` value used from previous step
```
$ dd if=/dev/pmem0 iflag=fullblock of=ntrace.bin bs=$(( 0x656c78 )) count=1
```

### Decode with NexRv
* Use the modified version with SrcBits=2 support: [repo](https://github.com/ganboing/tg-nexus-trace/tree/dev-p550/refcode/c)
```
$ NexRv.exe -dump ntrace.bin
0x74 011101_00: TCODE[6]=29 (MSG #585) - IndirectBranchHistSync
0x24 001001_00: SrcBits[2]=0x1 SYNC[4]=0x2
0x41 010000_01: BTYPE[2]=0x0 ICNT[4]=0x4
0x2C 001011_00:
0xA0 101000_00:
0x04 000001_00:
0x00 000000_00:
0x00 000000_00:
0x05 000001_01: FADDR[36]=0x40001A0B
0x0D 000011_01: HIST[6]=0x3
0x3C 001111_00:
0x74 011101_00:
0xA0 101000_00:
0xA0 101000_00:
0xCC 110011_00:
0xFC 111111_00:
0x13 000100_11: TSTAMP[42]=0xF3A2874F
...
```
* The 2-bit SrcBits denotes the Hart ID. FADDR/UADDR addresses are omitting the LSB of $pc, as all instructions must be aligned to the 2-byte boundary. For the meaning of different fields, refer to the [N-Trace Spec](https://github.com/riscv-non-isa/tg-nexus-trace/blob/main/pdfs/RISC-V-N-Trace.pdf)

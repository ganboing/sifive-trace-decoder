# itc_utils.c

The itc print functions facilitate embedding printed data into the collected trace, which the decoder later reconstructs the prints from. The `itc_printf()` and `itc_puts()` functions mirror the `printf()` and `puts()` functions included from `stdio.h`. The itc_nls_print functions allow a more effiecent mechanism to send data, not requiring the actual text format strings to be stored on the target or saved in the trace buffer.


The No-Load String (NLS) functions allow for the ITC stimulus register to be used to inject messages into the decoded trace output without the strings actually be written to the trace or stored in the traget's memory. This has the effect of allowing human-readable, predefined strings to be printed using dqr or Freedom Studio with only the numeric data stored in the trace buffer, permitting simplified debugging, without taking up storage space on the target device nor bandwidth over the debug link. The NLS strings are specified, alongside the channel they're assigned to, using the `#ident` pragma in one of the projects C files using the format `#ident "channel number:formattable string"`, and during linking the strings get added to the `.comment` section of the elf file. Note, only one string can be specified per channel, and only channels 0 through 15 can be specified. In addition, the specified no-load strings take precedent over other itc print, such as `itc_printf()` or `itc_write32()`, and using those functions on a channel with a specified no-load string might result in unexpected outputs. The NLS print functions take between zero and for arguments plus a channel number between 0 and 15 to print to. The optionally passed arguments get concatenated into the 32 bit ITC stimulus register at the specified channel, and only one write is performed. The options for passed variables are no-argument, 1 32 bit, 2 16 bit, 2 11 bit plus 1 10 bit, or 4 8 bit integers. When decoding the trace file, the NLS prints to channels that have strings specified to them in the `.comments` section are expanded. The dqr program when handling no-load strings determines how many variables are concatenated into the stimulus register by the number of format tags (`%[Flag]`) in the defined string for that channel. The extracted variables before printing get extended to 32 bits (either signed or unsigned extension based on the format flag). If any of the no-load string print functions are used on a channel that does not have a defined string, then the contents of the stimulus register will be handledin the same manner as an `itc_write()`, the trace decoder will display the Nexus trace message containing the channel number and written data. 

There are 32 ITC channels, but writing the upper 16 will map to the lower 16 with the addition of a timestamp in the trace message. For example, writing to channel 17 will produce a trace message for channel 2, with the addition of a timestamp. The format string defined for channel 2 will be used. Writing to these upper 16 channels is possible by adding  `ITC_TS_CHANNEL_OFFSET` to the desired channel number (e.x. `itc_nls_printstr(2+ITC_TS_CHANNEL_OFFSET)`).

Three helper functions to setup the itc channels are provided: `itc_enable()`, `itc_disable()`, and `itc_set_print_channel()`. Both  `itc_enable()` and `itc_disable()` take a single parameter, which channel to act upon, and enables or disables that channel for stimulus writes. Enabling or dissabling channels can also be done through the trace configuration tool in Freedom Studio. By default, all channels are disabled. The `itc_set_print_channel()` takes a channel number between 0 and 31, and subsequent `itc_puts()` and `itc_printf()` function calls will print to that channel, until the channel is changed using the `itc_set_print_channel()` again.

In addition, there are built in methods to create parings between defined performance counters and ITC channels, allowing for the injection of the 64 bit coutner value into the specified stimulus register through two, 32-bit writes. 

Note: The itc_enable/dissable functions are for specific channels, and not to enable or disable itc functionality on the target. For that Freedom Studio or Sifive_trace.h can be used to enable ITC mode.


## int itc_enable(int channel)
Enables one of the ITC stimmulus channels, 0-31, bassed of the value of the parameter. If the value `ITC_ALL_CHANNELS` is passed as the channel argument, all 32 channels will be enabled. Returns 1 if the specified channel is not in the range 0-31, otehrwise 0. 


## int itc_disable(int channel)
Enables one of the ITC stimmulus channels, 0-31, bassed of the value of the parameter. If the value `ITC_ALL_CHANNELS` is passed as the channel argument, all 32 channels will be disabled. Returns 1 if the specified channel is not in the range 0-31, otherwise 0. 


## int itc_set_print_channel(int channel)
Sets a specified channel, between 0 and 31, to be used by subsequent `itc_puts()` and `itc_printf()` function calls. Default value for `itc_puts()` and `itc_printf()` is channel 0 before this function is called for the first time. Returns 1 if the specified channel is not in the range 0-31, otherwise 0. 


## int itc_puts(const char *f)
Injects the passed c-style string into the trace buffer at the channel specified by the previous `itc_set_print_channel()` call, or channel 0 if there has yet to be a `itc_set_print_channel()` call. Appends a new line character to the end of the string. Returns the number of characters written to the stimulus register.

## int itc_printf(const char *format, ... )
Formats the passed string using the provided arguments, then injects the string into  trace buffer at the channel specified by the previous `itc_set_print_channel()` call, or channel 0 if there has yet to be a `itc_set_print_channel()` call. Returns the number of characters written to the stimulus register.


## int itc_nls_print(int channel)
Writes 0 to the ITC stimmulus register at the specified channel. Used for No Load String printing without variable parameters.

## int itc_nls_print_i32(int channel, uint32_t data1)
Writes `data1` to the ITC stimmulus register at the specified channel. Used for No Load String printing with one variable parameter.


## int itc_nls_print_i16(int channel, uint16_t data1,  uint16_t data2)
Writes `data1:data2` to the ITC stimmulus register at the specified channel. Used for No Load String printing with two variable parameter.

## int itc_nls_print_i11(int channel, uint32_t data1,  uint16_t data2,  uint16_t data3)
Writes `data1:data2:data3` to the ITC stimmulus register, with `data1` and `data2` truncated to 11 bits (Most significant bits cleared) and `data3` truncated to 10 bits  uint16_t data1, at the specified channel. Used for No Load String printing with three variable parameter.

## int itc_nls_print_i8(int channel, uint8_t data1,  uint8_t data2, uint8_t data3,  uint8_t data4)
Writes `data1:data2:data3:data4` to the ITC stimmulus register at the specified channel. Used for No Load String printing with two variable parameter.

## struct metal_cpu *init_pc()
Sets up the calling cpu's performance counters. Returns the cpu object's address if successful, needed for future function calls, NULL of failure.

## int set_pc_channel(int hpm_coutner, int channel, struct metal_cpu *cpu)
Creates a linking between a performance counter and a channel, will override any previous parings set to a performance counter. Multiple performance counters can be paired to the same channel. Return 1 on success, 0 on failure.

## int inject_pc(int hpm_counter, struct metal_cpu)
Write two, 32-bit messages to the ITC stimmulus register at the paired channel for the performance counter, with the loswer 32-bits of the register printed first. If a pairing between performance counter and itc channel has yet to be made, returns 0, otherwise 1 on success.

## int reset_pc_counter(int hpm_counter, struct emtal_cpu)
Sets the performance counter to zero. returns 0 on failure, 1 on success.

## Example:
### Example Code
Below is an example program to demonstrate the functionality of itc_utils. If using Freedom Studio, ITC can be enabed through the trace control dialog instead of `using sifive_trace.h`.


```
/* Copyright 2021 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include "itc_utils.h"
#include "sifive_trace.h"
#include <metal/hpm.h>
#include <stdio.h>

// No load string definitions for channels 1-5
#ident "1:This is a No-Load String!"
#ident "2:Uint32 is %x"
#ident "3:Uint16's are %u and %d"
#ident "4:Uint8's are %c, %c, %c, and %d"
#ident "5:Uint11's are %x and %x, Uint10 is %x"


int main() {

    // Use Sifive_Trace to enbale ITC on the target
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

    // use ITC_Utils to enable a specific channel (could also be done using Sifive_trace)
	// Enable all ITC channels
	itc_enable(ITC_ALL_CHANNELS);

	// Have the DQR print the no-load string "This is a No-Load String!"
	itc_nls_printstr(1);

	// Have the DQR print the no-load string "Uint32 is 12345678"
    uint32_t foo = 0x12345678;
    itc_nls_print_i32(2, foo);

    // Have the DQR print the no-load string "Uint16's are 1111 and -2222"
    itc_nls_print_i16(3, 1111, -2222);

    // Have the DQR print the no-load string "Uint11's are aa and bb, Uint10 is cc"
    itc_nls_print_i11(5, 0xaa, 0xbb, 0xcc);

    // Have the DQR print the no-load string "Uint8's are f, o, o, and 42"
    itc_nls_print_i8(4, 'f', 'o', 'o', 42);

    // Print the string "Puts: Foo!" using itc_puts
    itc_puts("Puts: Foo!\0");

    // Print the string "Printf: Foo!\n" using itc_print and a formated string
    itc_printf("Printf: %s\n", "Foo!");

    // get this CPU's object
    struct metal_cpu *this_cpu =  init_pc();

    // check to see if the CPU is valid
    if (this_cpu == NULL) printf("NULL CPU!\n");

    // set the cycle counter pc register to be paired with ITC channel 6
    set_pc_channel(METAL_HPM_CYCLE, 6, this_cpu);

    // Write the value of the cycle PC register to the ITC stimulus register
    inject_pc(METAL_HPM_CYCLE, this_cpu);

    // Clear the Cycle coutner PC register
    reset_pc_counter(METAL_HPM_CYCLE, this_cpu);

    // Re-write the value of the cycle PC register to the ITC stimulus register to verify it was reset
    inject_pc(METAL_HPM_CYCLE, this_cpu);

    return 0;

}
```

### Expected Output
Below is the expected DQR output from proccessing a trace generated from the code above. 
```
dqr -t trace.rtd -e src\debug\hello.elf -trace -itcprint
ITC Print: This is a No-Load String!
ITC Print: Uint32 is 12345678
ITC Print: Uint16's are 1111 and -2222
ITC Print: Uint11's are aa and bb, Uint10 is cc
ITC Print: Uint8's are f, o, o, and 42
ITC Print: Puts: Foo!
ITC Print: Printf: Foo!

End of Trace File
```

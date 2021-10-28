# trace-decoder
SiFive implementation of the Nexus trace decoder

The trace-decoder is a stand-alone program that reads Nexus 5001 traces for the RISC-V architecture as implemented by SiFive and reconstructs the instruction stream that generated the trace. Instruction traces can be used for debugging and performance analysis. Currently, Windows, Linux, and OS X are supported.

The process to create/capture a trace file is not covered here; only how to build and use the trace-decoder tool to reconstruct the program execution that generated the trace. Usage of the trace-decoder assumes you already have or know how to create trace files.

This document describes the SiFive trace-decoder, both how to build and how to use.

The name of the stand-alone trace-decoder is `dqr` (for de-queuer). There is also a dynamic library created for Windows, Linux, and Mac OS X (dqr.dll, or libdqr.so). Currently, Freedom Studio uses the  dynamic library to perform trace decodes.

### Licensing:

The trace-decoder is released under the GPL 3 License. For a full copy of the license, see the COPYING file included with the trace-decoder. The licensing statement is:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

In addition, the trace-decoder is statically linked with various licensed libraries. The libraries are listed below, along with their licensing:

	libbfd       GNU GPL V3
	libopcodes   GNU GPL V3
	libiberty    GNU LGPL V2.1
	linintl      GNU LGPL V2.1
	libz         Zlib/libpng License

The GNU GPL V3 license is in the file COPYING; the GNU LGPL V2.1 license is in the file COPYING.LIB; the zlib/libpng License is in the file zlib source file zlib.h, and does not require distribution with the library binary.

These libraries, along with much more, have been ported to RISC-V by SiFive, and are available at the link:

https://github.com/sifive/riscv-binutils-gdb

The binaries for these libraries for all supported platforms are included in this distribution in the lib folders.

### Features Summary as of Version 0.13.0

Available both as a stand-alone executable to decode traces, and as a library suitable for being used as part of a different project. The library also provides interfaces that can be called from Java or C++.

Decodes 32 and 64 bit traces.

Supports both SiFive BTM (Branch Trace Mode) and HTM (History Trace Mode) trace decoding.

Supports SiFive event trace decoding. When decoding event traces, textual event files can be optionally generated that can be processed by user tools.

Multi-core Support. The trace-decoder supports multi-core traces, up to 8 cores through the use of the -srcbits=n switch. When decoding multi-core traces, each line output is prefixed with a core number for identification.

Limited support for generating CTF (Common Trace Format) output files that can be processed by Trace Compass to dispaly Flame Charts and Graphs.

Will process simulator output files and generate a decoded trace-type output

Can process both SiFive Scalar and Vector cycle accurate traces, generating a decoded instruction output with cycle information. For vector traces, varrious vector unit cycle information is also included in the output. See "trace-deocder/doc/ca trace users guide.md" for more information.

Includes separate itc_utils.c, itc_utils.h files that can be used in the project being traced to output printf-like output directly into the trace stream, using ITC trace messages. The trace decoder can reassemble the ITC messages back into printf-like message and dispaly or make them available to the calling program. Also supported are no-load-string prints, where the textual portion of prints are not embedded in the trace for improved trace bandwidth and trace buffer usage. See trace-decoder/doc/ITCPrint.md for more information.

Includes sifive_trace.h, a comprehensive C macro library that can be used in programs under trace to control the trace process from inside the program under trace. See trace-decoder/doc/sifive_trace.md for more information.

Includes serveral example Java programs to demonstrate how to use the library from Java.

A separate trace-decoder-tests project on GitHub was added that can be used to test a new trace-decoder build and verify its functionality. It is available at https://github.com/sifive/trace-decoder-tests. Information on how to run the tests is included with that package.

Builds and runs on Windows, Linux, and Mac OS X.

### Building:

The trace-decoder will build on Windows, Linux, and Mac OS X.  To build the trace-decoder, the correct build tools must first be installed. On windows, MinGW and MSYS must be installed.  For Linux, g++ must be installed. On Mac OS X, use either XCode or g++. The trace-decoder can be built by itself, or as part of a Freedom-Tools build.

#### Prerequisites:

The trace-decoder compiles as a 64-bit application.

Below are the required tools that need to be installed for each of the supported platforms:

Windows:

To build under windows, you will need to install both MinGW-W64 and MSYS.

MinGW-w64 (with gcc 8.1.0 or newer) and MSYS2:

A working MinGW-w64 installer can be downloaded from [sourceforge](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe). 

When you run the installer for MinGW/MSYS, choose the *posix* threads, and *seh* exceptions.

If you are planning on using eclipse, install MinGW-w64 in a folder with no spaces in the path (for example, do not install it in "Program Files"). Chose the x86-64 architecture when installing.

You will also need to edit your PATH environment variable to include MinGW-w64. If for example, you installed MinGw-w64 in c:\mingw, you should add c:\mingw\bin to your path.

MSYS:

The MSYS installer can be downloaded from http://downloads.sourceforge.net/mingw/MSYS-1.0.11.exe. After downloading, run the installer and follow the instructions.

Linux:

To build on Linux, you will need to install g++ and the GNU binutils. This process varies for different Linux distributions. If you do not know how to do this, a quick web search for your distribution should provide a solution. Make sure you install the compilers that produce 64-bit code and not 32-bit code.

OS X:

You can use either XCode or the GNU g++/gcc and binutils tools. XCode is probably the easiest. If you do not already have XCode installed on your system, you can go to the Apple app store to download.

XCode or g++ (note: XCode uses the gcc/g++ front end, but uses the LLVM compiler chain. This means that XCode is not using the g++/gcc compilers). Either toolchain should work fine for compiling the trace-decoder, but only the XCode tool chain has been verified to work.

#### Build Targets

To build the trace-decoder, you can just type "make" on the command line from the trace-decoder directory. This will build the debug version of the trace-decoder, both its executable and dynamic library. Specific build targets include:

clean: Removes all temporary files created during a build.

Debug:  Builds the debug version (with symbol information). Using make with no arguments is the same as "make Debug".

Release: Build the release version.

install: Copies the executable, trace-decoder library, Windows dynamic libraries needed for the trace-decoder library, header files, and examples to the directory specified by the INSTALLPATH define, as in make install INSTALLPATH=<path to install at>. If INSTALLPATH is not specified, creates an directory "install" in the trace-decoder directory with the components placed in it.

#### Directory Structure of Source:

The directory structure for the trace decoder is:

	trace-decoder\
		include\
		src\
		lib\
			linux\
			mcaos\
			windows\
		Debug\
		Release\
		Doc\
		examples\
			itc-utils\
			java\
		scripts\

All source files for building the trace decoder are in src or include. The file main.cpp contains the driver program for the trace-decoder, but also can be used as a guide to building your own front-end which uses the Trace class for creating instruction streams from the collected trace information.

When building without a Freedom Tools build, the libraries located in trace-decoder/lib will be used (for Windows, Linux, or Mac OS X). These libraries are not the most up-to-date but should function correctly, and are provided so the trace decoder can be built without Freedom Tools. When build as part of a Freedom Tools build, the libraries built as part of the Freedom Tools build are used.

The trace-decoder can be built on the command line or within Eclipse.

#### Using Eclipse

Below is a workflow for getting the project into Eclipse:

 1. Clone the repo to your normal git repository (typically <home>/git) using whatever method you like
 2. Within Eclipse, import this repository/project directly into Eclipse using `File -> Import -> Git -> Projects from Git`
 3. Select "Existing local repository"
 4. Add the repository if not already added (you will need to do this only once)
 5. Search the repository for Eclipse projects (there should only be one in the result list)
 6. Check the 'trace-decoder' project (if not already checked)
 7. Finish the wizard to create the new project in your workspace.
 
When you do it this way you will be working on the files directly in the local git repository.  These files do not actually live in your workspace.  You can still easily interact with your repo from a git CLI.
  
The Eclipse project includes "build targets" to make for easy building in Eclipse.

#### Using the CLI

The makefile includes two configurations:  `Debug` and `Release` (they are capitalized).

The default configuration is `Debug`, so if you `make clean` or `make all` or `make clean all` you will act on the `Debug` configuration and all build artifacts will be in the `Debug` folder.

To build the `Release` configuration, use `make CONFIG=Release all` (This is what Jenkins will use to build a release)

On the Windows and Linux versions, all libraries are statically linked in the executable. On OS X, only the libbfd, libopcodes, libibery, libintl, and libz libraries are statically linked.

#### Notes

There is a `makefile.init` at the top level that contains the settings that apply to all configurations, including a list of OBJ files that needs to be updated when new source files are added. Each config sub-folder has a makefile that contains settings for the specific configuration (like optimization flags that generally differentiate a Release build from a Debug build).

### Running:

Use `dqr -h` to display usage information. It will display something like:

```
Usage: dqr -t tracefile -e elffile [-ca cafile -catype (none | instruction | vector)] [-btm | -htm] -basename name
           [-pf propfile] [-srcbits=nn] [-src] [-nosrc] [-file] [-nofile] [-func] [-nofunc] [-dasm] [-nodasm]
           [-trace] [-notrace] [-pathunix] [-pathwindows] [-pathraw] [--strip=path] [-itcprint | -itcprint=n] [-noitcprint]
           [-addrsize=n] [-addrsize=n+] [-32] [-64] [-32+] [-archsize=nn] [-addrsep] [-noaddrsep] [-analytics | -analyitcs=n]
           [-noanalytics] [-freq nn] [-tssize=n] [-callreturn] [-nocallreturn] [-branches] [-nobranches] [-msglevel=n]
           [-cutpath=<base path>] [-s file] [-r addr] [-labels] [-nolables] [-debug] [-nodebug] [-v] [-h]

-t tracefile: Specify the name of the Nexus trace message file. Must contain the file extension (such as .rtd).
-e elffile:   Specify the name of the executable elf file. Must contain the file extension (such as .elf).
-s simfile:   Specify the name of the simulator output file. When using a simulator output file, cannot use
              a tracefile (-t option). Can provide an elf file (-e option), but is not required.
-pf propfile: Specify a properties file containing information on trace. Properties may be overridden using command
              flags.
-ca cafile:   Specify the name of the cycle accurate trace file. Must also specify the -t and -e switches.
-catype nn:   Specify the type of the CA trace file. Valid options are none, instruction, and vector
-btm:         Specify the type of the trace file as btm (branch trace messages). On by default.
-htm:         Specify the type of the trace file as htm (history trace messages).
-n basename:  Specify the base name of the Nexus trace message file and the executable elf file. No extension
              should be given. The extensions .rtd and .elf will be added to basename.
-cutpath=<cutPath>[,<newRoot>]: When searching for source files, <cutPath> is removed from the beginning of thepath name
              found in the elf file for the source file name. If <newRoot> is given, it is prepended to the begging of the
              after removing <cutPath>. If <cutPath> is not found, <newRoot> is not prepended. This allows having a local copy
              of the source file sub-tree. If <cutPath> is not part of the file location, the original source path is used.
-src:         Enable display of source lines in output if available (on by default).
-nosrc:       Disable display of source lines in output.
-file:        Display source file information in output (on by default).
-nofile:      Do not display source file information.
-dasm:        Display disassembled code in output (on by default).
-nodasm:      Do not display disassembled code in output.
-func:        Display function name with source information (off by default).
-nofunc:      Do not display function information with source information.
-trace:       Display trace information in output (off by default).
-notrace:     Do not display trace information in output.
--strip=path: Strip of the specified path when displaying source file name/path. Strips off all that matches.
              Path may be enclosed in quotes if it contains spaces.
-itcprint:    Display ITC 0 data as a null terminated string. Data from consecutive ITC 0's will be concatenated
              and displayed as a string until a terminating \0 is found. Also enables processing and display of
              no-load-strings.
-itcprint=n:  Display ITC channel n data as a null terminated string. Data for consecutive ITC channel n's will be
              concatenated and display as a string until a terminating \n or \0 is found. Also enabled processing
              and display of no-load-strings
-noitcprint:  Display ITC 0 data as a normal ITC message; address, data pair
-nls:         Enables processing of no-load-strings
-nonls:       Disable processing of no-load-strings.
-addrsize=n:  Display address as n bits (32 <= n <= 64). Values larger than n bits will print, but take more space and
              cause the address field to be jagged. Overrides value address size read from elf file.
-addrsize=n+: Display address as n bits (32 <= n <= 64) unless a larger address size is seen, in which case the address
              size is increased to accommodate the larger value. When the address size is increased, it stays increased
              (sticky) and will be again increased if a new larger value is encountered. Overrides the address size
              read from the elf file.
-32:          Display addresses as 32 bits. Values lager than 32 bits will print, but take more space and cause
              the address field to be jagged. Selected by default if elf file indicates 32 bit address size.
              Specifying -32 overrides address size read from elf file
-32+          Display addresses as 32 bits until larger addresses are displayed and then adjust up to a larger
              enough size to display the entire address. When addresses are adjusted up, they do not later adjust
              back down, but stay at the new size unless they need to adjust up again. This is the default setting
              if the elf file specifies > 32 bit address size (such as 64). Specifying -32+ overrides the value
              read from the elf file
-64:          Display addresses as 64 bits. Overrides value read from elf file
-archsize=nn: Set the architecture size to 32 or 64 bits instead of getting it from the elf file
-addrsep:     For addresses greater than 32 bits, display the upper bits separated from the lower 32 bits by a '-'
-noaddrsep:   Do not add a separator for addresses greater than 32 bit between the upper bits and the lower 32 bits
              (default).
-srcbits=n:   The size in bits of the src field in the trace messages. n must 0 to 16. Setting srcbits to 0 disables
              multi-core. n > 0 enables multi-core. If the -srcbits=n switch is not used, srcbits is 0 by default.
-labels:      Treat labels as functions for source information and disassembly. On by default.
-nolables:    Do not use local labels as function names when returning source information or instruction location information.
-analytics:   Compute and display detail level 1 trace analytics.
-analytics=n: Specify the detail level for trace analytics display. N sets the level to either 0 (no analytics display)
              1 (sort system totals), or 2 (display analytics by core).
-noanaylitics: Do not compute and display trace analytics (default). Same as -analytics=0.
-freq nn:     Specify the frequency in Hz for the timestamp tics clock. If specified, time instead
              of tics will be displayed.
-tssize=n:    Specify size in bits of timestamp counter; used for timestamp wrap
-callreturn:  Annotate calls, returns, and exceptions
-nocallreturn Do not annotate calls, returns, exceptions (default)
-branches:    Annotate conditional branches with taken or not taken information
-nobrnaches:  Do not annotate conditional branches with taken or not taken information (default)
-pathunix:    Show all file paths using unix-type '/' path separators (default)
              Also cleans up path, removing // -> /, /./ -> /, and uplevels for each /../
-pathwindows: Show all file paths using windows-type '\' path separators
              Also cleans up path, removing // -> /, /./ -> /, and uplevels for each /../
-pathraw:     Show all file path in the format stored in the elf file
-msglevel=n:  Set the Nexus trace message detail level. n must be >= 0, <= 3
-r addr:      Display the label information for the address specified for the elf file specified
-debug:       Display some debug information for the trace to aid in debugging the trace decoder
-nodebug:     Do not display any debug information for the trace decoder
-v:           Display the version number of the DQer and exit.
-h:           Display this usage information.
```

Besides using the trace-decoder from inside Freedom Studio, the most common way to use the program is:

`> dqr -t sort.rtd -e sort.elf -trace`

This will generate an output that looks something like:

```
Trace: Msg # 1, NxtAddr: 404002e2, TCode: SYNC (9) Reason: (3) Exit Debug I-CNT: 0 F-Addr: 0x20200171

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:25
Source:   sort(data,sizeof data / sizeof data[0]);
    404002e2:         fc840793           addi	a5,s0,-56

Trace: Msg # 2, NxtAddr: 404002e6, TCode: CORRELATION (33) EVCODE: 0 I-CNT: 2

Trace: Msg # 3, NxtAddr: 404002e6, TCode: SYNC (9) Reason: (3) Exit Debug I-CNT: 0 F-Addr: 0x20200173
    404002e6:         45a1               li	a1,8
    404002e8:         853e               mv	a0,a5
    404002ea:         3d59               jal	40400180 <sort>

Trace: Msg # 4, NxtAddr: 40400180, TCode: DIRECT BRANCH (3) I-CNT: 3

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:4
Source: {
    40400180:         7179               addi	sp,sp,-48
    40400182:         d622               sw	s0,44(sp)
    40400184:         1800               addi	s0,sp,48
    40400186:         fca42e23           sw	a0,-36(s0)
    4040018a:         fcb42c23           sw	a1,-40(s0)

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    4040018e:         4785               li	a5,1
    40400190:         fef42623           sw	a5,-20(s0)

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    40400194:         a045               j	40400234 <sort+b4>

Trace: Msg # 5, NxtAddr: 40400234, TCode: DIRECT BRANCH (3) I-CNT: 11

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    40400234:         fec42703           lw	a4,-20(s0)
    40400238:         fd842783           lw	a5,-40(s0)
    4040023c:         f4f74de3           blt	a4,a5,40400196 <sort+16>

...

```

Except it will be much longer.

If the files specified by the -t and -e options are not in the current directory, the path to each should be included. If the trace was collected with timestamps, they will be displayed in the trace information if the -trace option is given.

All trace output is sent to stdout.

### Using a Properties File With the Trace Decoder

The command line dqr program can use a properties file if the `-pf <filename>` option is given on the command line. Not all options can be specified in the properties file, and some options, such as the CTF (Common Trace Format) and textual event file creation can only be specified in the properties file. The properties file is a text based file of the general form:

`tag = value`

Here is a list of the properties currently processed. The tag field is not case sensitive. Tags other than what is listed below are silently ignored.

```
rtd = <rtd file name. Can have abs or rel path>
elf = <elf file name. Can have abs or rel path>
srcbits = <number of src bits in trace messages. 2^srcbits = # cores. 0 is the default if srcbits is not present>
bits = <number of address bits. Normally 32 or 64. Default is 32 if not present>
trace.config.boolean.enable.itc.print.processing = <true | false. True enables normal ITC prints and nls prints. False enables nls prints. Default is false>
trace.config.int.itc.print.channel = <n, where n is the itc channel number for itc prints. 0 is the default>
trace.config.int.itc.print.buffersize = <n, where n is the number of bytes in the itc print buffer. 4096 is the default>
source.root = <path to prepend to the path for source file lookup. Null by default>
source.cutpath = <path to remove from beginning of source file lookup before adding source.root. Null by default>
cafile = <CA trace file name. Can have abs or rel path>
catype = <none | vector | instruction. Type of ca trace file. None by default>
tssize = <n, where n is the number of bits in the timestamp counter. 40 by default>
pathtype = <unix | windows | raw. How source paths should be displayed. unix will use '/' for folder separators, windows will use '\', raw will use what is in the elf file for paths. Default is raw>
labelsAsFunctions = <true | false. specifies if labels in the elf symbol table are considered functions or not. Default is true>
freq = <n, where n is the timestamp frequency in Hz. A value of 0 means unknown. 0 is the default>
ctfenable = <true | false. A value of true enables CTF file generation. Default is false>
eventconversionenable = <true | false. A value of true enables generation of text CSV files with event information. Default is false>
addressdisplayflags = <width of addresses to display in bits. If width has a '+' appended to the end, the address displayed will grow past what is specified if needed. Default is 32+>
starttime = <n, where n is the number of nanoseconds after the Unix Epoch. n == -1 (the default value if not specified) will use the system time as the start time for the trace. Starttime is used in the CTF conversion metadata file.>
hostname = <host name. This will override the system host name and use the name provided as the host in the CTF conversion metadata file.>
```

### CTF and Textual Event File Creation

The trace decoder will translate a SiFive Nexus trace file into a CTF (Common Trace Format) file and textual event files. The CTF file can be processed by Trace Compass for viewing Flame Chart and Flame Graph data.

If the property `ctfenable = true` is given in the properties file, a ctf folder will be created in the same folder as the Nexus trace file (usually named trace.rtd). CTF files will be written to the ctf folder, one per core plus a metadata file.

If the `eventconversionenable = true` is contained in the properties file, an events folder will be created in the same folder as the Nexus trace file. Text based files will be written to the events folder, one per event type, and one line per event. the name extension identifies the type of events in each file. Also, an events file will be created in the same folder as the Nexus trace file that contains all events. The all events text files with have the extension ".events". An event trace must be processed to get output in the event files. The first column of each event file is the processor ID (core number), the second column is the timestamp. If timestamps are not collected for the trace, all timestamp values will be 0. The columns after the timestamp are dependent on the type of events for the file. All events will have the address that created the event (identified by the "PC=" label). Transfer instructions (jumps, returns) will also have the destination address of the instruction (identified by the 'PCDest=' label).

The first line of each textual event file will have the name of the elf file for the trace. Also, each event line in the event files will have the file and line specified where in the source code the event occured (if available).

Not that CTF and Event conversion can only be enabled in the properties file, and not by using command line switches.

### ITC Print
add no-load string info

There are two mechanisms available to capture printed text in the trace message stream; redirected stdio and a custom itcprintf() and itcputs() functions. Both cause writes to the ITC 0 stimulus register which generates either Auxiliary Access Write trace message or Data Acquisition trace messages messages to be generated. Using the trace-decoder, the printed output can be displayed. Additional information is given in doc/ITCPrint.md.

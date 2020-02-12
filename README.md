# trace-decoder
SiFive implementation of the Nexus trace decoder

The trace-decoder is a stand-alone program that reads Nexus 2001 traces for the RISC-V architecture as implemented by SiFive and reconstructs the instruction stream that generated the trace. Instruction traces can be used for debugging and performance analysis. Currently, Windows, Linux, and OS X are supported.

The process to create/capture a trace file is not covered here; only how to build and use the trace-decoder tool to reconstruct the program execution that generated the trace. Usage of the trace-decoder assumes you already have or know how to create trace files.

This document describes the Alpha version of the SiFive trace-decoder, both how to build and how to use.

Currently the trace-decoder is at an Alpha level. The name of the trace-decoder is `dqr` (for de-queuer)

### Licensing:

The trace-decoder is released under the GPL 3 License. For a full copy of the license, see the COPYING file included with the trace-decoder. The licensing statement is:

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

In addition, the trace-decoder is statically linked with various licensed libraries. The libraries are listed below, along with their licensing:

	libbfd       GNU GPL V3
	libopcodes   GNU GPL V3
	libiberty    GNU LGPL V2.1
	linintl      GNU LGPL V2.1
	libz         Zlib/libpng License

The GNU GPL V3 license is in the file COPYING; the GNU LGPL V2.1 license is in the file COPYING.LIB; the zlib/libpng License is in the file zlib source file zlib.h, and does not require distribution with the library binary.

These libraries, along with much more, have been ported to RISC-V by SiFive, and are available at the link:

https://github.com/sifive/riscv-binutils-gdb

The binaries for these libraries for all supported platforms are included in this distribution in the lib folders.

### New for Version 0.3 (19.08 release)

Support for 64 bit trace decoding. Command line options can specify the address width to display. By default the type of elf file is used to determine address width.

Printing to the ITC Stimulus Registers. Instead of sending printed output to a UART, printed output can be sent to the ITC stimulus registers and captured in thre trace message stream. The trace-decoder (dqr) will can reconstruct the printed output and display it to the user. See doc/ITCPrint.md for more information.

Multi-core Support. The trace-decoder now supports multi-core traces, up to 8 cores through the use of the -srcbits-n switch. When decoding multi-core traces, each line output is prefixed with a core number for identification.

### Building:

To build the trace-decoder, the correct build tools must first be installed. On windows, MinGW and MSYS must be installed.  For Linux, g++ must be installed. On Mac OS X, use either XCode or g++.

#### Prerequisites:

The trace-decoder compiles as a 64-bit application.

Below are the required tools that need to be installed for each of the supported platforms:

Windows:

To build under windows, you will need to install both MinGW-W64 and MSYS.

MinGW-w64 (with gcc 8.1.0:

A working MinGW-w64 installer can be downloaded from [sourceforge](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe). 

If you are planning on using eclipse, install MinGW-w64 in a folder with no spaces in the path (for example, do not install it in Program Files). Chose the x86-64 architecture when installing.

You will also need to edit your PATH environment variable to include MinGW-w64. If for example, you installed MinGw-w64 in c:\mingw, you should add c:\mingw\bin to your path.

MSYS:

The MSYS installer can be downloaded from http://downloads.sourceforge.net/mingw/MSYS-1.0.11.exe. After downloading, run the installer and follow the instructions.

Linux:

To build on Linux, you will need to install g++ and the GNU binutils. This process varies for different Linux distributions. If you do not know how to do this, a quick web search for your distribution should provide a solution. Make sure you install the compilers that produce 64-bit code and not 32-bit code.

OS X:

You can use either XCode or the GNU g++/gcc and binutils tools. XCode is probably the easiest. If you do not already have XCode installed on your system, you can go to the Apple app store to download.

XCode or g++ (note: XCode uses the gcc/g++ front end, but uses the LLVM compiler chain. This means that XCode is not using the g++/gcc compilers). Either toolchain should work fine for compiling the trace-decoder, but only the XCode tool chain has been verified to work.

### Directory Structure of Source:

The directory structure for the dqr is:

	trace-decoder\
		include\
		src\
		lib\
			linux\
			mcaos\
			windows\
		Debug\
		Release\

All source files are in src or include. The file main.cpp contains the driver program for the trace-decoder, but also can be used as a guide to building your own front-end which uses the Trace class for creating instruction streams from the collected trace information.

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
 
When you do it this way you will be working on the files directly in the local git repository.  These files do not actually live in your workspace.  You can still easily interact with your repo from a git CLI.
  
The Eclipse project includes "build targets" to make for easy building in Eclipse.

#### Using the CLI

The makefile includes two configurations:  `Debug` and `Release` (they are capitalized).

The default configuration is `Debug`, so if you `make clean` or `make all` or `make clean all` you will act on the `Debug` configuration and all build artifacts will be in the `Debug` folder.

To build the `Release` configuration, use `make CONFIG=Release all` (This is what Jenkins will use to build a release)

On the Windows and Linux versions, all libraries are statically linked in the executable. On OS X, only the libbfd, libopcodes, libibery, libintl, and libz libraries are statically linked.

#### Notes

There is a `makefile.init` at the top level that contains the settings that apply to all configurations, including a list of OBJ files that needs to be updated when new source files are added. Each config sub-folder has a makefile that contains settings for the specific configuration (like optimization flags that generally differentiate a Release build from a Debug build).

### Running:

Use `dqr -h` to display usage information. It will display something like:

```
Usage: dqr -t tracefile -e elffile | -n basename) [-start mn] [-stop mn] [-src] [-nosrc]
           [-file] [-nofile] [-dasm] [-nodasm] [-trace] [-notrace] [--strip=path] [-v] [-h]
           [-multicore] [-nomulticore] [-unicore] [-32] [-64] [-32+] [-addrsep] [-noaddrsep]

-t tracefile: Specify the name of the Nexus trace message file. Must contain the file extension (such as .rtd).
-e elffile:   Specify the name of the executable elf file. Must contain the file extension (such as .elf).
-n basename:  Specify the base name of the Nexus trace message file and the executable elf file. No extension
              should be given. The extensions .rtd and .elf will be added to basename.
-start nm:    Select the Nexus trace message number to begin DQing at. The first message is 1. If -stop is
              not specified, continues to last trace message.
-stop nm:     Select the last Nexus trace message number to end DQing at. If -start is not specified, starts
              at trace message 1.
-src:         Enable display of source lines in output if available (on by default).
-nosrc:       Disable display of source lines in output.
-file:        Display source file information in output (on by default).
-nofile:      Do not display source file information.
-dasm:        Display disassembled code in output (on by default).
-nodasm:      Do not display disassembled code in output.
-trace:       Display trace information in output (off by default).
-notrace:     Do not display trace information in output.
--strip=path: Strip of the specified path when displaying source file name/path. Strips off all that matches.
              Path may be enclosed in quotes if it contains spaces.
-itcprint:    Display ITC 0 data as a null terminated string. Data from consecutive ITC 0's will be concatenated
              and displayed as a string until a terminating \0 is found
-itcprintnobuffer:    Display ITC 0 data as a null terminated string without buffering multiple itc messages into a
              single message. Data from consecutive ITC 0's will not be concatenated. Potentially useful where the
              buffering of itc 0 print messages causes flushing issues.
-noitcprint:  Display ITC 0 data as a normal ITC message; address, data pair
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
-addrsep:     For addresses greater than 32 bits, display the upper bits separated from the lower 32 bits by a '-'
-noaddrsep:   Do not add a separator for addresses greater than 32 bit between the upper bits and the lower 32 bits
              (default).
-srcbits=n:   The size in bits of the src field in the trace messages. n must 0 to 8. Setting srcbits to 0 disables
              multi-core. n > 0 enables multi-core. If the -srcbits=n switch is not used, srcbits is 0 by default.
-v:           Display the version number of the DQer and exit.
-h:           Display this usage information.
```

Besides using the trace-decoder from inside Freedom Studio, the most common way to use the program is:

`> dqr -t sort.rtf -e sort.elf -trace`

This will generate an output that looks something like:

```
Trace: Msg # 1, NxtAddr: 404002e2, TCode: SYNC (9) Reason: (3) Exit Debug I-CNT: 0 F-Addr: 0x20200171

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:25
Source:   sort(data,sizeof data / sizeof data[0]);
    404002e2:         fc840793           addi	a5,s0,-56

Trace: Msg # 2, NxtAddr: 404002e6, TCode: CORRELATION (33) EVCODE: 0 I-CNT: 2

Trace: Msg # 3, NxtAddr: 404002e6, TCode: SYNC (9) Reason: (3) Exit Debug I-CNT: 0 F-Addr: 0x20200173
    404002e6:         45a1               li	a1,8
    404002e8:         853e               mv	a0,a5
    404002ea:         3d59               jal	40400180 <sort>

Trace: Msg # 4, NxtAddr: 40400180, TCode: DIRECT BRANCH (3) I-CNT: 3

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:4
Source: {
    40400180:         7179               addi	sp,sp,-48
    40400182:         d622               sw	s0,44(sp)
    40400184:         1800               addi	s0,sp,48
    40400186:         fca42e23           sw	a0,-36(s0)
    4040018a:         fcb42c23           sw	a1,-40(s0)

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    4040018e:         4785               li	a5,1
    40400190:         fef42623           sw	a5,-20(s0)

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    40400194:         a045               j	40400234 <sort+b4>

Trace: Msg # 5, NxtAddr: 40400234, TCode: DIRECT BRANCH (3) I-CNT: 11

File: C:\workspaces\wsFreedomStudio\sort\src/sort.c:5
Source:   for (int i = 1; i < size; i++) {
    40400234:         fec42703           lw	a4,-20(s0)
    40400238:         fd842783           lw	a5,-40(s0)
    4040023c:         f4f74de3           blt	a4,a5,40400196 <sort+16>

...

```

Except it will be much longer.

If the files specified by the -t and -e options are not in the current directory, the path to each should be included. If the trace was collected with timestamps, they will be displayed in the trace information if the -trace option is given.

All trace output is sent to stdout.

### ITC Print

There are two mechanisms available to capture printed text in the trace message stream; redirected stdio and a custom itcprintf() and itcputs() functions. Both cause writes to the ITC 0 stimulus register which generates either Auxiliary Access Write trace message or Data Acquisition trace messages messages to be generated. Using the trace-decoder, the printed output can be displayed. Additional information is given in doc/ITCPrint.md.


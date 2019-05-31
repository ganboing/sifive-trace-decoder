# trace-decoder
SiFive implementation of the Nexus Trace decoder


Here you will find a very early version of the DQr. This document explains how to build it and how to use it.

Right now, almost everything is in main.cpp, just because it seemed esier for development. It probably was intially, but has grown
pretty large and would be esier now if it was broken up into files by class. After re-organizing stuff into separate files, main.cpp will just be
a driver program for the command line version of the DQr, and serve as an example of how to use it.

Source file display is not yet working, but at least I have figured out how to get the filename, function name, and src file line number given an address.
That is what I am currently working on. As soon as that works, I am going to tackle the to do list at the top of main.cpp.

### Building:
The trace-decoder
 can be built on the command line or within Eclipse.

#### Using Eclipse

My preffered workflow for getting the project into Eclipse:

 1. Clone the repo to your normal git repository (typically <home>/git) using whatever method you like
 2. Within Eclipse, import this repository/project directly into Eclipse using `File -> Import -> Git -> Projects from Git`
 3. Select "Existing local repository"
 4. Add the repository if not already added (you will need to do this only once)
 5. Search the repository for Eclipse projects (there should only be one in the result list)
 6. Check the 'trace-decoder' project (if not already checked)
 7. Finish the wizard to create the new project in your worksapce.
 
When you do it this way you will be working on the files directly in the local git repository.  These files do not actually live in your workspace.  You can still easily interact with your repo from a git CLI.
  
The Eclipse project include "build targets" to make for easy building in Eclipse.

#### Using the CLI

The makefile includes two configurations:  `Debug` and `Release` (they are capitalized).

The default configuration is `Debug`, so if you `make clean` or `make all` or `make clean all` you will act on the `Debug` configuration and all build artifacts will be in the `Debug` folder.

To build the `Release` configuration, use `make CONFIG=Release all` (This is what Jenkins will use to build a release)

#### Notes

There is a `makefile.init` at the top level that contains the settings that apply to all configurations, including a list of OBJ files that needs to be updated when new source files are added. Each config sub-folder has a makefile that contains settings for the specific configuration (like optimization flags that generally differentiate a Release build from a Debug build).


### Running:

Use `dqr -h` to display useage information. It will dipslay something like:

Usage: `dqr [-t tracefile -e elffile] | [-n basename] [ -start mn ] [-stop mn] [-v] [-h]`

```
Useage: dqr (-t tracefile -e elffile | -n basename) [-start mn] [-stop mn] [-v]
-t tracefile: Specify the name of the Nexus trace message file. Must contain the
-e elffile:   Specify the name of the executable elf file. Must contain the file
-n basename:  Specify the base name of hte Nexus trace message file and the exec
              should be given.
              The extensions .rdt and .elf will be added to basename.
-start nm:    Select the Nexus trace message number to begin DQing at. The first
              not specified, continues to last trace message.
-stop nm:     Select the last Nexus trace message number to end DQing at. If -st
              at trace message 1.
-src:         Enable display of source lines in output if available (on by defau
-nosrc:       Disable display of source lines in output.
-file:        Display source file information in output (on by default).
-nofile:      Do not dipslay source file information.
-dasm:        Display disassembled code in output (on by default).
-nodasm:      Do not display disassembled code in output.
-trace:       Display trace information in output (off by default).
-notrace:     Do not display trace information in output.
--strip=path: Strip of the specified path when displaying source file name/path.
              Path may be enclosed in quotes if it contains spaces.
-v:           Display the version number of the DQer and exit
-h:           Display this useage information.
```

In general, the most common way to use the program are:

`dqr -t trace.rtf -e hello.elf`

This will generate an output that looks something like:

```
  # Trace Message(1): Sync, SYNCREASON=3, ICNT=0, FADDR=0x202000bc, Target=0x40400178

40400178 <main+8>:
40400178:        4521                    li	a0,8  # Trace Message(2): Correlation, EVCODE=0, CDF=0, ICNT=1
  # Trace Message(3): Sync, SYNCREASON=3, ICNT=0, FADDR=0x202000bd, Target=0x4040017a

4040017a:        376d                    jal	40400124 <fib>  # Trace Message(4): Direct Branch, ICNT=1, Target=0x40400124

40400124 <fib>:
40400124:        1101                    addi	sp,sp,-32
40400126:        ce06                    sw	ra,28(sp)
40400128:        cc22                    sw	s0,24(sp)
4040012a:        ca26                    sw	s1,20(sp)
4040012c:        1000                    addi	s0,sp,32
4040012e:        fea42623                sw	a0,-20(s0)
40400132:        fec42783                lw	a5,-20(s0)
40400136:        e399                    bnez	a5,4040013c <fib+18>  # Trace Message(5): Direct Branch, ICNT=10, Target=0x4040013c

4040013c:        fec42703                lw	a4,-20(s0)
40400140:        4785                    li	a5,1
40400142:        00f71463                bne	a4,a5,4040014a <fib+26>  # Trace Message(6): Direct Branch, ICNT=5, Target=0x4040014a
```

Except it will be much longer.

The trace output is sent to stdout.

If you have any questions or problems, please let me know!
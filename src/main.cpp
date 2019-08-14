/*
 * Copyright 2019 Sifive, Inc.
 *
 * main.cpp
 */

/*
   This file is part of dqr, the SiFive Inc. Risc-V Nexus 2001 trace decoder.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "dqr.hpp"

using namespace std;

static void usage(char *name)
{
	printf("Usage: dqr (-t tracefile -e elffile | -n basename) [-start mn] [-stop mn] [-src] [-nosrc]\n");
	printf("            [-file] [-nofile] [-dasm] [-nodasm] [-trace] [-notrace] [--strip=path] [-v] [-h]\n");
	printf("\n");
	printf("-t tracefile: Specify the name of the Nexus trace message file. Must contain the file extension (such as .rtd).\n");
	printf("-e elffile:   Specify the name of the executable elf file. Must contain the file extention (such as .elf).\n");
	printf("-n basename:  Specify the base name of hte Nexus trace message file and the executable elf file. No extension\n");
	printf("              should be given. The extensions .rtd and .elf will be added to basename.\n");
	printf("-start nm:    Select the Nexus trace message number to begin DQing at. The first message is 1. If -stop is\n");
	printf("              not specified, continues to last trace message.\n");
	printf("-stop nm:     Select the last Nexus trace message number to end DQing at. If -start is not specified, starts\n");
	printf("              at trace message 1.\n");
	printf("-src:         Enable display of source lines in output if available (on by default).\n");
	printf("-nosrc:       Disable display of source lines in output.\n");
	printf("-file:        Display source file information in output (on by default).\n");
	printf("-nofile:      Do not display source file information.\n");
	printf("-dasm:        Display disassembled code in output (on by default).\n");
	printf("-nodasm:      Do not display disassembled code in output.\n");
	printf("-trace:       Display trace information in output (off by default).\n");
	printf("-notrace:     Do not display trace information in output.\n");
	printf("--strip=path: Strip of the specified path when displaying source file name/path. Strips off all that matches.\n");
	printf("              Path may be enclosed in quotes if it contains spaces.\n");
	printf("-itcprint:    Display ITC 0 data as a null terminated string. Data from consecutive ITC 0's will be concatinated\n");
	printf("              and displayed as a string until a terminating \\0 is found\n");
	printf("-noitcprint:  Display ITC 0 data as a normal ITC message; address, data pair\n");
	printf("-addrsize=n:  Display address as n bits (32 <= n <= 64). Values larger than n bits will print, but take more space and\n");
	printf("              cause the address field to be jagged. Overrides value address size read from elf file.\n");
	printf("-addrsize=n+: Display address as n bits (32 <= n <= 64) unless a larger address size is seen, in which case the address\n");
	printf("              size is increased to accommodate the larger value. When the address size is increased, it stays increased\n");
	printf("              (sticky) and will be again increased if a new larger value is encountered. Overrides the address size\n");
	printf("              read from the elf file.\n");
	printf("-32:          Display addresses as 32 bits. Values lager than 32 bits will print, but take more space and cause\n");
    printf("              the address field to be jagged. Selected by default if elf file indicates 32 bit address size.\n");
	printf("              Specifying -32 overrides address size read from elf file\n");
	printf("-32+          Display addresses as 32 bits until larger addresses are displayed and then adjust up to a larger\n");
	printf("              enough size to display the entire address. When addresses are adjusted up, they do not later adjust\n");
	printf("              back down, but stay at the new size unless they need to adjust up again. This is the default setting\n");
	printf("              if the elf file specifies > 32 bit address size (such as 64). Specifying -32+ overrides the value\n");
	printf("              read from the elf file\n");
	printf("-64:          Display addresses as 64 bits. Overrides value read from elf file\n");
	printf("-addrsep:     For addresses greater than 32 bits, display the upper bits separated from the lower 32 bits by a '-'");
	printf("-noaddrsep:   Do not add a separatfor For addresses greater than 32 bit between the upper bits and the lower 32 bits\n");
	printf("              (default)\n");
	printf("-v:           Display the version number of the DQer and exit.\n");
	printf("-h:           Display this usage information.\n");
}

static const char *stripPath(const char *prefix,const char *srcpath)
{
	if (prefix == nullptr) {
		return srcpath;
	}

	if (srcpath == nullptr) {
		return nullptr;
	}

	const char *s = srcpath;

	for (;;) {
		if (*prefix == 0) {
			return s;
		}

		if (tolower(*prefix) == tolower(*s)) {
			prefix += 1;
			s += 1;
		}
		else if (*prefix == '/') {
			if (*s != '\\') {
				return srcpath;
			}
			prefix += 1;
			s += 1;
		}
		else if (*prefix == '\\') {
			if (*s != '/') {
				return srcpath;
			}
			prefix += 1;
			s += 1;
		}
		else {
			return srcpath;
		}
	}

	return nullptr;
}

int main(int argc, char *argv[])
{
	bool binary_flag = true;
	char *tf_name = nullptr;
	char *base_name = nullptr;
	char *ef_name = nullptr;
	char buff[128];
	int buff_index = 0;
	bool usage_flag = false;
	bool version_flag = false;
	int start_msg_num = 0;
	int stop_msg_num = 0;
	bool src_flag = true;
	bool file_flag = true;
	bool dasm_flag = true;
	bool trace_flag = false;
	bool func_flag = false;
	char *strip_flag = nullptr;
	bool itcprint_flag = false;
	int  numAddrBits = 0;
	uint32_t addrDispFlags = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp("-t",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -t requires a file name\n");
				usage(argv[0]);
				return 1;
			}

			base_name = nullptr;

			tf_name = argv[i];
		}
		else if (strcmp("-n",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -n requires a file name\n");
				usage(argv[0]);
				return 1;
			}

			ef_name = nullptr;
			tf_name = nullptr;

			base_name = argv[i];
		}
		else if (strcmp("-e",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -e requires a file name\n");
				usage(argv[0]);
				return 1;
			}

			base_name = nullptr;

			ef_name = argv[i];
		}
		else if (strcmp("-binary",argv[i]) == 0) {
			binary_flag = true;
		}
		else if (strcmp("-text",argv[i]) == 0) {
			binary_flag = false;
		}
		else if (strcmp("-start",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -start requires a trace message number\n");
				usage(argv[0]);
				return 1;
			}

			start_msg_num = atoi(argv[i]);
			if (start_msg_num <= 0) {
				printf("Error: starting message number must be >= 1\n");
				return 1;
			}
		}
		else if (strcmp("-stop",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -stop requires a trace message number\n");
				usage(argv[0]);
				return 1;
			}

			stop_msg_num = atoi(argv[i]);
			if (stop_msg_num <= 0) {
				printf("Error: stopping message number must be >= 1\n");
				return 1;
			}
		}
		else if (strcmp("-src",argv[i]) == 0) {
			src_flag = true;
		}
		else if (strcmp("-nosrc",argv[i]) == 0) {
			src_flag = false;
		}
		else if (strcmp("-file",argv[i]) == 0) {
			file_flag = true;
		}
		else if (strcmp("-nofile",argv[i]) == 0) {
			file_flag = false;
		}
		else if (strcmp("-dasm",argv[i]) == 0) {
			dasm_flag = true;
		}
		else if (strcmp("-nodasm",argv[i]) == 0) {
			dasm_flag = false;
		}
		else if (strcmp("-trace",argv[i]) == 0) {
			trace_flag = true;
		}
		else if (strcmp("-notrace",argv[i]) == 0) {
			trace_flag = false;
		}
		else if (strcmp("-func",argv[i]) == 0) {
			func_flag = true;
		}
		else if (strcmp("-nofunc",argv[i]) == 0) {
			func_flag = false;
		}
		else if (strncmp("--strip=",argv[i],strlen("--strip=")) == 0) {
			strip_flag = argv[i]+strlen("--strip=");

			if (strip_flag[0] == '"') {
				strip_flag += 1;

				if (strip_flag[strlen(strip_flag)] == '"') {
					strip_flag[strlen(strip_flag)] = 0;
				}
			}
		}
		else if (strcmp("-v",argv[i]) == 0) {
			version_flag = true;
		}
		else if (strcmp("-h",argv[i]) == 0) {
			usage_flag = true;
		}
		else if (strcmp("-itcprint",argv[i]) == 0) {
			itcprint_flag = true;
		}
		else if (strcmp("-noitcprint",argv[i]) == 0) {
			itcprint_flag = false;
		}
		else if (strcmp("-32",argv[i]) == 0) {
			numAddrBits = 32;
			addrDispFlags = addrDispFlags & ~dqr::ADDRDISP_WIDTHAUTO;
		}
		else if (strcmp("-64",argv[i]) == 0) {
			numAddrBits = 64;
			addrDispFlags = addrDispFlags & ~dqr::ADDRDISP_WIDTHAUTO;
		}
		else if (strcmp("-32+",argv[i]) == 0) {
			numAddrBits = 32;
			addrDispFlags = addrDispFlags | dqr::ADDRDISP_WIDTHAUTO;
		}
		else if (strncmp("-addrsize=",argv[i],strlen("-addrsize=")) == 0) {
			int l;
			char *endptr;

			l = strtol(&argv[i][strlen("-addrsize=")], &endptr, 10);

			if (endptr[0] == 0 ) {
				numAddrBits = l;
				addrDispFlags = addrDispFlags  & ~dqr::ADDRDISP_WIDTHAUTO;
			}
			else if (endptr[0] == '+') {
				numAddrBits = l;
				addrDispFlags = addrDispFlags | dqr::ADDRDISP_WIDTHAUTO;
			}
			else {
				printf("Error: option -addressize= requires a valid number <= 32, >= 64\n");
				usage(argv[0]);
				return 1;
			}

			if ((l < 32) || (l > 64)) {
				printf("Error: option -addressize= requires a valid number <= 32, >= 64\n");
				usage(argv[0]);
				return 1;
			}
		}
		else if (strcmp("-addrsep", argv[i]) == 0) {
			addrDispFlags = addrDispFlags | dqr::ADDRDISP_SEP;
		}
		else if (strcmp("-noaddrsep", argv[i]) == 0) {
			addrDispFlags = addrDispFlags & ~dqr::ADDRDISP_SEP;
		}
		else {
			printf("Unkown option '%s'\n",argv[i]);
			usage_flag = true;
		}
	}

	if (usage_flag) {
		usage(argv[0]);
		return 0;
	}

	if (version_flag) {
		printf("%s: version %s\n",argv[0],"0.2");
		return 0;
	}

	if (tf_name == nullptr) {
		if (base_name == nullptr) {
			printf("Error: must specify either a base name or a trace file name\n");
			usage(argv[0]);
			return 1;
		}

		tf_name = &buff[buff_index];
		strcpy(tf_name,base_name);
		strcat(tf_name,".rtd");
		buff_index += strlen(tf_name) + 1;
	}

	if (ef_name == nullptr) {
		if (base_name != nullptr) {
			ef_name = &buff[buff_index];
			strcpy(ef_name,base_name);
			strcat(ef_name,".elf");
			buff_index += strlen(ef_name) + 1;
		}
	}

	Trace::SymFlags symFlags = Trace::SYMFLAGS_NONE;

	// might want to include some path info!

	Trace *trace = new (std::nothrow) Trace(tf_name,binary_flag,ef_name,symFlags,numAddrBits,addrDispFlags);

	assert(trace != nullptr);

	if (trace->getStatus() != dqr::DQERR_OK) {
		delete trace;
		trace = nullptr;

		printf("Error: new Trace(%s,%s) failed\n",tf_name,ef_name);

		return 1;
	}

	trace->setTraceRange(start_msg_num,stop_msg_num);

	dqr::DQErr ec;

	// main loop

//	this shouldn't be next instruction. it should be next because we may not be generating instruction'
//	we may just be dumping raw traces, or we may be dumping traces with addresses or we may be doing
//	dissasembled instruction traces or we may be adding source code
//
//	flags:
//
//		raw trace messages
//		decoded trace message
//		include trace addresses
//		include disassembly
//		include source

	// we want to be able to select the level of output, from minimal (raw trace only) to full (everything)

//	do we select when we create the trace object?
//	does it always generate as much as possible and we jsut print what we want?
//
//	should not print, but should return a string or way to make a string!
//
//	should look at source code display!


	Instruction *instInfo;
	NexusMessage *msgInfo;
	Source *srcInfo;
	char dst[80];
	int instlevel = 1;
	int msgLevel = 2;
	const char *lastSrcFile = nullptr;
	const char *lastSrcLine = nullptr;
	unsigned int lastSrcLineNum = 0;
	dqr::ADDRESS lastAddress = 0;
	int lastInstSize = 0;
	bool firstPrint = true;

	do {
		ec = trace->NextInstruction(&instInfo,&msgInfo,&srcInfo);
		if (ec == dqr::DQERR_OK) {
			if (srcInfo != nullptr) {
				if ((lastSrcFile != srcInfo->sourceFile) || (lastSrcLine != srcInfo->sourceLine) || (lastSrcLineNum != srcInfo->sourceLineNum)) {
					lastSrcFile = srcInfo->sourceFile;
					lastSrcLine = srcInfo->sourceLine;
					lastSrcLineNum = srcInfo->sourceLineNum;

					if (file_flag) {
						if (srcInfo->sourceFile != nullptr) {
							if (firstPrint == false) {
								printf("\n");
							}

							const char *sfp;

							sfp = stripPath(strip_flag,srcInfo->sourceFile);

							printf("File: %s:%d\n",sfp,srcInfo->sourceLineNum);

							firstPrint = false;
						}
					}

					if (src_flag) {
						if (srcInfo->sourceLine != nullptr) {
							printf("Source: %s\n",srcInfo->sourceLine);

							firstPrint = false;
						}
					}
				}
			}

			if (dasm_flag && (instInfo != nullptr)) {
//			    instInfo->addressToText(dst,instlevel);
				instInfo->addressToText(dst,0);

				if (func_flag) {
					if (instInfo->address != (lastAddress + lastInstSize / 8)) {
						if (instInfo->addressLabel != nullptr) {
							printf("<%s",instInfo->addressLabel);
							if (instInfo->addressLabelOffset != 0) {
								printf("+%x",instInfo->addressLabelOffset);
							}
							printf(">\n");
						}
					}

					lastAddress = instInfo->address;
					lastInstSize = instInfo->instSize;
				}

				int n;

				n = printf("    %s:",dst);

				for (int i = n; i < 20; i++) {
					printf(" ");
				}

				instInfo->instructionToText(dst,instlevel);
				printf("  %s",dst);

				printf("\n");

				firstPrint = false;
			}

			if ((trace_flag || itcprint_flag) && (msgInfo != nullptr)) {
				// got the goods! Get to it!
				char *itcprint = nullptr;

				msgInfo->messageToText(dst,&itcprint,msgLevel);

				if (trace_flag) {
					if (firstPrint == false) {
						printf("\n");
					}

					printf("Trace: %s",dst);

					printf("\n");

					firstPrint = false;
				}

				if (itcprint_flag && (itcprint != nullptr)) {
					if (firstPrint == false) {
						printf("\n");
					}

					puts(itcprint);

					firstPrint = false;
				}
			}
		}
	} while (ec == dqr::DQERR_OK);

//	if (firstPrint == false) {
//		cout << endl;
//	}

	delete trace;

	return 0;
}

/*
 * Copyright 2019 SiFive, Inc.
 *
 * trace.hpp
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

#ifndef TRACE_HPP_
#define TRACE_HPP_

// if config.h is not present, uncomment the lines below

//#define PACKAGE 1
//#define PACKAGE_VERSION 1

// private definitions

#include "config.h"
#include "bfd.h"
#include "dis-asm.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>

// class section: work with elf file sections using libbfd

class section {
public:
	section();
	section *initSection(section **head,asection *newsp);
	section *getSectionByAddress(dqr::ADDRESS addr);

	section     *next;
	bfd         *abfd;
	dqr::ADDRESS startAddr;
	dqr::ADDRESS endAddr;
	int          size;
	asection    *asecptr;
	uint16_t    *code;
};

// class fileReader: Helper class to handler list of source code files

class fileReader {
public:
	struct fileList {
		fileList *next;
		char     *name;
		int       lineCount;
		char    **lines;
	};

	fileReader(/*paths?*/);

	fileList *findFile(const char *file);
private:
	fileList *readFile(const char *file);

	fileList *lastFile;
	fileList *files;
};

// class Symtab: Interface class between bfd symbols and what is needed for dqr

class Symtab {
public:
	             Symtab(bfd *abfd);
	            ~Symtab();
	const char  *getSymbolByAddress(dqr::ADDRESS addr);
	const char  *getNextSymbolByAddress();
//	dqr::ADDRESS getSymbolByName();
	asymbol    **getSymbolTable() { return symbol_table; }
	void         dump();

private:
	bfd      *abfd;
	long      number_of_symbols;
    asymbol **symbol_table;

    dqr::ADDRESS vma;
    int          index;
};

// Class ElfReader: Interface class between dqr and bfd

class ElfReader {
public:
        	   ElfReader(char *elfname);
	          ~ElfReader();
	dqr::DQErr getStatus() { return status; }
	dqr::DQErr getInstructionByAddress(dqr::ADDRESS addr, dqr::RV_INST &inst);
	Symtab    *getSymtab();
	bfd       *get_bfd() {return abfd;}
	int        getArchSize() { return archSize; }
	int        getBitsPerAddress() { return bitsPerAddress; }

private:
	static bool init;
	dqr::DQErr  status;
	bfd        *abfd;
	int         archSize;
	int	        bitsPerWord;
	int         bitsPerAddress;
	section	   *codeSectionLst;
	Symtab     *symtab;
};

class itcPrint {
private:
	static bool init();
public:
	static char *print(uint8_t core, uint32_t address, uint32_t data);

	static bool inited;
	static bool buffering;
	static bool eol[DQR_MAXCORES];
	static char pbuff[DQR_MAXCORES][1024];
	static int pbi[DQR_MAXCORES];
};

#ifdef foo
class linkedNexusMessage {
public:
	linkedNexusMessage();
	static void init();
	static dqr::DQErr buildLinkedMsgs(NexusMessage &nm);
	static dqr::DQErr nextTraceMessage(NexusMessage &nm);

    linkedNexusMessage *nextCoreMessage;
    linkedNexusMessage *nextInOrderMessage;

    bool consumed;
    static linkedNexusMessage *firstMsg;
    static int lastCore;
    static linkedNexusMessage *linkedNexusMessageHeads[8];
    static linkedNexusMessage *lastNexusMsgPtr[8];

    NexusMessage nm;
};
#endif // foo

// class SliceFileParser: Class to parse binary or ascii nexus messages into a NexusMessage object
class SliceFileParser {
public:
             SliceFileParser(char *filename, bool binary, int srcBits);
  dqr::DQErr readNextTraceMsg(NexusMessage &nm,class Analytics &analytics);

// foo  dqr::DQErr readAllTraceMsgs();
  dqr::DQErr getErr() { return status; };
  void       dump();

private:
  dqr::DQErr status;
  bool		 firstMsg;

  // add other counts for each message type

  bool          binary;
  int           srcbits;
  std::ifstream tf;
  int           bitIndex;
  int           msgSlices;
  uint8_t       msg[64];
  bool          eom = false;

  dqr::ADDRESS	 currentAddress;
  dqr::TIMESTAMP currentTime;

  dqr::DQErr readBinaryMsg();
  dqr::DQErr readNextByte(uint8_t *byte);
  dqr::DQErr readAscMsg();
  dqr::DQErr parseVarField(uint64_t *val,int *width);
  dqr::DQErr parseFixedField(int width, uint64_t *val);
  dqr::DQErr parseDirectBranch(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseIndirectBranch(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseDirectBranchWS(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseIndirectBranchWS(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseSync(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseCorrelation(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseAuxAccessWrite(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseDataAcquisition(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseOwnershipTrace(NexusMessage &nm,Analytics &analytics);
  dqr::DQErr parseError(NexusMessage &nm,Analytics &analytics);
};

// class Disassembler: class to help in the dissasemblhy of instrucitons

class Disassembler {
public:
	      Disassembler(bfd *abfd);
	int   Disassemble(dqr::ADDRESS addr);

	int   getSrcLines(dqr::ADDRESS addr, const char **filename, const char **functionname, unsigned int *linenumber, const char **line);

	int   decodeInstructionSize(uint32_t inst, int &inst_size);
	int   decodeInstruction(uint32_t instruction,int &inst_size,dqr::instType &inst_type,int32_t &immeadiate,bool &is_branch);

	void  overridePrintAddress(bfd_vma addr, struct disassemble_info *info); // hmm.. don't need info - part of object!

	Instruction getInstructionInfo() { return instruction; }
	Source      getSourceInfo() { return source; }

	dqr::DQErr getStatus() {return status;}

private:
	typedef struct {
		flagword sym_flags;
		bfd_vma  func_vma;
		int      func_size;
	} func_info_t;

	bfd               *abfd;
	disassembler_ftype disassemble_func;
	dqr::DQErr         status;

	bfd_vma           start_address;
	long              number_of_syms;
	asymbol         **symbol_table;
	asymbol         **sorted_syms;
	func_info_t      *func_info;
	disassemble_info *info;
	section	         *codeSectionLst;
	int               prev_index;
	int               cached_sym_index;
	bfd_vma           cached_sym_vma;
	int               cached_sym_size;

	Instruction instruction;
	Source      source;

	class fileReader *fileReader;

	const char  *lastFileName;
	unsigned int lastLineNumber;

	void print_address(bfd_vma vma);
	void print_address_and_instruction(bfd_vma vma);
	void setInstructionAddress(bfd_vma vma);

	int lookup_symbol_by_address(bfd_vma,flagword flags,int *index,int *offset);
	int lookupInstructionByAddress(bfd_vma vma,uint32_t *ins,int *ins_size);
//	int get_ins(bfd_vma vma,uint32_t *ins,int *ins_size);

	int decodeRV32Q0Instruction(uint32_t instruction,int &inst_size,dqr::instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Q1Instruction(uint32_t instruction,int &inst_size,dqr::instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Q2Instruction(uint32_t instruction,int &inst_size,dqr::instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Instruction(uint32_t instruction,int &inst_size,dqr::instType &inst_type,int32_t &immeadiate,bool &is_branch);
};

struct NexusMessageSync {
	NexusMessageSync();
	int          firstMsgNum;
	int          lastMsgNum;
	int          index;
	NexusMessage msgs[512];
};

#endif /* TRACE_HPP_ */

/*
 * Copyright 2019 SiFive, Inc.
 *
 * dqr.hpp
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

#ifndef DQR_HPP_
#define DQR_HPP_

// if config.h is not present, uncomment the lines below

//#define PACKAGE 1
//#define PACKAGE_VERSION 1

#include "config.h"
#include "bfd.h"
#include "dis-asm.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>

// could wrap all this in a class to provide namespace scoping - do it later!!

class dqr {
public:
  typedef uint32_t RV_INST;

  typedef uint64_t ADDRESS;
  typedef uint64_t TIMESTAMP;

  typedef enum {
  	MSEO_NORMAL  = 0x00,
  	MSEO_VAR_END = 0x01,
  	MSEO_END     = 0x03,
  } MSEO;

  typedef enum {
  	DQERR_OK   = 0,		// no error
  	DQERR_OPEN = 1,		// can't open file
  	DQERR_EOF  = 2,		// at file eof
  	DQERR_EOM  = 3,		// at file eom
  	DQERR_BM   = 4,		// bad message (mallformed)
  	DQERR_ERR  = 5,		// general error
  	DQERR_DONE = 6,		// done with trace message
  } DQErr;

  typedef enum {
  	TCODE_DEBUG_STATUS       = 0,
  	TCODE_DEVICE_ID          = 1,
  	TCODE_OWNERSHIP_TRACE    = 2,
  	TCODE_DIRECT_BRANCH      = 3,
  	TCODE_INDIRECT_BRANCH    = 4,
  	TCODE_DATA_WRITE         = 5,
  	TCODE_DATA_READ          = 6,
  	TCODE_DATA_ACQUISITION   = 7,
  	TCODE_ERROR              = 8,
  	TCODE_SYNC               = 9,
  	TCODE_CORRECTION         = 10,
  	TCODE_DIRECT_BRANCH_WS   = 11,
  	TCODE_INDIRECT_BRANCH_WS = 12,
  	TCODE_DATA_WRITE_WS      = 13,
  	TCODE_DATA_READ_WS       = 14,
  	TCODE_WATCHPOINT         = 15,
  	TCODE_OUTPUT_PORTREPLACEMENT  = 20,
  	TCODE_INPUT_PORTREPLACEMENT   = 21,
  	TCODE_AUXACCESS_READ          = 22,
  	TCODE_AUXACCESS_WRITE         = 23,
  	TCODE_AUXACCESS_READNEXT      = 24,
  	TCODE_AUXACCESS_WRITENEXT     = 25,
  	TCODE_AUXACCESS_RESPONSE      = 26,
  	TCODE_RESURCEFULL             = 27,
  	TCODE_INDIRECTBRANCHHISOTRY   = 28,
  	TCODE_INDIRECTBRANCHHISORY_WS = 29,
  	TCODE_REPEATBRANCH            = 30,
  	TCODE_REPEATINSTRUCITON       = 31,
  	TCODE_REPEATSINSTURCIONT_WS   = 32,
  	TCODE_CORRELATION             = 33,
  	TCODE_INCIRCUITTRACE          = 34,

  	TCODE_UNDEFINED
  } TCode;

  typedef enum {
  	EVCODE_ENTERDEBUG	= 0,
  	EVCODE_TRACEDISABLE = 4,
  	EVCODE_ENTERRESET   = 8
  } EVCode;

  typedef enum {
  	SYNC_EVTI               = 0,
  	SYNC_EXIT_RESET         = 1,
  	SYNC_T_CNT              = 2,
  	SYNC_EXIT_DEBUG         = 3,
  	SYNC_I_CNT_OVERFLOW     = 4,
  	SYNC_TRACE_ENABLE       = 5,
  	SYNC_WATCHPINT          = 6,
  	SYNC_FIFO_OVERRUN       = 7,
  	SYNC_EXIT_POWERDOWN     = 9,
  	SYNC_MESSAGE_CONTENTION = 11,
  	SYNC_NONE
  } SyncReason;

  typedef enum {
  	BTYPE_INDIRECT  = 0,
  	BTYPE_EXCEPTION = 1,
  	BTYPE_HARDWARE  = 2,

  	BTYPE_UNDEFINED
  } BType;

  typedef enum {
	  ADDRDISP_WIDTHAUTO = 1,
	  ADDRDISP_SEP  = 2,
  } AddrDisp;
};

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

// class Instruction: work with an instruction

class Instruction {
public:
	void addressToText(char *dst,int labelLevel);
	void opcodeToText();
	void instructionToText(char *dst,int labelLevel);

	dqr::ADDRESS      address;
	dqr::RV_INST      instruction;
	char              instructionText[64];
	int               instSize;
	static int        addrSize;
	static uint32_t   addrDispFlags;
	static int        addrPrintWidth;
	const char       *addressLabel;
	int               addressLabelOffset;
	bool              haveOperandAddress;
	dqr::ADDRESS      operandAddress;
	const char       *operandLabel;
	int               operandLabelOffset;
};

// class Source: Helper class for source code information for an address

class Source {
public:
	const char  *sourceFile;
	const char  *sourceFunction;
	unsigned int sourceLineNum;
	const char  *sourceLine;
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
	dqr::ADDRESS getSymbolByName();
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

// class NexusMessage: class to hold Nexus messages and convert them to text

class NexusMessage {
public:
	NexusMessage();
	void messageToText(char *dst,char **pdst,int level);
	void dump();

	int        	   msgNum;
	dqr::TCode     tcode;
    bool       	   haveTimestamp;
    dqr::TIMESTAMP timestamp;
    dqr::ADDRESS   currentAddress;
    dqr::TIMESTAMP time;

    union {
    	struct {
    		int i_cnt;
    	} directBranch;
    	struct {
    		int          i_cnt;
    		dqr::ADDRESS u_addr;
    		dqr::BType   b_type;
    	} indirectBranch;
    	struct {
    		int             i_cnt;
    		dqr::ADDRESS    f_addr;
    		dqr::SyncReason sync;
    	} directBranchWS;
    	struct {
    		int             i_cnt;
    		dqr::ADDRESS    f_addr;
    		dqr::BType      b_type;
    		dqr::SyncReason sync;
    	} indirectBranchWS;
    	struct {
    		int             i_cnt;
    		dqr::ADDRESS    f_addr;
    		dqr::SyncReason sync;
    	} sync;
    	struct {
    		uint8_t etype;
    	} error;
    	struct {
    		int     i_cnt;
    		uint8_t cdf;
    		uint8_t evcode;
    	} correlation;
    	struct {
    		uint32_t data;
    		uint32_t addr;
    	} auxAccessWrite;
    	struct {
    		uint32_t process;
    	} ownership;
    };

private:
    // empty
};

// class SliceFileParser: Class to parse binary or ascii nexus messages into a NexusMessage object
class SliceFileParser {
public:
             SliceFileParser(char *filename, bool binary);
  dqr::DQErr nextTraceMsg(NexusMessage &nm);
  dqr::DQErr getErr() { return status; };
  void       dump();

private:
  dqr::DQErr status;
  int        numTraceMsgs;
  int        numSyncMsgs;

  // add other counts for each message type

  bool          binary;
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
  dqr::DQErr parseVarField(uint64_t *val);
  dqr::DQErr parseFixedField(int width, uint64_t *val);
  dqr::DQErr parseDirectBranch(NexusMessage &nm);
  dqr::DQErr parseIndirectBranch(NexusMessage &nm);
  dqr::DQErr parseDirectBranchWS(NexusMessage &nm);
  dqr::DQErr parseIndirectBranchWS(NexusMessage &nm);
  dqr::DQErr parseSync(NexusMessage &nm);
  dqr::DQErr parseCorrelation(NexusMessage &nm);
  dqr::DQErr parseAuxAccessWrite(NexusMessage &nm);
  dqr::DQErr parseOwnershipTrace(NexusMessage &nm);
  dqr::DQErr parseError(NexusMessage &nm);
};

// class Disassembler: class to help in the dissasemblhy of instrucitons

class Disassembler {
public:
	enum instType {
		UNKNOWN = 0,
		JAL,
		JALR,
		BEQ,
		BNE,
		BLT,
		BGE,
		BLTU,
		BGEU,
		C_J,
		C_JAL,
		C_JR,
		C_JALR,
		C_BEQZ,
		C_BNEZ,
	};

	      Disassembler(bfd *abfd);
	int   Disassemble(dqr::ADDRESS addr);

	int   getSrcLines(dqr::ADDRESS addr, const char **filename, const char **functionname, unsigned int *linenumber, const char **line);

	int   decodeInstructionSize(uint32_t inst, int &inst_size);
	int   decodeInstruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);

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

	int decodeRV32Q0Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Q1Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Q2Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);
	int decodeRV32Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);
};

struct NexusMessageSync {
	NexusMessageSync();
	int          firstMsgNum;
	int          lastMsgNum;
	int          index;
	NexusMessage msgs[512];
};

// class Trace: high level class that performs the raw trace data to dissasemble and decorated instruction trace

class Trace {
public:
	enum SymFlags {
		SYMFLAGS_NONE = 0,
		SYMFLAGS_xx   = 1 << 0,
	};
	           Trace(char *tf_name, bool binaryFlag, char *ef_name, SymFlags sym_flags, int numAddrBits, uint32_t addrDispFlags);
	          ~Trace();
	dqr::DQErr setTraceRange(int start_msg_num,int stop_msg_num);

	enum traceFlags {
		TF_INSTRUCTION = 0x01,
		TF_ADDRESS     = 0x02,
		TF_DISSASEMBLE = 0x04,
		TF_TIMESTAMP   = 0x08,
		TF_TRACEINFO   = 0x10,
	};
	dqr::DQErr getStatus() { return status; }
	dqr::DQErr NextInstruction(Instruction **instInfo, NexusMessage **msgInfo, Source **srcInfo);

	const char *getSymbolByAddress(dqr::ADDRESS addr) { return symtab->getSymbolByAddress(addr); }
	const char *getNextSymbolByAddress() { return symtab->getNextSymbolByAddress(); }
	int         Disassemble(dqr::ADDRESS addr);
	int         getArchSize();
	int         getAddressSize();

private:
	enum state {
		TRACE_STATE_GETFIRSTYNCMSG,
		TRACE_STATE_GETSECONDMSG,
		TRACE_STATE_GETSTARTTRACEMSG,
		TRACE_STATE_COMPUTESTARTINGADDRESS,
		TRACE_STATE_RETIREMESSAGE,
		TRACE_STATE_GETNEXTMSG,
		TRACE_STATE_GETNEXTINSTRUCTION,
		TRACE_STATE_DONE,
		TRACE_STATE_ERROR
	};

	dqr::DQErr       status;
	SliceFileParser *sfp;
	ElfReader       *elfReader;
	Symtab          *symtab;
	Disassembler    *disassembler;
	SymFlags		 symflags;
	dqr::ADDRESS     currentAddress;
	dqr::ADDRESS	 lastFaddr;
	dqr::TIMESTAMP   lastTime;
	enum state       state;

	int              startMessageNum;
	int              endMessageNum;

//	need current message number and list of messages??

	NexusMessage     nm;

	NexusMessage     messageInfo;
	Instruction      instructionInfo;
	Source           sourceInfo;

	//	or maybe have this stuff in the nexus messages??

	int i_cnt;

	uint32_t               inst = -1;
	int                    inst_size = -1;
	Disassembler::instType inst_type = Disassembler::instType::UNKNOWN;
	int32_t                immeadiate = -1;
	bool                   is_branch = false;

	NexusMessageSync      *messageSync;

	int decodeInstructionSize(uint32_t inst, int &inst_size);
	int decodeInstruction(uint32_t instruction,int &inst_size,Disassembler::instType &inst_type,int32_t &immeadiate,bool &is_branch);

	dqr::ADDRESS computeAddress();
};

#endif /* DQR_HPP_ */

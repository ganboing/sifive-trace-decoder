/*
 * Copyright 2019 Sifive, Inc. All rights reserved.
 *
 * main.cpp
 *
 *  Created on: Feb 11, 2019
 *      Author: Brad Seevers
 */

/*
 * to do:
 *
 * create .h files for classes
 * creates separate source files for classes
 * eliminate redundancy in dissassembler and other classes (get instruction, symtab, text block)
 * don't print in classes - just create stings or have method to create strings
 * re-factor nextinstruction to reduce size and maybe redundant code for states
 * maybe add method to check for presence of next instruction
 * use bfd_sprintf functions
 * get line info working
 * consistent ADDR useage
 * figure out base+offset or just ADDR for addresses
 * consistent naming style
 * correct destructors for everything
 * consistnat error code useage, and more types of error codes
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>

#include "dqr.hpp"

using namespace std;

// could wrap all this in a class to provide namespace scoping - do it later!!

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
	TCODE_OUTPUT_PORTREPLACEMENT = 20,
	TCODE_INPUT_PORTREPLACEMENT = 21,
	TCODE_AUXACCESS_READ        = 22,
	TCODE_AUXACCESS_WRITE       = 23,
	TCODE_AUXACCESS_READNEXT    = 24,
	TCODE_AUXACCESS_WRITENEXT   = 25,
	TCODE_AUXACCESS_RESPONSE    = 26,
	TCODE_RESURCEFULL           = 27,
	TCODE_INDIRECTBRANCHHISOTRY = 28,
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

class section {
public:
	section();
	section  *initSection(section **head,asection *newsp);
	section  *getSectionByAddress(ADDRESS addr);
	section  *next;
	bfd      *abfd;
	ADDRESS   startAddr;
	ADDRESS   endAddr;
	int       size;
	asection *asecptr;
	uint16_t *code;
};

section::section()
{
	next      = nullptr;
	abfd      = nullptr;
	asecptr   = nullptr;
	size      = 0;
	startAddr = (ADDRESS)0;
	endAddr   = (ADDRESS)0;
	code      = nullptr;
}

section *section::initSection(section **head, asection *newsp)
{
	next = *head;
	*head = this;

	abfd = newsp->owner;
	asecptr = newsp;
	size = newsp->size;
	startAddr = (ADDRESS)newsp->vma;
	endAddr = (ADDRESS)(newsp->vma + size - 1);

	int words = (size+1)/2;

	code = nullptr;

//	could make code only read when it is needed (lazzy eval), so if nver used, no memory is
//	ever allocated and no time to read

	code = new (std::nothrow) uint16_t[words];

	assert(code != nullptr);

    bfd_boolean rc;
    rc = bfd_get_section_contents(abfd,newsp,(void*)code,0,size);
    if (rc != TRUE) {
      printf("bfd_get_section_contents() failed\n");
      return nullptr;
    }

    return this;
}

section *section::getSectionByAddress(ADDRESS addr)
{
	section *sp = this;

	while (sp != nullptr) {
		if (addr >= sp->startAddr && addr <= sp->endAddr) {
			return sp;
		}

		sp = sp->next;
	}

	return nullptr;
}

class Instruction {
public:
	void addressToText(char *dst,int labelLevel);
	void opcodeToText();
	void instructionToText(char *dst,int labelLevel);

	ADDRESS address;
	RV_INST instruction;
	char instructionText[64];
	int instSize;
	const char *addressLabel;
	int addressLabelOffset;
	bool haveOperandAddress;
	ADDRESS operandAddress;
	const char *operandLabel;
	int operandLabelOffset;
};

void Instruction::addressToText(char *dst,int labelLevel)
{
	assert(dst != nullptr);

	dst[0] = 0;

	if (labelLevel == 1) {
		if (addressLabel != nullptr) {
			if (addressLabelOffset != 0) {
				sprintf(dst,"%08x <%s+%x>",address,addressLabel,addressLabelOffset);
			}
			else {
				sprintf(dst,"%08x <%s>",address,addressLabel);
			}
		}
		else {
			sprintf(dst,"%08x",address);
		}
	}
	else if (labelLevel == 0) {
		sprintf(dst,"%08x",address);
	}
}

void Instruction::instructionToText(char *dst,int labelLevel)
{
	assert(dst != nullptr);

	int n;

	dst[0] = 0;

	if (instSize == 32) {
		n = sprintf(dst,"%08x           %s",instruction,instructionText);
	}
	else {
		n = sprintf(dst,"%04x               %s",instruction,instructionText);
	}

	if (haveOperandAddress) {
		n += sprintf(dst+n,"%x",operandAddress);

		if (labelLevel >= 1) {
			if (operandLabel != nullptr) {
				if (operandLabelOffset != 0) {
					sprintf(dst+n," <%s+%x>",operandLabel,operandLabelOffset);
				}
				else {
					sprintf(dst+n," <%s>",operandLabel);
				}
			}
		}
	}
}

class Source {
public:
	const char *sourceFile;
	const char *sourceFunction;
	unsigned int sourceLineNum;
	const char *sourceLine;
};

class fileReader {
public:
	struct fileList {
		fileList *next;
		char *name;
		int lineCount;
		char **lines;
	};

	fileReader(/*paths?*/);

	fileList *findFile(const char *file);
private:
	fileList *readFile(const char *file);

	fileList *lastFile;
	fileList *files;
};

fileReader::fileReader(/*paths*/)
{
	lastFile = nullptr;
	files = nullptr;
}

fileReader::fileList *fileReader::readFile(const char *file)
{
	assert(file != nullptr);

	ifstream  f(file, ifstream::binary);

	if (!f) {
//		printf("Error: readFile(): could not open file %s for input\n",file);

		// try again after stripping off path

		int l = -1;

		for (int i = 0; file[i] != 0; i++) {
			if ((file[i] == '/') || (file[i] == '\\')) {
				l = i;
			}
		}

		if (l != -1) {
			file = &file[l];
			f.open(file, ifstream::binary);
		}
	}

	// could put another try here is !f is true, such as searching a provided path?

	// if (!f) {
	//    char fn[256];
	//
	//    strcpy(fn,path);
	//    if ((path[strlen(path)] != '/') && (path[strlen(path)] != '\\')) {
	//        strcat(fn,'/');
	//    }
	//    strcat(fn,file);	// file is potentially adjusted above]
	//
	//    f.open(fn, ifsteam::binary);
	//  }

	if (!f) {
		return nullptr;
	}

	// get length of file:

	f.seekg (0, f.end);
	int length = f.tellg();
	f.seekg (0, f.beg);

	// allocate memory:

	char *buffer = new char [length];

	// read file into buffer

	f.read (buffer,length);

	f.close();

	// count lines

	int lc = 1;

	for (int i = 0; i < length; i++) {
		if (buffer[i] == '\n') {
			lc += 1;
		}
	}

	// create array of line pointers

	char **lines = new char *[lc];

	// initialize arry of ptrs

	int l;
	int s;

	l = 0;
	s = 1;

	for (int i = 0; i < length;i++) {
		if (s != 0) {
			lines[l] = &buffer[i];
			l += 1;
			s = 0;
		}

		// strip out CRs and LFs

		if (buffer[i] == '\r') {
			buffer[i] = 0;
		}
		else if (buffer[i] == '\n') {
			buffer[i] = 0;
			s = 1;
		}
	}

	if (l >= lc) {
		printf("Error: readFile(): Error computing line count for file\n");
		return nullptr;
	}

	lines[l] = nullptr;

	fileList *fl = new fileList;

	fl->next = files;

	int len = strlen(file)+1;
	char *name = new char[len];
	strcpy(name,file);

	fl->name = name;

	fl->lineCount = l;
	fl->lines = lines;

	files = fl;

	return fl;

}

fileReader::fileList *fileReader::findFile(const char *file)
{
	assert(file != nullptr);

	// first check if file is same as last one uesed


	if (lastFile != nullptr) {
		if (strcmp(lastFile->name,file) == 0) {
			// found file!
			return lastFile;
		}
	}

	struct fileList *fp;

	for (fp = files; fp != nullptr; fp = fp->next) {
		if (strcmp(fp->name,file) == 0) {
			lastFile = fp;
			return fp;
		}
	}

	// didn't find file. See if we can read it in

	fp = readFile(file);

	return fp;
}

static void override_print_address(bfd_vma addr, struct disassemble_info *info);

class Symtab {
public:
	            Symtab(bfd *abfd);
	           ~Symtab();
	const char *getSymbolByAddress(ADDRESS addr);
	const char *getNextSymbolByAddress();
	ADDRESS     getSymbolByName();
	asymbol   **getSymbolTable() { return symbol_table; }
	void        dump();

private:
	bfd *abfd;
	long number_of_symbols;
    asymbol **symbol_table;

    bfd_vma vma;
    int index;
};

Symtab::Symtab(bfd *abfd)
{
	assert(abfd != nullptr);

	this->abfd = abfd;

	if ((bfd_get_file_flags (abfd) & HAS_SYMS)) {
		long storage_needed;

		storage_needed = bfd_get_symtab_upper_bound(abfd);

		if (storage_needed > 0 ) {
			symbol_table = (asymbol**)new (std::nothrow) char[storage_needed];

			assert(symbol_table != nullptr);

			number_of_symbols = bfd_canonicalize_symtab(abfd,symbol_table);

//			printf("symtable 2: storage needed: %d, number of symbols: %d, symbol_table: %08x\n",storage_needed, number_of_symbols, symbol_table);
		}
		else {
			symbol_table = nullptr;
		}
	}
	else {
		symbol_table = nullptr;
	}

    vma = 0;
    index = 0;
}

Symtab::~Symtab()
{
	if (symbol_table != nullptr) {
		delete[] symbol_table;
	}
}

const char *Symtab::getSymbolByAddress(ADDRESS addr)
{
	printf("getSymbolByAddress()\n");

	if (addr == 0) {
		return nullptr;
	}

	vma = addr;

	struct bfd_section *section;
	bfd_vma section_base_vma;

	index = 0;

	for (int i = 0; i < number_of_symbols; i++) {
		section = symbol_table[i]->section;
	    section_base_vma = section->vma;

	    if (section_base_vma + symbol_table[i]->value == vma) {
	    	printf("symabol match for address %p, name: %s\n",vma,symbol_table[i]->name);
//	    	&& (symbol_table[i]->flags & BSF_FUNCTION))
	    }

	    if ((section_base_vma + symbol_table[i]->value == vma) && (symbol_table[i]->flags & BSF_FUNCTION)) {
	    	index = i;

	    	return symbol_table[i]->name;
	    }
	}

	return nullptr;
}

// probably can delete the getNextSymboByAddress() funcs

const char *Symtab::getNextSymbolByAddress()
{
	struct bfd_section *section;
	bfd_vma section_base_vma;

	for (int i = index+1; i < number_of_symbols; i++) {
		section = symbol_table[i]->section;
	    section_base_vma = section->vma;

	    if ((section_base_vma + symbol_table[i]->value == vma) && (symbol_table[i]->flags & BSF_FUNCTION)) {
	    	index = i;

	    	return symbol_table[i]->name;
	    }
	}

	return nullptr;
}

void Symtab::dump()
{
    printf("number_of_symbols: %ld\n",number_of_symbols);

    for (int i = 0; i < number_of_symbols; i++) {
      printf("symname: %s: \t",symbol_table[i]->name);
      bfd_print_symbol_vandf(abfd,stdout,symbol_table[i]);
      printf("\n");
    }
}

class ElfReader {
public:
        	 ElfReader(char *elfname);
	        ~ElfReader();
	DQErr    getStatus() { return status; }
	DQErr    getInstructionByAddress(ADDRESS addr, RV_INST &inst);
	Symtab  *getSymtab();
	bfd     *get_bfd() {return abfd;}

private:
	static bool  init;
	DQErr        status;
	bfd         *abfd;
	section	    *codeSectionLst;
	Symtab      *symtab;
};

bool ElfReader::init = false;

ElfReader::ElfReader(char *elfname)
{
  assert(elfname != nullptr);

  if (init == false) {
	  // only call bfd_init once - not once per object

	  bfd_init();
	  init = true;
  }

  abfd = bfd_openr(elfname,NULL);
  if (abfd == nullptr) {
    status = DQERR_ERR;

    bfd_error_type bfd_error = bfd_get_error();
    printf("Error: bfd_openr() returned null. Error: %d\n",bfd_error);

    return;
  }

  if(!bfd_check_format(abfd,bfd_object)) {
    printf("Error: ElfReader(): %s not object file: %d\n",elfname,bfd_get_error());

	status = DQERR_ERR;

	return;
  }

  symtab = nullptr;
  codeSectionLst = nullptr;

  for (asection *p = abfd->sections; p != NULL; p = p->next) {
	  if (p->flags & SEC_CODE) {
          // found a code section, add to list

		  section *sp = new section;
		  sp->initSection(&codeSectionLst, p);
	  }
  }

  status = DQERR_OK;
}

ElfReader::~ElfReader()
{
	if (symtab != nullptr) {
		delete symtab;
		symtab = nullptr;
	}
}

DQErr ElfReader::getInstructionByAddress(ADDRESS addr,RV_INST &inst)
{
	// get instruction at addr

	// Address for code[0] is text->vma

	//don't forget base!!'

	section *sp;
	if (codeSectionLst == nullptr) {
		status = DQERR_ERR;
		return status;
	}

	sp = codeSectionLst->getSectionByAddress(addr);
	if (sp == nullptr) {
		status = DQERR_ERR;
		return status;
	}

	if ((addr < sp->startAddr) || (addr > sp->endAddr)) {
		status = DQERR_ERR;
		return status;
	}

//	if ((addr < text->vma) || (addr >= text->vma + text->size)) {
////			don't know instruction - not part of text segment. need to handle for os
////			if we can't get the instruciton, get the next trace message and try again. or do
////			we need the next sync message and the next trace message and try again?
//
//		// for now, just return an error
//
//		status = DQERR_ERR;
//		return status;
//	}

	int index;

	index = (addr - sp->startAddr) / 2;

	inst = sp->code[index];

	// if 32 bit instruction, get the second half

	switch (inst & 0x0003) {
	case 0x0000:	// quadrant 0, compressed
	case 0x0001:	// quadrant 1, compressed
	case 0x0002:	// quadrant 2, compressed
		status = DQERR_OK;
		break;
	case 0x0003:	// not compressed. Assume RV32 for now
		if ((inst & 0x1f) == 0x1f) {
			fprintf(stderr,"Error: getInstructionByAddress(): cann't decode instructions longer than 32 bits\n");
			status = DQERR_ERR;
			break;
		}

		inst = inst | (((uint32_t)sp->code[index+1]) << 16);

		status = DQERR_OK;
		break;
	}

	return status;
}

Symtab *ElfReader::getSymtab()
{
	if (symtab == nullptr) {
		symtab = new (std::nothrow) Symtab(abfd);

		assert(symtab != nullptr);
	}

	return symtab;
}

class NexusMessage {
public:
	NexusMessage();
	void messageToText(char *dst,int level);
	void dump();

	int        msgNum;
	TCode      tcode;
    bool       haveTimestamp;
    uint64_t   timestamp;
    ADDRESS    currentAddress;
    uint64_t   time;

    union {
    	struct {
    		int i_cnt;
    	} directBranch;
    	struct {
    		int      i_cnt;
    		uint64_t u_addr;
    		BType    b_type;
    	} indirectBranch;
    	struct {
    		int        i_cnt;
    		uint64_t   f_addr;
    		SyncReason sync;
    	} directBranchWS;
    	struct {
    		int        i_cnt;
    		uint64_t   f_addr;
    		BType      b_type;
    		SyncReason sync;
    	} indirectBranchWS;
    	struct {
    		int i_cnt;
    		uint64_t f_addr;
    		SyncReason sync;
    	} sync;
    	struct {
    		uint8_t etype;
    	} error;
    	struct {
    		int i_cnt;
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

NexusMessage::NexusMessage()
{
	msgNum         = 0;
	tcode          = TCODE_UNDEFINED;
	haveTimestamp  = false;
	timestamp      = 0;
	currentAddress = 0;
	time           = 0;
}

void NexusMessage::messageToText(char *dst,int level)
{
	assert(dst != nullptr);

	const char *sr;
	const char *bt;
	int n;

	// level = 0, nothing
	// level = 1, timestamp and target
	// level = 2, message info + timestamp + target

	if (level <= 0) {
		dst[0] = 0;
		return;
	}

	if (haveTimestamp) {
		n = sprintf(dst,"Msg # %d, Tics: %d, NxtAddr: %08x, TCode: ",msgNum,time,currentAddress);
	}
	else {
		n = sprintf(dst,"Msg # %d, NxtAddr: %08x, TCode: ",msgNum,currentAddress);
	}

	switch (tcode) {
	case TCODE_DEBUG_STATUS:
		sprintf(dst+n,"DEBUG STATUS (%d)",tcode);
		break;
	case TCODE_DEVICE_ID:
		sprintf(dst+n,"DEVICE ID (%d)",tcode);
		break;
	case TCODE_OWNERSHIP_TRACE:
		n += sprintf(dst+n,"OWNERSHIP TRACE (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," process: %d",ownership.process);
		}
		break;
	case TCODE_DIRECT_BRANCH:
		n += sprintf(dst+n,"DIRECT BRANCH (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," I-CNT: %d",directBranch.i_cnt);
		}
		break;
	case TCODE_INDIRECT_BRANCH:
		n += sprintf(dst+n,"INDIRECT BRANCH (%d)",tcode);

		if (level >= 2) {
			switch (indirectBranch.b_type) {
			case BTYPE_INDIRECT:
				bt = "Indirect";
				break;
			case BTYPE_EXCEPTION:
				bt = "Exception";
				break;
			case BTYPE_HARDWARE:
				bt = "Hardware";
				break;
			case BTYPE_UNDEFINED:
				bt = "Undefined";
				break;
			default:
				bt = "Bad Branch Type";
				break;
			}

			sprintf(dst+n," Branch Type: %s (%d) I-CNT: %d U-ADDR: 0x%08x ",bt,indirectBranch.b_type,indirectBranch.i_cnt,indirectBranch.u_addr);
		}
		break;
	case TCODE_DATA_WRITE:
		sprintf(dst+n,"DATA WRITE (%d)",tcode);
		break;
	case TCODE_DATA_READ:
		sprintf(dst+n,"DATA READ (%d)",tcode);
		break;
	case TCODE_DATA_ACQUISITION:
		sprintf(dst+n,"DATA ACQUISITION (%d)",tcode);
		break;
	case TCODE_ERROR:
		n += sprintf(dst+n,"ERROR (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," Error Type %d",error.etype);
		}
		break;
	case TCODE_SYNC:
		n += sprintf(dst+n,"SYNC (%d)",tcode);

		if (level >= 2) {
			switch (sync.sync) {
			case SYNC_EVTI:
				sr = "EVTI";
				break;
			case SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case SYNC_T_CNT:
				sr = "T Count";
				break;
			case SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s I-CNT: %d F-Addr: 0x%08x",sync.sync,sr,sync.i_cnt,sync.f_addr);
		}
		break;
	case TCODE_CORRECTION:
		sprintf(dst+n,"Correction (%d)",tcode);
		break;
	case TCODE_DIRECT_BRANCH_WS:
		n += sprintf(dst+n,"DIRECT BRANCH WS (%d)",tcode);

		if (level >= 2) {
			switch (directBranchWS.sync) {
			case SYNC_EVTI:
				sr = "EVTI";
				break;
			case SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case SYNC_T_CNT:
				sr = "T Count";
				break;
			case SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s I-CNT: %d F-Addr: 0x%08x",directBranchWS.sync,sr,directBranchWS.i_cnt,directBranchWS.f_addr);
		}
		break;
	case TCODE_INDIRECT_BRANCH_WS:
		n += sprintf(dst+n,"INDIRECT BRANCH WS (%d)",tcode);

		if (level >= 2) {
			switch (indirectBranchWS.sync) {
			case SYNC_EVTI:
				sr = "EVTI";
				break;
			case SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case SYNC_T_CNT:
				sr = "T Count";
				break;
			case SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			switch (indirectBranchWS.b_type) {
			case BTYPE_INDIRECT:
				bt = "Indirect";
				break;
			case BTYPE_EXCEPTION:
				bt = "Exception";
				break;
			case BTYPE_HARDWARE:
				bt = "Hardware";
				break;
			case BTYPE_UNDEFINED:
				bt = "Undefined";
				break;
			default:
				bt = "Bad Branch Type";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s Branch Type %s (%d) I-CNT: %d F-Addr: 0x%08x",indirectBranchWS.sync,sr,bt,indirectBranchWS.b_type,indirectBranchWS.i_cnt,indirectBranchWS.f_addr);
		}
		break;
	case TCODE_DATA_WRITE_WS:
		sprintf(dst+n,"DATA WRITE WS (%d)",tcode);
		break;
	case TCODE_DATA_READ_WS:
		sprintf(dst+n,"DATA READ WS (%d)",tcode);
		break;
	case TCODE_WATCHPOINT:
		sprintf(dst+n,"TCode: WATCHPOINT (%d)",tcode);
		break;
	case TCODE_OUTPUT_PORTREPLACEMENT:
		sprintf(dst+n,"OUTPUT PORT REPLACEMENT (%d)",tcode);
		break;
	case TCODE_INPUT_PORTREPLACEMENT:
		sprintf(dst+n,"INPUT PORT REPLACEMENT (%d)",tcode);
		break;
	case TCODE_AUXACCESS_READ:
		sprintf(dst+n,"AUX ACCESS READ (%d)",tcode);
		break;
	case TCODE_AUXACCESS_WRITE:
		n += sprintf(dst+n,"AUX ACCESS WRITE (%d)",tcode);

		if (level >= 2) {
			sprintf(dst+n," Addr: 0x%08x Data: %0x08x",auxAccessWrite.addr,auxAccessWrite.data);
		}
		break;
	case TCODE_AUXACCESS_READNEXT:
		sprintf(dst+n,"AUX ACCESS READNEXT (%d)",tcode);
		break;
	case TCODE_AUXACCESS_WRITENEXT:
		sprintf(dst+n,"AUX ACCESS WRITENEXT (%d)",tcode);
		break;
	case TCODE_AUXACCESS_RESPONSE:
		sprintf(dst+n,"AUXACCESS RESPOINSE (%d)",tcode);
		break;
	case TCODE_RESURCEFULL:
		sprintf(dst+n,"RESOURCE FULL (%d)",tcode);
		break;
	case TCODE_INDIRECTBRANCHHISOTRY:
		sprintf(dst+n,"INDIRECT BRANCH HISTORY (%d)",tcode);
		break;
	case TCODE_INDIRECTBRANCHHISORY_WS:
		sprintf(dst+n,"INDIRECT BRANCH HISTORY WS (%d)",tcode);
		break;
	case TCODE_REPEATBRANCH:
		sprintf(dst+n,"REPEAT BRANCH (%d)",tcode);
		break;
	case TCODE_REPEATINSTRUCITON:
		sprintf(dst+n,"REPEAT INSTRUCTION (%d)",tcode);
		break;
	case TCODE_REPEATSINSTURCIONT_WS:
		sprintf(dst+n,"REPEAT INSTRUCTIN WS (%d)",tcode);
		break;
	case TCODE_CORRELATION:
		n += sprintf(dst+n,"CORRELATION (%d)",tcode);

		if (level >= 2) {
			sprintf(dst+n," EVCODE: %d I-CNT: %d",correlation.evcode,correlation.i_cnt);
		}
		break;
	case TCODE_INCIRCUITTRACE:
		sprintf(dst+n,"INCIRCUITTRACE (%d)",tcode);
		break;
	case TCODE_UNDEFINED:
		sprintf(dst+n,"UNDEFINED (%d)",tcode);
		break;
	default:
		sprintf(dst+n,"BAD TCODE (%d)",tcode);
		break;
	}
}

void NexusMessage::dump()
{
	switch (tcode) {
	case TCODE_DEBUG_STATUS:
		cout << "unsupported debug status trace message\n";
		break;
	case TCODE_DEVICE_ID:
		cout << "unsupported device id trace message\n";
		break;
	case TCODE_OWNERSHIP_TRACE:
//bks
		cout << "  # Trace Message(" << msgNum << "): Ownership, process=" << ownership.process; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case TCODE_DIRECT_BRANCH:
//		cout << "(" << traceNum << ") ";
//		cout << "  | Direct Branch | TYPE=DBM, ICNT=" << i_cnt << ", TS=0x" << hex << timestamp << dec; // << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Direct Branch, ICNT=" << i_cnt; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case TCODE_INDIRECT_BRANCH:
//		cout << "(" << traceNum << ") ";
//		cout << "  | Indirect Branch | TYPE=IBM, BTYPE=" << b_type << ", ICNT=" << i_cnt << ", UADDR=0x" << hex << u_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Indirect Branch, BTYPE=" << b_type << ", ICNT=" << i_cnt << ", UADDR=0x" << hex << u_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case TCODE_DATA_WRITE:
		cout << "unsupported data write trace message\n";
		break;
	case TCODE_DATA_READ:
		cout << "unsupported data read trace message\n";
		break;
	case TCODE_DATA_ACQUISITION:
		cout << "unsupported data acquisition trace message\n";
		break;
	case TCODE_ERROR:
//bks		cout << "  # Trace Message(" << msgNum << "): Error, ETYPE=" << (uint32_t)etype; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case TCODE_SYNC:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Sync | TYPE=SYNC, SYNC=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Sync, SYNCREASON=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case TCODE_CORRECTION:
		cout << "unsupported correction trace message\n";
		break;
	case TCODE_DIRECT_BRANCH_WS:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Direct Branch With Sync | TYPE=DBWS, SYNC=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Direct Branch With Sync, SYNCTYPE=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case TCODE_INDIRECT_BRANCH_WS:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Indirect Branch With Sync | TYPE=IBWS, SYNC=" << sync << ", BTYPE=" << b_type << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Indirect Branch With sync, SYNCTYPE=" << sync << ", BTYPE=" << b_type << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case TCODE_DATA_WRITE_WS:
		cout << "unsupported data write with sync trace message\n";
		break;
	case TCODE_DATA_READ_WS:
		cout << "unsupported data read with sync trace message\n";
		break;
	case TCODE_WATCHPOINT:
		cout << "unsupported watchpoint trace message\n";
		break;
	case TCODE_AUXACCESS_WRITE:
//bks		cout << "  # Trace Message(" << msgNum << "): Auxillary Access Write, address=" << hex << auxAddr << dec << ", data=" << hex << data << dec; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case TCODE_CORRELATION:
//bks		cout << "  # Trace Message(" << msgNum << "): Correlation, EVCODE=" << (uint32_t)evcode << ", CDF=" << (int)cdf << ", ICNT=" << i_cnt << "\n"; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	default:
		cout << "Error: NexusMessage::dump(): Unknown TCODE " << hex << tcode << dec << "msgnum: " << msgNum << endl;
	}
}

class SliceFileParser {
public:
        SliceFileParser(char *filename, bool binary);
  DQErr nextTraceMsg(NexusMessage &nm);
  DQErr getErr() { return status; };
  void  dump();

private:
  DQErr    status;
  int      numTraceMsgs;
  int      numSyncMsgs;
  // add other counts for each message type

  bool     binary;
  ifstream tf;
  int      bitIndex;
  int      msgSlices;
  uint8_t  msg[64];
  bool     eom = false;

  uint64_t	currentAddress;
  uint64_t  currentTime;

  NexusMessage *chachedMsgs = NULL;

  DQErr readBinaryMsg();
  DQErr readNextByte(uint8_t *byte);
  int   atoh(char a);
  DQErr readAscMsg();
  DQErr parseVarField(uint64_t *val);
  DQErr parseFixedField(int width, uint64_t *val);
  DQErr parseDirectBranch(NexusMessage &nm);
  DQErr parseIndirectBranch(NexusMessage &nm);
  DQErr parseDirectBranchWS(NexusMessage &nm);
  DQErr parseIndirectBranchWS(NexusMessage &nm);
  DQErr parseSync(NexusMessage &nm);
  DQErr parseCorrelation(NexusMessage &nm);
  DQErr parseAuxAccessWrite(NexusMessage &nm);
  DQErr parseOwnershipTrace(NexusMessage &nm);
  DQErr parseError(NexusMessage &nm);
};

SliceFileParser::SliceFileParser(char *filename, bool binary)
{
	assert(filename != nullptr);

	numTraceMsgs   = 0;
	numSyncMsgs    = 0;
	msgSlices      = 0;
	bitIndex       = 0;
	currentAddress = 0;
	currentTime    = 0;

	this->binary = binary;

	if (binary) {
		tf.open(filename, ios::in | ios::binary);
	}
	else {
		tf.open(filename, ios::in);
	}

	if (!tf) {
		printf("Error: SliceFileParder(): could not open file %s for input",filename);
		status = DQERR_OPEN;
	}
	else {
		status = DQERR_OK;
	}
}

void SliceFileParser::dump()
{
	//msg and msgSlices

	for (int i = 0; i < msgSlices; i++) {
		cout << setw(2) << i+1;
		cout << " | ";
		cout << setfill('0');
		cout << setw(2) << hex << int(msg[i] >> 2) << dec;
		cout << setfill(' ');
		cout << " | ";
		cout << int((msg[i] >> 1) & 1);
		cout << " ";
		cout << int(msg[i] & 1);
		cout << endl;
	}
}

DQErr SliceFileParser::parseDirectBranch(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t i_cnt;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message has been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode     = TCODE_DIRECT_BRANCH;
	nm.directBranch.i_cnt     = (int)i_cnt;
	printf("direct branch i_cnt = %d\n",i_cnt);
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = timestamp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseDirectBranchWS(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t i_cnt;
	uint64_t sync;
	uint64_t f_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed length sync reason field

	rc = parseFixedField(4,&sync);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode                 = TCODE_DIRECT_BRANCH_WS;
	nm.directBranchWS.sync   = (SyncReason)sync;
	nm.directBranchWS.i_cnt  = i_cnt;
	nm.directBranchWS.f_addr = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseIndirectBranch(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t b_type;
	uint64_t i_cnt;
	uint64_t u_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed lenght b-type

	rc = parseFixedField(2,&b_type);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&u_addr);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}
		haveTimestamp = true;
	}

	nm.tcode                 = TCODE_INDIRECT_BRANCH;
	nm.indirectBranch.b_type = (BType)b_type;
	nm.indirectBranch.i_cnt  = (int)i_cnt;
	nm.indirectBranch.u_addr = u_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseIndirectBranchWS(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t sync;
	uint64_t b_type;
	uint64_t i_cnt;
	uint64_t f_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed length sync field

	rc = parseFixedField(4,&sync);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the fixed length b-type

	rc = parseFixedField(2,&b_type);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode                   = TCODE_INDIRECT_BRANCH_WS;
	nm.indirectBranchWS.sync   = (SyncReason)sync;
	nm.indirectBranchWS.b_type = (BType)b_type;
	nm.indirectBranchWS.i_cnt  = (int)i_cnt;
	nm.indirectBranchWS.f_addr = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseSync(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t i_cnt;
	uint64_t timestamp;
	uint64_t sync;
	uint64_t f_addr;
	bool     haveTimestamp;

	// parse the variable length the i-cnt

	rc = parseFixedField(4,&sync);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&i_cnt);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode         = TCODE_SYNC;
	nm.sync.sync     = (SyncReason)sync;
	nm.sync.i_cnt    = (int)i_cnt;
	nm.sync.f_addr   = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseCorrelation(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = TCODE_CORRELATION;

	// parse the 4-bit evcode field

	rc = parseFixedField(4,&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.correlation.evcode = tmp;

	// parse the 2-bit cdf field

	rc = parseFixedField(2,&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (tmp != 0) {
		printf("Error: DQErr SliceFileParser::parseCorrelation(): Expected EVCODE to be 0\n");

		status = DQERR_ERR;

		return status;
	}

	nm.correlation.cdf = tmp;

	// parse the variable length i-cnt field

	rc = parseVarField(&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.correlation.i_cnt = (int)tmp;

	if (eom == true) {
		haveTimestamp = false;
		tmp = 0;
	}
	else {
		rc = parseVarField(&tmp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseError(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = TCODE_ERROR;

	// parse the 4 bit ETYPE field

	rc = parseFixedField(4,&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.error.etype = (uint8_t)tmp;

	// parse the variable sized padd field

	rc = parseVarField(&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		tmp = 0;
	}
	else {
		rc = parseVarField(&tmp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseOwnershipTrace(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = TCODE_OWNERSHIP_TRACE;

	// parse the variable length process ID field

	rc = parseVarField(&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.ownership.process = (int)tmp;

	if (eom == true) {
		haveTimestamp = false;
		tmp = 0;
	}
	else {
		rc = parseVarField(&tmp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseAuxAccessWrite(NexusMessage &nm)
{
	DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = TCODE_AUXACCESS_WRITE;

	// parse the ADDR field

	rc = parseVarField(&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.auxAccessWrite.addr = (uint32_t)tmp;

	// parse the data field

	rc = parseVarField(&tmp);
	if (rc != DQERR_OK) {
		status = rc;

		return status;
	}

	nm.auxAccessWrite.data = (uint32_t)tmp;

	if (eom == true) {
		haveTimestamp = false;
		tmp = 0;
	}
	else {
		rc = parseVarField(&tmp); // this field is optional - check err
		if (rc != DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return DQERR_OK;
}

DQErr SliceFileParser::parseFixedField(int width, uint64_t *val)
{
	assert(width != 0);
	assert(val != nullptr);

	int i;
	int b;

	i = bitIndex / 6;
	b = bitIndex % 6;

//	printf("parseFixedField(): bitIndex:%d, i:%d, b:%d, width: %d\n",bitIndex,i,b,width);

	bitIndex += width;

	// for read error checking we should make sure that the MSEO bits are
	// correct for this field. But for now, we don't

	if (i >= msgSlices) {
		// read past end of message

		status = DQERR_EOM;

		return DQERR_EOM;
	}

	if (b+width > 6) {
		// don't support fixed width > 6 bits or that cross slice boundary

		status = DQERR_ERR;

		return DQERR_ERR;
	}

	uint8_t v;

	// strip off upper and lower bits not part of field

	v = msg[i] << (6-(b+width));
	v = v >> ((6-(b+width))+b+2);

	*val = uint64_t(v);

//	printf("-> bitIndex: %d, value: %x\n",bitIndex,v);

	if ((msg[i] & 0x03) == MSEO_END) {
		eom = true;
	}

	return DQERR_OK;
}

DQErr SliceFileParser::parseVarField(uint64_t *val)
{
	assert(val != nullptr);

	int i;
	int b;
	int width = 0;

	i = bitIndex / 6;
	b = bitIndex % 6;

	if (i >= msgSlices) {
		// read past end of message

		status = DQERR_EOM;

		return DQERR_EOM;
	}

	uint64_t v;

	// strip off upper and lower bits not part of field

	width = 6-b;

//	printf("parseVarField(): bitIndex:%d, i:%d, b:%d, width: %d\n",bitIndex,i,b,width);

	v = msg[i] >> (b+2);

	while ((msg[i] & 0x03) == MSEO_NORMAL) {
		i += 1;
		if (i >= msgSlices) {
			// read past end of message

			status = DQERR_ERR;

			return DQERR_ERR;
		}

		v = v | ((msg[i] >> 2) << width);
		width += 6;
	}

	if ((msg[i] & 0x03) == MSEO_END) {
		eom = true;
	}

	bitIndex += width;
	*val = v;

//	printf("-> bitIndex: %d, value: %llx\n",bitIndex,v);

	return DQERR_OK;
}

DQErr SliceFileParser::readBinaryMsg()
{
	do {
		tf.read((char*)&msg[0],sizeof msg[0]);
		if (!tf) {
			status = DQERR_EOF;

			return DQERR_EOF;
		}
	} while ((msg[0] & 0x3) == MSEO_END);

	// make sure this is start of nexus message

	if ((msg[0] & 0x3) != MSEO_NORMAL) {
		status = DQERR_ERR;

		cout << "Error: SliceFileParser::readBinaryMsg(): expected start of message; got" << hex << static_cast<uint8_t>(msg[0] & 0x3) << dec << endl;

		return DQERR_ERR;
	}

	bool done = false;

	for (int i = 1; !done; i++) {
		if (i >= (int)(sizeof msg / sizeof msg[0])) {
			cout << "Error: SliceFileParser::readBinaryMsg(): msg buffer overflow" << endl;

			status = DQERR_ERR;

			return DQERR_ERR;
		}

		tf.read((char*)&msg[i],sizeof msg[0]);
		if (!tf) {
			status = DQERR_ERR;

			cout << "error reading stream\n";

			return DQERR_ERR;
		}

		if ((msg[i] & 0x03) == MSEO_END) {
			done = true;
			msgSlices = i+1;
		}
	}

	eom = false;

	bitIndex = 0;

	return DQERR_OK;
}

int SliceFileParser::atoh(char a)
{
	if (a >= '0' && a <= '9') {
		return a-'0';
	}

	if (a >= 'a' && a <= 'f') {
		return a-'a'+10;
	}

	if (a >= 'A' && a <= 'F') {
		return a-'A'+10;
	}

	return -1;
}

DQErr SliceFileParser::readNextByte(uint8_t *byte)
{
	char c;

	// strip white space, comments, cr, and lf

	enum {
		STRIPPING_WHITESPACE,
		STRIPPING_COMMENT,
		STRIPPING_DONE
	} ss = STRIPPING_WHITESPACE;

	do {
		tf.read((char*)&c,sizeof c);
		if (!tf) {
			status = DQERR_EOF;

			return DQERR_EOF;
		}

		switch (ss) {
		case STRIPPING_WHITESPACE:
			switch (c) {
			case '#':
				ss = STRIPPING_COMMENT;
				break;
			case '\n':
			case '\r':
			case ' ':
			case '\t':
				break;
			default:
				ss = STRIPPING_DONE;
			}
			break;
		case STRIPPING_COMMENT:
			switch (c) {
			case '\n':
				ss = STRIPPING_WHITESPACE;
				break;
			}
			break;
		default:
			break;
		}
	} while (ss != STRIPPING_DONE);

	// now try to get two hex digits

	tf.read((char*)&c,sizeof c);
	if (!tf) {
		status = DQERR_EOF;

		return DQERR_EOF;
	}

	int hd;
	uint8_t hn;

	hd = atoh(c);
	if (hd < 0) {
		status = DQERR_ERR;

		return DQERR_ERR;
	}

	hn = hd << 4;

	hd = atoh(c);
	if (hd < 0) {
		status = DQERR_ERR;

		return DQERR_ERR;
	}

	hn = hn | hd;

	*byte = hn;

	return DQERR_OK;
}

DQErr SliceFileParser::readAscMsg()
{
	cout << "Error: SliceFileP{arser::readAscMsg(): not implemented" << endl;
	status = DQERR_ERR;

	return DQERR_ERR;

	// strip off idle bytes

	do {
		if (readNextByte(&msg[0]) != DQERR_OK) {
			return status;
		}
	} while ((msg[0] & 0x3) == MSEO_END);

	// make sure this is start of nexus message

	if ((msg[0] & 0x3) != MSEO_NORMAL) {
		status = DQERR_ERR;

		cout << "Error: SliceFileParser::readBinaryMsg(): expected start of message; got" << hex << static_cast<uint8_t>(msg[0] & 0x3) << dec << endl;

		return DQERR_ERR;
	}

	// now get bytes

	bool done = false;

	for (int i = 1; !done; i++) {
		if (i >= (int)(sizeof msg / sizeof msg[0])) {
			cout << "Error: SliceFileParser::readBinaryMsg(): msg buffer overflow" << endl;

			status = DQERR_ERR;

			return DQERR_ERR;
		}

		if (readNextByte(&msg[i]) != DQERR_OK) {
			return status;
		}

		if ((msg[i] & 0x03) == MSEO_END) {
			done = true;
			msgSlices = i+1;
		}
	}

	eom = false;

	bitIndex = 0;

	return DQERR_OK;
}

DQErr SliceFileParser::nextTraceMsg(NexusMessage &nm)	// generator to return trace messages one at a time
{

	assert(status == DQERR_OK);

	DQErr    rc;
	uint64_t val;
	uint8_t  tcode;

	// read from file, store in object, compute and fill out full fields, such as address and more later
	// need some place to put it. An object

	if (binary) {
		rc = readBinaryMsg();
		if (rc != DQERR_OK) {
			if (rc != DQERR_EOF) {
				cout << "Error: (): readBinaryMsg() returned error " << rc << "\n";
			}

			status = rc;

			return status;
		}
	}
	else {	// text trace messages
		rc = readAscMsg();
		if (rc != DQERR_OK) {
			if (rc != DQERR_EOF) {
				cout << "Error: (): readTxtMsg() returned error " << rc << "\n";
			}

			status = rc;

			return status;
		}
	}

// crow		dump();

	rc = parseFixedField(6, &val);
	if (rc != DQERR_OK) {
		cout << "Error: (): could not read tcode\n";

		status = rc;

		return status;
	}

	tcode = (uint8_t)val;

	switch (tcode) {
	case TCODE_DEBUG_STATUS:
		cout << "unsupported debug status trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_DEVICE_ID:
		cout << "unsupported device id trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_OWNERSHIP_TRACE:
		status = parseOwnershipTrace(nm);
		break;
	case TCODE_DIRECT_BRANCH:
//		cout << " | direct branch";
		status = parseDirectBranch(nm);
		break;
	case TCODE_INDIRECT_BRANCH:
//		cout << "| indirect branch";
		status = parseIndirectBranch(nm);
		break;
	case TCODE_DATA_WRITE:
		cout << "unsupported data write trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_DATA_READ:
		cout << "unsupported data read trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_DATA_ACQUISITION:
		cout << "unsupported data acquisition trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_ERROR:
		status = parseError(nm);
		break;
	case TCODE_SYNC:
//		cout << "| sync";
		status = parseSync(nm);
		break;
	case TCODE_CORRECTION:
		cout << "unsupported correction trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_DIRECT_BRANCH_WS:
//		cout << "| direct branch with sync\n";
		status = parseDirectBranchWS(nm);
		break;
	case TCODE_INDIRECT_BRANCH_WS:
//		cout << "| indirect branch with sync";
		status = parseIndirectBranchWS(nm);
		break;
	case TCODE_DATA_WRITE_WS:
		cout << "unsupported data write with sync trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_DATA_READ_WS:
		cout << "unsupported data read with sync trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_WATCHPOINT:
		cout << "unsupported watchpoint trace message\n";
		status = DQERR_ERR;
		break;
	case TCODE_CORRELATION:
		status = parseCorrelation(nm);
		break;
	case TCODE_AUXACCESS_WRITE:
		status = parseAuxAccessWrite(nm);
		break;
	default:
		cout << "Error: nextTraceMsg(): Unknown TCODE " << hex << tcode << dec << endl;
		status = DQERR_ERR;
	}

	return status;
}

static int asymbol_compare_func(const void *arg1,const void *arg2)
{
	assert(arg1 != nullptr);
	assert(arg2 != nullptr);

	asymbol *first;
	asymbol *second;

	first = *(asymbol **)arg1;
	second = *(asymbol **)arg2;

	// note: use bfd_asymbol_value() because first and second symnbols may not be in same section

	return bfd_asymbol_value((asymbol*)first) - bfd_asymbol_value((asymbol*)second);
}


__attribute__((unused)) static void dump_syms(asymbol **syms, int num_syms) // @suppress("Unused static function")
{
	assert(syms != nullptr);

    for (int i = 0; i < num_syms; i++) {
    	printf("%d: 0x%llx '%s'",i,bfd_asymbol_value(syms[i]),syms[i]->name);

        if (syms[i]->flags & BSF_LOCAL) {
          printf(" LOCAL");
        }

        if (syms[i]->flags & BSF_GLOBAL) {
          printf(" GLOBAL");
        }

        if (syms[i]->flags & BSF_EXPORT) {
          printf(" EXPORT");
        }

        if (syms[i]->flags & BSF_DEBUGGING) {
          printf(" DEBUGGING");
        }

        if (syms[i]->flags & BSF_FUNCTION) {
          printf(" FUNCTION");
        }

        if (syms[i]->flags & BSF_KEEP) {
          printf(" KEEP");
        }

        if (syms[i]->flags & BSF_ELF_COMMON) {
          printf(" ELF_COMMON");
        }

        if (syms[i]->flags & BSF_WEAK) {
          printf(" WEAK");
        }

        if (syms[i]->flags & BSF_SECTION_SYM) {
          printf(" SECTION_SYM");
        }

        if (syms[i]->flags & BSF_OLD_COMMON) {
          printf(" OLD_COMMON");
        }

        if (syms[i]->flags & BSF_NOT_AT_END) {
          printf(" NOT_AT_END");
        }

        if (syms[i]->flags & BSF_CONSTRUCTOR) {
          printf(" CONSTRUCTOR");
        }

        if (syms[i]->flags & BSF_WARNING) {
          printf(" WARNING");
        }

        if (syms[i]->flags & BSF_INDIRECT) {
          printf(" INDIRECT");
        }

        if (syms[i]->flags & BSF_FILE) {
          printf(" FILE");
        }

        if (syms[i]->flags & BSF_DYNAMIC) {
          printf(" DYNAMIC");
        }

        if (syms[i]->flags & BSF_OBJECT) {
          printf(" OBJECT");
        }

        if (syms[i]->flags & BSF_DEBUGGING_RELOC) {
          printf(" DEBUGGING_RELOC");
        }

        if (syms[i]->flags & BSF_THREAD_LOCAL) {
          printf(" THREAD_LOCAL");
        }

        if (syms[i]->flags & BSF_RELC) {
          printf(" RELC");
        }

        if (syms[i]->flags & BSF_SRELC) {
          printf(" SRELC");
        }

        if (syms[i]->flags & BSF_SYNTHETIC) {
          printf(" SYNTHETIC");
        }

        if (syms[i]->flags & BSF_GNU_INDIRECT_FUNCTION) {
          printf(" GNU_INDIRECT_FUNCTION");
        }

        printf("\n");
    }
}

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
	int   Disassemble(ADDRESS addr);

	int   getSrcLines(ADDRESS addr, const char **filename, const char **functionname, unsigned int *linenumber, const char **line);

	int   decodeInstructionSize(uint32_t inst, int &inst_size);
	int   decodeInstruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch);

	void  overridePrintAddress(bfd_vma addr, struct disassemble_info *info); // hmm.. don't need into - part of object!

	Instruction getInstructionInfo() { return instruction; }
	Source getSourceInfo() { return source; }

	DQErr getStatus() {return status;}

private:
	typedef struct {
		flagword sym_flags;
		bfd_vma func_vma;
		int func_size;
	} func_info_t;

	bfd *abfd;
	disassembler_ftype disassemble_func;
	DQErr status;

	bfd_vma start_address;
	long number_of_syms;
	asymbol **symbol_table;
	asymbol **sorted_syms;
	func_info_t *func_info;
	disassemble_info *info;
	section	    *codeSectionLst;
	int prev_index;
	int cached_sym_index;
	bfd_vma cached_sym_vma;
	int cached_sym_size;

	Instruction instruction;
	Source source;

	class fileReader *fileReader;

	const char *lastFileName;
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

static char *dis_output;

int stringify_callback(FILE *stream, const char *format, ...)
{
	char buffer[128];
	va_list vl;
	int rc;

	if (dis_output == nullptr) {
		return 0;
	}

	va_start(vl, format);
	rc = vsprintf(buffer, format, vl);
	va_end(vl);

	strcat(dis_output,buffer);

	return rc;
}

Disassembler::Disassembler(bfd *abfd)
{
	assert(abfd != nullptr);

	this->abfd = abfd;

    prev_index       = -1;
    cached_sym_index = -1;
    cached_sym_vma   = 0;
    cached_sym_size  = 0;

    start_address = bfd_get_start_address(abfd);

    long storage_needed;

    storage_needed = bfd_get_symtab_upper_bound(abfd);

//    printf("storage needed = %d\n",storage_needed);

    if (storage_needed > 0) {
    	symbol_table = (asymbol **)new (std::nothrow) char[storage_needed];

    	assert(symbol_table != nullptr);

    	number_of_syms = bfd_canonicalize_symtab(abfd,symbol_table);

//    	printf("symbol table @ %08x, num syms: %d\n",symbol_table,number_of_syms);

    	if (number_of_syms > 0) {
    		sorted_syms = new (std::nothrow) asymbol*[number_of_syms];

    		assert(sorted_syms != nullptr);

//    		printf("sorted syms @ %08x\n",sorted_syms);

    		for (int i = 0; i < number_of_syms; i++) {
    			sorted_syms[i] = symbol_table[i];
    		}

    		// could probably use C++ template sort here, but this works for now

//    		for (int i = 0; i < number_of_syms; i++) {
//    			printf("symbol[%d]:%s, addr:%08x, flags:%08x\n",i,sorted_syms[i]->name,bfd_asymbol_value(sorted_syms[i]),sorted_syms[i]->flags);
//    		}

    		// note: qsort does not preserver order on equal items!

    		qsort((void*)sorted_syms,(size_t)number_of_syms,sizeof sorted_syms[0],asymbol_compare_func);

    		func_info = new (std::nothrow) func_info_t[number_of_syms];

    		assert(func_info != nullptr);

    		// compute sizes of functions and data items for lookup by address

    		for (int i = 0; i < number_of_syms; i++) {
				func_info[i].sym_flags = sorted_syms[i]->flags;

				if ((sorted_syms[i]->flags & (BSF_FUNCTION | BSF_OBJECT)) != 0) {

    				// set func_vma to the base+offset address, not just the offest

    				func_info[i].func_vma = bfd_asymbol_value(sorted_syms[i]);

    				int j;

    				for (j = i+1; j < number_of_syms; j++) {
    					if ((sorted_syms[j]->flags & (BSF_FUNCTION | BSF_OBJECT)) != 0) {
    						func_info[i].func_size = bfd_asymbol_value(sorted_syms[j]) - func_info[i].func_vma;
    						break;
    					}
    				}

    				if (j >= number_of_syms) {
    					// size size of func(i) to end of section

    					asection *section;
    					bfd_vma base_vma;
    					section = sorted_syms[i]->section;
    					base_vma = section->vma;

    					func_info[i].func_size = (base_vma + section->size) - func_info[i].func_vma;
    				}
    			}
    			else {
    				func_info[i].func_size = 0;	// this probably isn't right
    			}

//				printf("symbol[%d]:%s, addr:%08x size:%d, flags:%08x\n",i,sorted_syms[i]->name,func_info[i].func_vma,func_info[i].func_size,sorted_syms[i]->flags);
    		}
    	}
    }
    else {
    	symbol_table = nullptr;
    	sorted_syms = nullptr;
    }

    codeSectionLst = nullptr;

    for (asection *p = abfd->sections; p != NULL; p = p->next) {
  	  if (p->flags & SEC_CODE) {
            // found a code section, add to list

  		  section *sp = new section;
  		  if (sp->initSection(&codeSectionLst, p) == nullptr) {
  			  status = DQERR_ERR;
  			  return;
  		  }
  	  }
    }

    if (codeSectionLst == nullptr) {
    	printf("Error: no code sections found\n");

    	status = DQERR_ERR;
    	return;
    }

   	info = new (std::nothrow) disassemble_info;

   	assert(info != nullptr);

   	disassemble_func = disassembler(bfd_get_arch(abfd), bfd_big_endian(abfd),bfd_get_mach(abfd), abfd);
   	if (disassemble_func == nullptr) {
   		printf("Error: Disassmbler::Disassembler(): could not create disassembler\n");

   		delete [] info;
   		info = nullptr;

   		status = DQERR_ERR;
   		return;
   	}

   	init_disassemble_info(info,stdout,(fprintf_ftype)stringify_callback);

   	info->print_address_func = override_print_address;
   	info->arch = bfd_get_arch(abfd);
   	info->mach = bfd_get_mach(abfd);

   	info->buffer_vma = codeSectionLst->startAddr;
   	info->buffer_length = codeSectionLst->size;
   	info->section = codeSectionLst->asecptr;
   	info->buffer = (bfd_byte*)codeSectionLst->code;

//   	info->buffer_vma = text->vma;
//   	info->buffer_length = text->size;
//   	info->section = text;
//   	bfd_malloc_and_get_section(abfd,text,&info->buffer);

   	info->application_data = (void*)this;

   	disassemble_init_for_target(info);

   	fileReader = new class fileReader();

   	lastFileName = nullptr;
   	lastLineNumber = -1;

    status = DQERR_OK;
}

int Disassembler::lookupInstructionByAddress(bfd_vma vma,uint32_t *ins,int *ins_size)
{
	assert(ins != nullptr);

	uint32_t inst;
	int size;
	int rc;

	// need to support multiple code sections and do some error checking on the address!

	if (codeSectionLst == nullptr) {
		status = DQERR_ERR;

		return 1;
	}

	section *sp;

	sp = codeSectionLst->getSectionByAddress((ADDRESS)vma);

	if (sp == nullptr) {
		status = DQERR_ERR;

		return 1;
	}

	inst = (uint32_t)sp->code[(vma - sp->startAddr)/2];

	rc = decodeInstructionSize(inst, size);
	if (rc != 0) {
		status = DQERR_ERR;

		return rc;
	}

	*ins_size = size;

	if (size == 16) {
//    printf("instruction: %04x\n",inst);
		*ins = inst;
	}
	else {
		*ins = (((uint32_t)sp->code[(vma - sp->startAddr)/2+1]) << 16) | inst;

//    printf("instruction: %08x\n",*ins);
	}

	return 0;
}

int Disassembler::lookup_symbol_by_address(bfd_vma vma,flagword flags,int *index,int *offset)
{
	assert(index != nullptr);
	assert(offset != nullptr);

	// find the function closest to the address. Address should either be start of function, or in body
	// of function.

	if ( vma == 0) {
		return 0;
	}

	//asymbol **syms;
	int i;

	// check for a cache hit

	if (cached_sym_index != -1) {
		if ((vma >= cached_sym_vma) && (vma < (cached_sym_vma + cached_sym_size))) {
			*index = cached_sym_index;
			*offset = vma - cached_sym_vma;

			return 1;
		}
	}

	//syms = sorted_syms;

	for (i = 0; i < number_of_syms; i++) {
		if ((func_info[i].sym_flags & (BSF_FUNCTION | BSF_OBJECT)) != 0) {

			// note: func_vma already has the base+offset address in it

			if (vma == func_info[i].func_vma) {
				// exact match on vma. Make sure function isn't zero sized
				// if it is, try to find a following one at the same address

//				printf("\n->vma:%08x size: %d, i=%d, name: %s\n",vma,func_info[i].func_size,i,sorted_syms[i]->name);

				for (int j = i+1; j < number_of_syms; j++) {
					if ((func_info[j].sym_flags & (BSF_FUNCTION | BSF_OBJECT)) != 0) {
						if (func_info[i].func_vma == func_info[j].func_vma) {
							i = j;
						}
					}
				}

				// have a match with a function

				//printf("have match. Index %d\n",i);

				// cache it for re-lookup speed improvement

				cached_sym_index = i;
				cached_sym_vma = func_info[i].func_vma;
				cached_sym_size = func_info[i].func_size;

				*index = i;
				*offset = vma - cached_sym_vma;

				return 1;
			}
			else if (vma > func_info[i].func_vma) {
				if (vma < (func_info[i].func_vma + func_info[i].func_size)) {

					cached_sym_index = i;
					cached_sym_vma = func_info[i].func_vma;
					cached_sym_size = func_info[i].func_size;

					*index = i;
					*offset = vma - cached_sym_vma;

					return 1;
				}
			}
		}
	}

	return 0;
}

void Disassembler::overridePrintAddress(bfd_vma addr, struct disassemble_info *info)
{
	assert(info != nullptr);

	//	lookup symbol at addr.

	// use field in info to point to disassembler object so we can call member funcs

	if (info != this->info) {
		printf("Error: overridePrintAddress(): info does not match\n");
	}

	instruction.operandAddress = addr;
	instruction.haveOperandAddress = true;

	int index;
	int offset;
	int rc;

	rc = lookup_symbol_by_address(addr,BSF_FUNCTION,&index,&offset);

	if (rc != 0) {

		// found symbol

		instruction.operandLabel = sorted_syms[index]->name;
		instruction.operandLabelOffset = offset;

//		if (offset == 0) {
//			printf("%08lx <%s>",addr,sorted_syms[index]->name);
//		}
//		else {
//			printf("%08lx <%s+%x>",addr,sorted_syms[index]->name,offset);
//		}
	}
	else {
//		printf("%08lx",addr);

		instruction.operandLabel = nullptr;
		instruction.operandLabelOffset = 0;
	}
}


void Disassembler::print_address(bfd_vma vma)
{
	// find closest preceeding function symbol and print with offset

	int index;
	int offset;
	int rc;

	rc = lookup_symbol_by_address(vma,BSF_FUNCTION,&index,&offset);
	if (rc != 0) {
		// found symbol

	    if (prev_index != index) {
	    	prev_index = index;

//	    	printf("\n");

	    	if (offset == 0) {
	    		printf("%08lx <%s>:\n",vma,sorted_syms[index]->name);
	    	}
	    	else {
	    		printf("%08lx <%s+%x>:\n",vma,sorted_syms[index]->name,offset);
	    	}
	    }

	    printf("%8lx: ",vma);
	}
	else {
		// didn't find symbol

		prev_index = -1;

		printf("%8lx:",vma);
	}
}

void Disassembler::print_address_and_instruction(bfd_vma vma)
{
	uint32_t ins = 0;
	int ins_size = 0;

	print_address(vma);

	lookupInstructionByAddress(vma,&ins,&ins_size);

	if (ins_size > 16) {
		 printf("       %08x                ",ins);
	}
	else {
		 printf("       %04x                    ",ins);
	}
}

void Disassembler::setInstructionAddress(bfd_vma vma)
{
	instruction.address = vma;

	// find closest preceeding function symbol and print with offset

	int index;
	int offset;
	int rc;

	lookupInstructionByAddress(vma,&instruction.instruction,&instruction.instSize);

	rc = lookup_symbol_by_address(vma,BSF_FUNCTION,&index,&offset);
	if (rc == 0) {
		// did not find symbol

		instruction.addressLabel = nullptr;
		instruction.addressLabelOffset = 0;
	}
	else {

		// found symbol

		instruction.addressLabel = sorted_syms[index]->name;
		instruction.addressLabelOffset = offset;
	}
}

int Disassembler::decodeInstructionSize(uint32_t inst, int &inst_size)
{
	switch (inst & 0x0003) {
	case 0x0000:	// quadrant 0, compressed
		inst_size = 16;
		return 0;
	case 0x0001:	// quadrant 1, compressed
		inst_size = 16;
		return 0;
	case 0x0002:	// quadrant 2, compressed
		inst_size = 16;
		return 0;
	case 0x0003:	// not compressed. Assume RV32 for now
		if ((inst & 0x1f) == 0x1f) {
			fprintf(stderr,"Error: decode_instruction(): cann't decode instructions longer than 32 bits\n");
			return 1;
		}

		inst_size = 32;
		return 0;
	}

	// error return

	return 1;
}

#define MOVE_BIT(bits,s,d)	((bits&(1<<s))?(1<<d):0)

int Disassembler::decodeRV32Q0Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	// no branch or jump instruction in quadrant 0

	inst_size = 16;
	is_branch = false;

	if ((instruction & 0x0003) != 0x0000) {
		return 1;
	}

	inst_type = UNKNOWN;

	return 0;
}

int Disassembler::decodeRV32Q1Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	// Q1 compressed instruction

	uint32_t t;

	inst_size = 16;

	switch (instruction >> 13) {
	case 0x1:
		inst_type = C_JAL;
		is_branch = true;

		t = MOVE_BIT(instruction,3,1);
		t |= MOVE_BIT(instruction,4,2);
		t |= MOVE_BIT(instruction,5,3);
		t |= MOVE_BIT(instruction,11,4);
		t |= MOVE_BIT(instruction,2,5);
		t |= MOVE_BIT(instruction,7,6);
		t |= MOVE_BIT(instruction,6,7);
		t |= MOVE_BIT(instruction,9,8);
		t |= MOVE_BIT(instruction,10,9);
		t |= MOVE_BIT(instruction,8,10);
		t |= MOVE_BIT(instruction,12,11);

		if (t & (1<<11)) { // sign extend offset
			t |= 0xfffff000;
		}

		immeadiate = t;
		break;
	case 0x5:
		inst_type = C_J;
		is_branch = true;

		t = MOVE_BIT(instruction,3,1);
		t |= MOVE_BIT(instruction,4,2);
		t |= MOVE_BIT(instruction,5,3);
		t |= MOVE_BIT(instruction,11,4);
		t |= MOVE_BIT(instruction,2,5);
		t |= MOVE_BIT(instruction,7,6);
		t |= MOVE_BIT(instruction,6,7);
		t |= MOVE_BIT(instruction,9,8);
		t |= MOVE_BIT(instruction,10,9);
		t |= MOVE_BIT(instruction,8,10);
		t |= MOVE_BIT(instruction,12,11);

		if (t & (1<<11)) { // sign extend offset
			t |= 0xfffff000;
		}

		immeadiate = t;
		break;
	case 0x6:
		inst_type = C_BEQZ;
		is_branch = true;

		t = MOVE_BIT(instruction,3,1);
		t |= MOVE_BIT(instruction,4,2);
		t |= MOVE_BIT(instruction,10,3);
		t |= MOVE_BIT(instruction,11,4);
		t |= MOVE_BIT(instruction,2,5);
		t |= MOVE_BIT(instruction,5,6);
		t |= MOVE_BIT(instruction,6,7);
		t |= MOVE_BIT(instruction,12,8);

		if (t & (1<<8)) { // sign extend offset
			t |= 0xfffffe00;
		}

		immeadiate = t;
		break;
	case 0x7:
		inst_type = C_BNEZ;
		is_branch = true;

		t = MOVE_BIT(instruction,3,1);
		t |= MOVE_BIT(instruction,4,2);
		t |= MOVE_BIT(instruction,10,3);
		t |= MOVE_BIT(instruction,11,4);
		t |= MOVE_BIT(instruction,2,5);
		t |= MOVE_BIT(instruction,5,6);
		t |= MOVE_BIT(instruction,6,7);
		t |= MOVE_BIT(instruction,12,8);

		if (t & (1<<8)) { // sign extend offset
			t |= 0xfffffe00;
		}

		immeadiate = t;
		break;
	default:
		inst_type = UNKNOWN;
		is_branch = 0;
		break;
	}

	return 0;
}

int Disassembler::decodeRV32Q2Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	// Q1 compressed instruction

	inst_size = 16;

	inst_type = UNKNOWN;
	is_branch = false;

	switch (instruction >> 13) {
	case 0x4:
		if (instruction & (1<<12)) {
			if ((instruction & 0x007c) == 0x0000) {
				if ((instruction & 0x0f80) != 0x0000) {
					inst_type = C_JALR;
					is_branch = true;
					immeadiate = 0;
				}
			}
		}
		else {
			if ((instruction & 0x007c) == 0x0000) {
				if ((instruction & 0x0f80) != 0x0000) {
					inst_type = C_JR;
					is_branch = true;
					immeadiate = 0;
				}
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

int Disassembler::decodeRV32Instruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	if ((instruction & 0x1f) == 0x1f) {
		fprintf(stderr,"Error: decodeBranch(): cann't decode instructions longer than 32 bits\n");
		return 1;
	}

	uint32_t t;

	inst_size = 32;

	switch (instruction & 0x7f) {
	case 0x6f:
		inst_type = JAL;
		is_branch = true;

		t = MOVE_BIT(instruction,21,1);
		t |= MOVE_BIT(instruction,22,2);
		t |= MOVE_BIT(instruction,23,3);
		t |= MOVE_BIT(instruction,24,4);
		t |= MOVE_BIT(instruction,25,5);
		t |= MOVE_BIT(instruction,26,6);
		t |= MOVE_BIT(instruction,27,7);
		t |= MOVE_BIT(instruction,28,8);
		t |= MOVE_BIT(instruction,29,9);
		t |= MOVE_BIT(instruction,30,10);
		t |= MOVE_BIT(instruction,20,11);
		t |= MOVE_BIT(instruction,12,12);
		t |= MOVE_BIT(instruction,13,13);
		t |= MOVE_BIT(instruction,14,14);
		t |= MOVE_BIT(instruction,15,15);
		t |= MOVE_BIT(instruction,16,16);
		t |= MOVE_BIT(instruction,17,17);
		t |= MOVE_BIT(instruction,18,18);
		t |= MOVE_BIT(instruction,19,19);
		t |= MOVE_BIT(instruction,31,20);

		if (t & (1<<20)) { // sign extend offset
			t |= 0xffe00000;
		}

		immeadiate = t;
		break;
	case 0x67:
		if ((instruction & 0x7000) == 0x000) {
			inst_type = JALR;
			is_branch = true;

			t = instruction >> 20;

			if (t & (1<<11)) { // sign extend offset
				t |= 0xfffff000;
			}

			immeadiate = t;
		}
		else {
			inst_type = UNKNOWN;
			is_branch = false;
		}
		break;
	case 0x63:
		switch ((instruction >> 12) & 0x7) {
		case 0x0:
			inst_type = BEQ;
			break;
		case 0x1:
			inst_type = BNE;
			break;
		case 0x4:
			inst_type = BLT;
			break;
		case 0x5:
			inst_type = BGE;
			break;
		case 0x6:
			inst_type = BLTU;
			break;
		case 0x7:
			inst_type = BGEU;
			break;
		default:
			inst_type = UNKNOWN;
			is_branch = false;
			return 0;
		}

		is_branch = true;

		t = MOVE_BIT(instruction,8,1);
		t |= MOVE_BIT(instruction,9,2);
		t |= MOVE_BIT(instruction,10,3);
		t |= MOVE_BIT(instruction,11,4);
		t |= MOVE_BIT(instruction,25,5);
		t |= MOVE_BIT(instruction,26,6);
		t |= MOVE_BIT(instruction,27,7);
		t |= MOVE_BIT(instruction,28,8);
		t |= MOVE_BIT(instruction,29,9);
		t |= MOVE_BIT(instruction,30,10);
		t |= MOVE_BIT(instruction,7,11);
		t |= MOVE_BIT(instruction,31,12);

		if (t & (1<<12)) { // sign extend offset
			t |= 0xfffffe00;
		}

		immeadiate = t;
		break;
	}

	return 0;
}

int Disassembler::decodeInstruction(uint32_t instruction,int &inst_size,instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	int rc;

	switch (instruction & 0x0003) {
	case 0x0000:	// quadrant 0, compressed
		rc = decodeRV32Q0Instruction(instruction,inst_size,inst_type,immeadiate,is_branch);
		break;
	case 0x0001:	// quadrant 1, compressed
		rc = decodeRV32Q1Instruction(instruction,inst_size,inst_type,immeadiate,is_branch);
		break;
	case 0x0002:	// quadrant 2, compressed
		rc = decodeRV32Q2Instruction(instruction,inst_size,inst_type,immeadiate,is_branch);
		break;
	case 0x0003:	// not compressed. Assume RV32 for now
		rc = decodeRV32Instruction(instruction,inst_size,inst_type,immeadiate,is_branch);
		break;
	}

	return rc;
}

int Disassembler::getSrcLines(ADDRESS addr, const char **filename, const char **functionname, unsigned int *linenumber, const char **lineptr)
{
	const char *file;
	const char *function;
	unsigned int line;
	unsigned int discrim;

	// need to loop through all sections with code below and try to find one that succeeds

	section *sp;

	if (codeSectionLst == nullptr) {
		return 0;
	}

	sp = codeSectionLst->getSectionByAddress(addr);

	if (sp == nullptr) {
		return 0;
	}

	if (bfd_find_nearest_line_discriminator(abfd,sp->asecptr,symbol_table,addr-sp->startAddr,&file,&function,&line,&discrim) == 0) {
		return 0;
	}

	class fileReader::fileList *fl;

	*filename = file;
	*functionname = function;
	*linenumber = line;

	if (file == nullptr) {
		return 0;
	}

	fl = fileReader->findFile(file);
	if (fl == nullptr) {
		*lineptr = nullptr;
	}
	else {
		*lineptr = fl->lines[line-1];
	}

	return 1;
}

int Disassembler::Disassemble(ADDRESS addr)
{
	assert(disassemble_func != nullptr);

	bfd_vma vma;
	vma = (bfd_vma)addr;

	getSrcLines(addr, &source.sourceFile, &source.sourceFunction, &source.sourceLineNum, &source.sourceLine);

	setInstructionAddress(vma);

	int rc;

	instruction.instructionText[0] = 0;
	dis_output = instruction.instructionText;

	instruction.haveOperandAddress = false;

	// before calling disassemble_func, need to update info struct to point to correct section!

	if (codeSectionLst == nullptr) {
		return 1;
	}

	section *sp = codeSectionLst->getSectionByAddress(addr);

	if (sp == nullptr) {
		return 1;
	}

   	info->buffer_vma = sp->startAddr;
   	info->buffer_length = sp->size;
   	info->section = sp->asecptr;

   	// potential memory leak below the first time this is done because buffer was initially allocated by bfd

   	info->buffer = (bfd_byte*)sp->code;

	rc = disassemble_func(vma,info);

	// output from disassemble_func is in instruction.instrucitonText

	return rc;
}

struct NexusMessageSync {
	NexusMessageSync();
	int firstMsgNum;
	int lastMsgNum;
	int index;
	NexusMessage msgs[256];
};

NexusMessageSync::NexusMessageSync()
{
	firstMsgNum = 0;
	lastMsgNum  = 0;
	index = 0;
}

class Trace {
public:
	enum SymFlags {
		SYMFLAGS_NONE = 0,
		SYMFLAGS_xx = 1 << 0,
	};
	Trace(char *tf_name, bool binaryFlag, char *ef_name, SymFlags sym_flags);
	~Trace();
	DQErr setTraceRange(int start_msg_num,int stop_msg_num);

	enum traceFlags {
		TF_INSTRUCTION = 0x01,
		TF_ADDRESS     = 0x02,
		TF_DISSASEMBLE = 0x04,
		TF_TIMESTAMP   = 0x08,
		TF_TRACEINFO   = 0x10,
	};
	DQErr getStatus() { return status; }
	DQErr NextInstruction(Instruction **instInfo, NexusMessage **msgInfo, Source **srcInfo);

	const char *getSymbolByAddress(ADDRESS addr) { return symtab->getSymbolByAddress(addr); }
	const char *getNextSymbolByAddress() { return symtab->getNextSymbolByAddress(); }
	int Disassemble(ADDRESS addr);

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

	DQErr            status;
	SliceFileParser *sfp;
	ElfReader       *elfReader;
	Symtab          *symtab;
	Disassembler    *disassembler;
	SymFlags		 symflags;
	ADDRESS          currentAddress;
	ADDRESS			 lastFaddr;
	uint64_t	     lastTime;
	enum state       state;

	int              startMessageNum;
	int              endMessageNum;

//	need current message number and list of messages??

	NexusMessage     nm;

	NexusMessage     messageInfo;
	Instruction      instructionInfo;
	Source           sourceInfo;

	//	or maybe have this stuff in the nexus messages??

	int              i_cnt;

	uint32_t               inst = -1;
	int                    inst_size = -1;
	Disassembler::instType inst_type = Disassembler::instType::UNKNOWN;
	int32_t                immeadiate = -1;
	bool                   is_branch = false;

	NexusMessageSync      *messageSync;

	int decodeInstructionSize(uint32_t inst, int &inst_size);
	int decodeInstruction(uint32_t instruction,int &inst_size,Disassembler::instType &inst_type,int32_t &immeadiate,bool &is_branch);

	uint64_t computeAddress();
};

int Trace::decodeInstructionSize(uint32_t inst, int &inst_size)
{
  return disassembler->decodeInstructionSize(inst,inst_size);
}

int Trace::decodeInstruction(uint32_t instruction,int &inst_size,Disassembler::instType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	return disassembler->decodeInstruction(instruction,inst_size,inst_type,immeadiate,is_branch);
}

Trace::Trace(char *tf_name, bool binaryFlag, char *ef_name, SymFlags sym_flags)
{
  sfp          = nullptr;
  elfReader    = nullptr;
  symtab       = nullptr;
  disassembler = nullptr;

  assert(tf_name != nullptr);

  sfp = new (std::nothrow) SliceFileParser(tf_name,binaryFlag);

  assert(sfp != nullptr);

  if (sfp->getErr() != DQERR_OK) {
	printf("Error: cannot open trace file '%s' for input\n",tf_name);
	delete sfp;
	sfp = nullptr;

	status = DQERR_ERR;

	return;
  }

  if (ef_name != nullptr ) {
	// create elf object

//    printf("ef_name:%s\n",ef_name);

     elfReader = new (std::nothrow) ElfReader(ef_name);

    assert(elfReader != nullptr);

    if (elfReader->getStatus() != DQERR_OK) {
    	if (sfp != nullptr) {
    		delete sfp;
    		sfp = nullptr;
    	}

    	delete elfReader;
    	elfReader = nullptr;

    	status = DQERR_ERR;

    	return;
    }

    // create disassembler object

    bfd *abfd;
    abfd = elfReader->get_bfd();

	disassembler = new (std::nothrow) Disassembler(abfd);

	assert(disassembler != nullptr);

	if (disassembler->getStatus() != DQERR_OK) {
		if (sfp != nullptr) {
			delete sfp;
			sfp = nullptr;
		}

		if (elfReader != nullptr) {
			delete elfReader;
			elfReader = nullptr;
		}

		delete disassembler;
		disassembler = nullptr;

		status = DQERR_ERR;

		return;
	}

	symflags = sym_flags;

    // get symbol table

    symtab = elfReader->getSymtab();
    if (symtab == nullptr) {
    	delete elfReader;
    	elfReader = nullptr;

    	delete sfp;
    	sfp = nullptr;

    	status = DQERR_ERR;

    	return;
    }
  }

  lastFaddr      = 0;
  currentAddress = 0;
  state          = TRACE_STATE_GETFIRSTYNCMSG;
  i_cnt          = 0;

  lastTime = 0;

  startMessageNum  = 0;
  endMessageNum    = 0;

  messageSync      = nullptr;

  instructionInfo.address = 0;
  instructionInfo.instruction = 0;
  instructionInfo.instSize = 0;
  instructionInfo.addressLabel = nullptr;
  instructionInfo.addressLabelOffset = 0;
  instructionInfo.haveOperandAddress = false;
  instructionInfo.operandAddress = 0;
  instructionInfo.operandLabel = nullptr;
  instructionInfo.operandLabelOffset = 0;

  sourceInfo.sourceFile = nullptr;
  sourceInfo.sourceFunction = nullptr;
  sourceInfo.sourceLineNum = 0;
  sourceInfo.sourceLine = nullptr;

  status = DQERR_OK;
}

Trace::~Trace()
{
	if (sfp != nullptr) {
		delete sfp;
		sfp = nullptr;
	}

	if (elfReader != nullptr) {
		delete elfReader;
		elfReader = nullptr;
	}
}

DQErr Trace::setTraceRange(int start_msg_num,int stop_msg_num)
{
	if (start_msg_num < 0) {
		status = DQERR_ERR;
		return DQERR_ERR;
	}

	if (stop_msg_num < 0) {
		status = DQERR_ERR;
		return DQERR_ERR;
	}

	if ((stop_msg_num != 0) && (start_msg_num > stop_msg_num)) {
		status = DQERR_ERR;
		return DQERR_ERR;
	}

	startMessageNum = start_msg_num;
	endMessageNum = stop_msg_num;

	state = TRACE_STATE_GETSTARTTRACEMSG;

	return DQERR_OK;
}

uint64_t Trace::computeAddress()
{
	switch (nm.tcode) {
	case TCODE_DEBUG_STATUS:
		break;
	case TCODE_DEVICE_ID:
		break;
	case TCODE_OWNERSHIP_TRACE:
		break;
	case TCODE_DIRECT_BRANCH:
//		currentAddress = target of branch.
		break;
	case TCODE_INDIRECT_BRANCH:
		currentAddress = currentAddress ^ (nm.indirectBranch.u_addr << 1);	// note - this is the next address!
		break;
	case TCODE_DATA_WRITE:
		break;
	case TCODE_DATA_READ:
		break;
	case TCODE_DATA_ACQUISITION:
		break;
	case TCODE_ERROR:
		break;
	case TCODE_SYNC:
		currentAddress = nm.sync.f_addr << 1;
		break;
	case TCODE_CORRECTION:
		break;
	case TCODE_DIRECT_BRANCH_WS:
		currentAddress = nm.directBranchWS.f_addr << 1;
		break;
	case TCODE_INDIRECT_BRANCH_WS:
		currentAddress = nm.indirectBranchWS.f_addr << 1;
		break;
	case TCODE_DATA_WRITE_WS:
		break;
	case TCODE_DATA_READ_WS:
		break;
	case TCODE_WATCHPOINT:
		break;
	case TCODE_CORRELATION:
		break;
	default:
		break;
	}

	cout << "New address 0x" << hex << currentAddress << dec << endl;

	return currentAddress;
}

int Trace::Disassemble(ADDRESS addr)
{
	assert(disassembler != nullptr);

	int   rc;
	DQErr s;

	rc = disassembler->Disassemble(addr);

	s = disassembler->getStatus();

	if (s != DQERR_OK ) {
	  status = s;
	  return 0;
	}

	instructionInfo = disassembler->getInstructionInfo();
	sourceInfo = disassembler->getSourceInfo();

	return rc;
}

//NextInstruction() want to return address, instruction, trace message if any, label+offset for instruction, target of instruciton
//		source code for instruction (file, function, line)
//
//		return instruction object (include label informatioon)
//		return message object
//		return source code object//
//
//				if instruction object ptr is null, don't return any instruction info
//				if message object ptr is null, don't return any message info
//				if source code object is null, don't return source code info

DQErr Trace::NextInstruction(Instruction **instInfo, NexusMessage **msgInfo, Source **srcInfo)
{
	assert(sfp != nullptr);

	DQErr rc;

	if (instInfo != nullptr) {
		*instInfo = nullptr;
	}

	if (msgInfo != nullptr) {
		*msgInfo = nullptr;
	}

	if (srcInfo != nullptr) {
		*srcInfo = nullptr;
	}

	for (;;) {
		switch (state) {
		case TRACE_STATE_GETSTARTTRACEMSG:
//			printf("state TRACE_STATE_GETSTARTTRACEMSG\n");

			if (startMessageNum <= 1) {
				// if starting at beginning, just switch to normal state for starting

				state = TRACE_STATE_GETFIRSTYNCMSG;
				break;
			}

			rc = sfp->nextTraceMsg(nm);
			if (rc != DQERR_OK) {
				// have an error. either eof, or error

				status = sfp->getErr();

				if (status == DQERR_EOF) {
					if ( messageSync != nullptr) {
						printf("Error: Trace file does not contain %d trace messages. %d message found\n",startMessageNum,messageSync->lastMsgNum);
					}
					else {
						printf("Error: Trace file does not contain any trace messages, or is unreadable\n");
					}
				}

				return status;
			}

			if (messageSync == nullptr) {
				messageSync = new NexusMessageSync;
			}

			switch (nm.tcode) {
			case TCODE_SYNC:
			case TCODE_DIRECT_BRANCH_WS:
			case TCODE_INDIRECT_BRANCH_WS:
				messageSync->msgs[0] = nm;
				messageSync->index = 1;

				messageSync->firstMsgNum = nm.msgNum;
				messageSync->lastMsgNum = nm.msgNum;

				if (nm.msgNum >= startMessageNum) {
					state = TRACE_STATE_COMPUTESTARTINGADDRESS;
				}
				break;
			case TCODE_DIRECT_BRANCH:
			case TCODE_INDIRECT_BRANCH:
				if (messageSync->index == 0) {
					if (nm.msgNum >= startMessageNum) {
						// can't start at this trace message because we have not seen a sync yet
						// so we cannot compute the address

						state = TRACE_STATE_ERROR;

						printf("Error: cannot start at trace message %d because no preceeding sync\n",startMessageNum);

						status = DQERR_ERR;
						return status;
					}

					// first message. Not a sync, so ignore
				}
				else {
					// stuff it in the list

					messageSync->msgs[messageSync->index] = nm;
					messageSync->index += 1;

					// don't forget to check for messageSync->msgs[] overrun!!

					if (messageSync->index >= (int)(sizeof messageSync->msgs / sizeof messageSync->msgs[0])) {
						status = DQERR_ERR;
						state = TRACE_STATE_ERROR;

						return status;
					}

					messageSync->lastMsgNum = nm.msgNum;

					if (nm.msgNum >= startMessageNum) {
						state = TRACE_STATE_COMPUTESTARTINGADDRESS;
					}
				}
				break;
			case TCODE_CORRELATION:
				// we are leaving trace mode, so we no longer know address we are at until
				// we see a sync message, so set index to 0 to start over

				messageSync->index = 0;
				break;
			case TCODE_AUXACCESS_WRITE:
			case TCODE_OWNERSHIP_TRACE:
			case TCODE_ERROR:
				// these message types we just stuff in the list incase we are interested in the
				// information later

				messageSync->msgs[messageSync->index] = nm;
				messageSync->index += 1;

				// don't forget to check for messageSync->msgs[] overrun!!

				if (messageSync->index >= (int)(sizeof messageSync->msgs / sizeof messageSync->msgs[0])) {
					status = DQERR_ERR;
					state = TRACE_STATE_ERROR;

					return status;
				}

				messageSync->lastMsgNum = nm.msgNum;

				if (nm.msgNum >= startMessageNum) {
					state = TRACE_STATE_COMPUTESTARTINGADDRESS;
				}
				break;
			default:
				state = TRACE_STATE_ERROR;

				status = DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_COMPUTESTARTINGADDRESS:
			// printf("state TRACE_STATE_COMPUTSTARTINGADDRESS\n");

			// compute address from trace message queued up in messageSync->msgs

			for (int i = 0; i < messageSync->index; i++) {
				switch (messageSync->msgs[i].tcode) {
				case TCODE_DIRECT_BRANCH:
					// need to get the direct branch instruction so we can compute the next address.
					// Instruction should be at currentAddress - 1 or -2, which is the last known address)
					// plus the i-cnt in this trace message (i-cnt is the number of 16 byte blocks in this
					// span of insturctions between last trace message and this trace message) - 1 if the
					// last instruction in the block is 16 bits, or -2 if the last instruction is 32 bits.
					// The problem is we don't know, and checking the length bits in the last two
					// 16 bit words will not always work. So we just compute instruction sizes from
					// the last known address to this trace message to find the last instruciton.

					ADDRESS addr;

					addr = currentAddress;

					i_cnt = messageSync->msgs[i].directBranch.i_cnt;

					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = lastTime ^ messageSync->msgs[i].timestamp;
					}

					while (i_cnt > 0) {
						status = elfReader->getInstructionByAddress(addr,inst);
						if (status != DQERR_OK) {
							printf("Error: getInstructionByAddress failed\n");

							printf("Addr: %08x\n",addr);

							state = TRACE_STATE_ERROR;

							return status;
						}

						// figure out how big the instruction is

						int rc;

						rc = decodeInstructionSize(inst,inst_size);
						if (rc != 0) {
							printf("Error: Cann't decode size of instruction %04x\n",inst);

							state = TRACE_STATE_ERROR;

							status = DQERR_ERR;
							return status;
						}

						switch (inst_size) {
						case 16:
							i_cnt -= 1;
							if (i_cnt > 0) {
								addr += 2;
							}
							break;
						case 32:
							i_cnt -= 2;
							if (i_cnt > 0) {
								addr += 4;
							}
							break;
						default:
							printf("Error: unsupported instruction size: %d\n",inst_size);

							state = TRACE_STATE_ERROR;

							status = DQERR_ERR;
							return status;
						}
					}

					decodeInstruction(inst,inst_size,inst_type,immeadiate,is_branch);

					if (is_branch == false) {
						status = DQERR_ERR;

						state = TRACE_STATE_ERROR;

						return status;
					}

					switch (inst_type) {
					case Disassembler::JAL:
					case Disassembler::BEQ:
					case Disassembler::BNE:
					case Disassembler::BLT:
					case Disassembler::BGE:
					case Disassembler::BLTU:
					case Disassembler::BGEU:
					case Disassembler::C_J:
					case Disassembler::C_JAL:
					case Disassembler::C_BEQZ:
					case Disassembler::C_BNEZ:
						currentAddress = addr + immeadiate;
						break;
					default:
						printf("Error: bad instruction type in state TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH: %d\n",inst_type);

						state = TRACE_STATE_ERROR;
						status = DQERR_ERR;
						return status;
					}
					break;
				case TCODE_INDIRECT_BRANCH:
					lastFaddr = lastFaddr ^ (messageSync->msgs[i].indirectBranch.u_addr << 1);
					currentAddress = lastFaddr;
					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = lastTime ^ messageSync->msgs[i].timestamp;
					}
					break;
				case TCODE_SYNC:
					lastFaddr = messageSync->msgs[i].sync.f_addr << 1;
					currentAddress = lastFaddr;
					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = messageSync->msgs[i].timestamp;
					}
					break;
				case TCODE_DIRECT_BRANCH_WS:
					lastFaddr = messageSync->msgs[i].directBranchWS.f_addr << 1;
					currentAddress = lastFaddr;
					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = messageSync->msgs[i].timestamp;
					}
					break;
				case TCODE_INDIRECT_BRANCH_WS:
					lastFaddr = messageSync->msgs[i].indirectBranchWS.f_addr << 1;
					currentAddress = lastFaddr;
					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = messageSync->msgs[i].timestamp;
					}
					break;
				case TCODE_CORRELATION:
					printf("Error: should never see this!!\n");
					state = TRACE_STATE_ERROR;

					status = DQERR_ERR;
					return status;
				case TCODE_AUXACCESS_WRITE:
				case TCODE_OWNERSHIP_TRACE:
				case TCODE_ERROR:
					// just skip these for now. Later we will add additional support for them,
					// such as handling the error or setting the process ID from the message
					if (messageSync->msgs[i].haveTimestamp) {
						lastTime = lastTime ^ messageSync->msgs[i].timestamp;
					}
					break;
				default:
					state = TRACE_STATE_ERROR;

					status = DQERR_ERR;
					return status;
				}
			}

			state = TRACE_STATE_GETNEXTMSG;

			if ((msgInfo != nullptr) && (messageSync->index > 0)) {
				messageInfo = messageSync->msgs[messageSync->index-1];
				messageInfo.currentAddress = currentAddress;
				messageInfo.time = lastTime;
				*msgInfo = &messageInfo;

				status = DQERR_OK;
				return status;
			}

			break;
		case TRACE_STATE_GETFIRSTYNCMSG:
			// read trace messages until a sync is found. Should be the first message normally

			// only exit this state when sync type message is found

			rc = sfp->nextTraceMsg(nm);
			if (rc != DQERR_OK) {
				// have an error. either eof, or error

				status = sfp->getErr();
				return status;
			}

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state = TRACE_STATE_DONE;
				status = DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TCODE_SYNC:
				nm.sync.i_cnt = 0; // just making sure on first sync message
				lastFaddr = nm.sync.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}
				state = TRACE_STATE_GETSECONDMSG;
				break;
			case TCODE_DIRECT_BRANCH_WS:
				nm.directBranchWS.i_cnt = 0; // just making sure on first sync message
				lastFaddr = nm.directBranchWS.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}
				state = TRACE_STATE_GETSECONDMSG;
				break;
			case TCODE_INDIRECT_BRANCH_WS:
				nm.indirectBranchWS.i_cnt = 0; // just making sure on first sync message
				lastFaddr = nm.indirectBranchWS.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}
				state = TRACE_STATE_GETSECONDMSG;
				break;
			default:
				// if we haven't found a sync message yet, keep the state the same so it will
				// kepp looking for a sync. Could also set he process ID if seen and handle
				// any TCODE_ERROR message

				// until we find our first sync, we don't know the time (need a full timestamp)

				nm.timestamp = 0;
				lastTime = 0;
				break;
			}

			// here we return the trace messages before we have actually started tracing
			// this could be at the start of a trace, or after leaving a trace bec ause of
			// a correlation message

			if (msgInfo != nullptr) {
				messageInfo = nm;
				messageInfo.currentAddress = currentAddress;
				messageInfo.time = lastTime;
				*msgInfo = &messageInfo;
			}

			status = DQERR_OK;
			return status;
		case TRACE_STATE_GETSECONDMSG:

			// only message with i-cnt will release from this state

			// return any message without an i-cnt

			// do not return message with i-cnt; process them when i-cnt expires

			rc = sfp->nextTraceMsg(nm);
			if (rc != DQERR_OK) {
				// have either eof, or error

				status = sfp->getErr();
				return status;
			}

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state = TRACE_STATE_DONE;
				status = DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TCODE_SYNC:
				i_cnt = nm.sync.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_DIRECT_BRANCH_WS:
				i_cnt = nm.directBranchWS.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_INDIRECT_BRANCH_WS:
				i_cnt = nm.indirectBranchWS.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_DIRECT_BRANCH:
				i_cnt = nm.directBranch.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_INDIRECT_BRANCH:
				i_cnt = nm.indirectBranch.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_CORRELATION:
				i_cnt = nm.correlation.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_AUXACCESS_WRITE:
			case TCODE_OWNERSHIP_TRACE:
			case TCODE_ERROR:
				// these message have no address or i.cnt info, so we still need to get
				// another message.

				// might want to keep track of press, but will add that later

				// for now, return message;

				if (nm.haveTimestamp) {
					lastTime = lastTime ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;

					*msgInfo = &messageInfo;
				}

				return status;
			default:
				printf("Error: bad tcode type in sate TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH\n");

				state = TRACE_STATE_ERROR;
				status = DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_RETIREMESSAGE:

			// Process message being retired (currently in nm) i_cnt has gone to 0

			switch (nm.tcode) {
			case TCODE_SYNC:
				lastFaddr = nm.sync.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;

					*msgInfo = &messageInfo;
				}

				state = TRACE_STATE_GETNEXTMSG;
				break;
			case TCODE_DIRECT_BRANCH_WS:
				lastFaddr = nm.directBranchWS.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;

					*msgInfo = &messageInfo;
				}

				state = TRACE_STATE_GETNEXTMSG;
				break;
			case TCODE_INDIRECT_BRANCH_WS:
				lastFaddr = nm.indirectBranchWS.f_addr << 1;
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;
					*msgInfo = &messageInfo;
				}

				state = TRACE_STATE_GETNEXTMSG;
				break;
			case TCODE_DIRECT_BRANCH:
				// note: inst is set in the state before this state (TRACE_STATE_GETNEXTINSTRUCTION)
				// instr must be valid to get to this point, so use it for computing new address

				decodeInstruction(inst,inst_size,inst_type,immeadiate,is_branch);

				if (nm.haveTimestamp) {
					lastTime = lastTime ^ nm.timestamp;
				}

				switch (inst_type) {
				case Disassembler::JAL:
				case Disassembler::BEQ:
				case Disassembler::BNE:
				case Disassembler::BLT:
				case Disassembler::BGE:
				case Disassembler::BLTU:
				case Disassembler::BGEU:
				case Disassembler::C_J:
				case Disassembler::C_JAL:
				case Disassembler::C_BEQZ:
				case Disassembler::C_BNEZ:
					currentAddress = currentAddress + immeadiate;

					if (msgInfo != nullptr) {
						messageInfo = nm;
						messageInfo.time = lastTime;
						messageInfo.currentAddress = currentAddress;

						*msgInfo = &messageInfo;
					}

					break;
				default:
					printf("Error: bad instruction type in state TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH: %d\n",inst_type);

					state = TRACE_STATE_ERROR;
					status = DQERR_ERR;
					return status;
				}

				state = TRACE_STATE_GETNEXTMSG;
				break;
			case TCODE_INDIRECT_BRANCH:
				lastFaddr = lastFaddr ^ (nm.indirectBranch.u_addr << 1);
				currentAddress = lastFaddr;
				if (nm.haveTimestamp) {
					lastTime = lastTime ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;

					*msgInfo = &messageInfo;
				}

				state = TRACE_STATE_GETNEXTMSG;
				break;
			case TCODE_CORRELATION:

				// correlation has i_cnt, but no address info

				// leaving trace mode - need to get next sync

				if (nm.haveTimestamp) {
					lastTime = lastTime ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
//					messageInfo.currentAddress = currentAddress;
					messageInfo.currentAddress = lastFaddr + nm.correlation.i_cnt*2;

					*msgInfo = &messageInfo;
				}

				state = TRACE_STATE_GETFIRSTYNCMSG;
				break;
			case TCODE_AUXACCESS_WRITE:
			case TCODE_OWNERSHIP_TRACE:
			case TCODE_ERROR:
				// these messages have no address or i-cnt info and should have been
				// instantly retired when they were read.

				state = TRACE_STATE_ERROR;
				status = DQERR_ERR;
				return status;
			default:
				printf("Error: bad tcode type in sate TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH\n");

				state = TRACE_STATE_ERROR;
				status = DQERR_ERR;
				return status;
			}

			status = DQERR_OK;
			return status;
		case TRACE_STATE_GETNEXTMSG:

			// exit this state when message with i-cnt is read

			rc = sfp->nextTraceMsg(nm);
			if (rc != DQERR_OK) {
				// have either eof, or error

				status = sfp->getErr();
				return status;
			}

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state = TRACE_STATE_DONE;
				status = DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TCODE_DIRECT_BRANCH:
				i_cnt = nm.directBranch.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_INDIRECT_BRANCH:
				i_cnt = nm.indirectBranch.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_SYNC:
				i_cnt = nm.sync.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_DIRECT_BRANCH_WS:
				i_cnt = nm.directBranchWS.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_INDIRECT_BRANCH_WS:
				i_cnt = nm.indirectBranchWS.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_CORRELATION:
				i_cnt = nm.correlation.i_cnt;
				state = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TCODE_AUXACCESS_WRITE:
			case TCODE_OWNERSHIP_TRACE:
			case TCODE_ERROR:
				// retire these instantly by returning them through msgInfo

				if (nm.haveTimestamp) {
					lastTime = lastTime ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime;
					messageInfo.currentAddress = currentAddress;

					*msgInfo = &messageInfo;
				}

				// leave state along. Need to get another message with an i-cnt!

				return status;
			default:
				status = DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_GETNEXTINSTRUCTION:
			// printf("Trace::NextInstruction():TRACE_STATE_GETNEXTINSTRUCTION\n");

			// get instruction at addr

			ADDRESS addr;

			addr = currentAddress;

//		    where to put file object? Should above be part of file object? Make a section object??

//		    need to have a list of sections of code. lookup section based on address, use that section for symbol
//			and line number lookup.
//          also, read in file by line and keep an array of lines. Need to handle the #line and #file directrives
//			check if objdump follows #line and #file preprocessor directives (sure it will)

			status = elfReader->getInstructionByAddress(addr,inst);
			if (status != DQERR_OK) {
				printf("Error: getInstructionByAddress failed\n");

				printf("Addr2: %08x\n",addr);

				state = TRACE_STATE_ERROR;

				return status;
			}

			// figure out how big the instruction is

			int rc;

			rc = decodeInstructionSize(inst,inst_size);
			if (rc != 0) {
				printf("Error: Cann't decode size of instruction %04x\n",inst);
				status = DQERR_ERR;
				return status;
			}

			switch (inst_size) {
			case 16:
				i_cnt -= 1;
				if (i_cnt > 0) {
					currentAddress += 2;
				}
				break;
			case 32:
				i_cnt -= 2;
				if (i_cnt > 0) {
					currentAddress += 4;
				}
				break;
			default:
				printf("Error: unsupported instruction size: %d\n",inst_size);
				status = DQERR_ERR;
				return status;
			}

			// calling disassemble() fills in the instructionInfo object

			Disassemble(addr);

			if (instInfo != nullptr) {
				*instInfo = &instructionInfo;
			}

			if (srcInfo != nullptr) {
				*srcInfo = &sourceInfo;
			}

			if (i_cnt <= 0) {
				// update addr based on saved nexus message
				// get next nexus message

				state = TRACE_STATE_RETIREMESSAGE;

				// note: break below runs that state machine againa and does not fall through
				// to code below if ()

				break;
			}

			status = DQERR_OK;
			return status;
		case TRACE_STATE_DONE:
			status = DQERR_DONE;
			return status;
		case TRACE_STATE_ERROR:
//			printf("Trace::NextInstruction():TRACE_STATE_ERROR\n");

			status = DQERR_ERR;
			return status;
		default:
			printf("Trace::NextInstruction():unknown\n");

			state = TRACE_STATE_ERROR;
			status = DQERR_ERR;
			return status;
		}
	}

	status = DQERR_OK;
	return DQERR_OK;
}

static void override_print_address(bfd_vma addr, struct disassemble_info *info)
{
//	lookup symbol at addr.

  Disassembler *dp;

  // use field in info to point to disassembler object so we can call member funcs

//  printf("addr: %08x\n",addr);

  dp = (Disassembler *)info->application_data;

  assert(dp != nullptr);

  dp->overridePrintAddress(addr,info);
}

static void useage(char *name)
{
	printf("Useage: dqr (-t tracefile -e elffile | -n basename) [-start mn] [-stop mn] [-v] [-h]\n");
	printf("-t tracefile: Specify the name of the Nexus trace message file. Must contain the file extension (such as .rtd)\n");
	printf("-e elffile:   Specify the name of the executable elf file. Must contain the file extention (such as .elf)\n");
	printf("-n basename:  Specify the base name of hte Nexus trace message file and the executable elf file. No extension\n");
	printf("              should be given.\n");
	printf("              The extensions .rdt and .elf will be added to basename.\n");
	printf("-start nm:    Select the Nexus trace message number to begin DQing at. The first message is 1. If -stop is\n");
	printf("              not specified, continues to last trace message.\n");
	printf("-stop nm:     Select the last Nexus trace message number to end DQing at. If -start is not specified, starts\n");
	printf("              at trace message 1.\n");
	printf("-src:         Enable display of source lines in output if available (on by default).\n");
	printf("-nosrc:       Disable display of source lines in output.\n");
	printf("-file:        Display source file information in output (on by default).\n");
	printf("-nofile:      Do not dipslay source file information.\n");
	printf("-dasm:        Display disassembled code in output (on by default).\n");
	printf("-nodasm:      Do not display disassembled code in output.\n");
	printf("-trace:       Display trace information in output (off by default).\n");
	printf("-notrace:     Do not display trace information in output.\n");
	printf("--strip=path: Strip of the specified path when displaying source file name/path. Strips off all that matches.\n");
	printf("              Path may be enclosed in quotes if it contains spaces.\n");
	printf("-v:           Display the version number of the DQer and exit\n");
	printf("-h:           Display this useage information.\n");
}

const char *stripPath(const char *prefix,const char *srcpath)
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
	bool useage_flag = false;
	bool version_flag = false;
	int start_msg_num = 0;
	int stop_msg_num = 0;
	bool src_flag = true;
	bool file_flag = true;
	bool dasm_flag = true;
	bool trace_flag = false;
	bool func_flag = false;
	char *strip_flag = nullptr;

	for (int i = 1; i < argc; i++) {
		if (strcmp("-t",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -t requires a file name\n");
				useage(argv[0]);
				return 1;
			}

			base_name = nullptr;

			tf_name = argv[i];
		}
		else if (strcmp("-n",argv[i]) == 0) {
			i += 1;
			if (i >= argc) {
				printf("Error: option -n requires a file name\n");
				useage(argv[0]);
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
				useage(argv[0]);
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
				useage(argv[0]);
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
				useage(argv[0]);
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
			useage_flag = true;
		}
		else {
			printf("Unkown option '%s'\n",argv[i]);
			useage_flag = true;
		}
	}

	if (useage_flag) {
		useage(argv[0]);
		return 0;
	}

	if (version_flag) {
		printf("%s: version %s\n",argv[0],"0.1");
		return 0;
	}

	if (tf_name == nullptr) {
		if (base_name == nullptr) {
			printf("Error: must specify either a base name or a trace file name\n");
			useage(argv[0]);
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

	Trace *trace = new (std::nothrow) Trace(tf_name,binary_flag,ef_name,symFlags);

	assert(trace != nullptr);

	if (trace->getStatus() != DQERR_OK) {
		delete trace;
		trace = nullptr;

		printf("Error: new Trace(%s,%s) failed\n",tf_name,ef_name);

		return 1;
	}

	trace->setTraceRange(start_msg_num,stop_msg_num);

	DQErr ec;

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
	ADDRESS lastAddress = 0;
	int lastInstSize = 0;
	bool firstPrint = true;

	do {
		ec = trace->NextInstruction(&instInfo,&msgInfo,&srcInfo);
		if (ec == DQERR_OK) {
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

			if (trace_flag && (msgInfo != nullptr)) {
				// got the goods! Get to it!

				msgInfo->messageToText(dst,msgLevel);

				if (firstPrint == false) {
					printf("\n");
				}

				printf("Trace: %s",dst);

				printf("\n");

				firstPrint = false;
			}
		}
	} while (ec == DQERR_OK);

	if (firstPrint == false) {
		cout << endl;
	}

	delete trace;

	return 0;
}

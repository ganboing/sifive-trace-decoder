/*
 * Copyright 2019 Sifive, Inc.
 *
 * dqr.cpp
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

// static C type helper functions

static int atoh(char a)
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

// should probably delete this

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

static char *dis_output;

static int stringify_callback(FILE *stream, const char *format, ...)
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

// Section Class Methods

// work with elf file sections using libbfd

section::section()
{
	next      = nullptr;
	abfd      = nullptr;
	asecptr   = nullptr;
	size      = 0;
	startAddr = (dqr::ADDRESS)0;
	endAddr   = (dqr::ADDRESS)0;
	code      = nullptr;
}

section *section::initSection(section **head, asection *newsp)
{
	next = *head;
	*head = this;

	abfd = newsp->owner;
	asecptr = newsp;
	size = newsp->size;
	startAddr = (dqr::ADDRESS)newsp->vma;
	endAddr = (dqr::ADDRESS)(newsp->vma + size - 1);

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

section *section::getSectionByAddress(dqr::ADDRESS addr)
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

fileReader::fileReader(/*paths*/)
{
	lastFile = nullptr;
	files = nullptr;
}

fileReader::fileList *fileReader::readFile(const char *file)
{
	assert(file != nullptr);

	std::ifstream  f(file, std::ifstream::binary);

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
			f.open(file, std::ifstream::binary);
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

const char *Symtab::getSymbolByAddress(dqr::ADDRESS addr)
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
    status = dqr::dqr::DQERR_ERR;

    bfd_error_type bfd_error = bfd_get_error();
    printf("Error: bfd_openr() returned null. Error: %d\n",bfd_error);

    return;
  }

  if(!bfd_check_format(abfd,bfd_object)) {
    printf("Error: ElfReader(): %s not object file: %d\n",elfname,bfd_get_error());

	status = dqr::dqr::DQERR_ERR;

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

  status = dqr::DQERR_OK;
}

ElfReader::~ElfReader()
{
	if (symtab != nullptr) {
		delete symtab;
		symtab = nullptr;
	}
}

dqr::DQErr ElfReader::getInstructionByAddress(dqr::ADDRESS addr,dqr::RV_INST &inst)
{
	// get instruction at addr

	// Address for code[0] is text->vma

	//don't forget base!!'

	section *sp;
	if (codeSectionLst == nullptr) {
		status = dqr::DQERR_ERR;
		return status;
	}

	sp = codeSectionLst->getSectionByAddress(addr);
	if (sp == nullptr) {
		status = dqr::DQERR_ERR;
		return status;
	}

	if ((addr < sp->startAddr) || (addr > sp->endAddr)) {
		status = dqr::DQERR_ERR;
		return status;
	}

//	if ((addr < text->vma) || (addr >= text->vma + text->size)) {
////			don't know instruction - not part of text segment. need to handle for os
////			if we can't get the instruciton, get the next trace message and try again. or do
////			we need the next sync message and the next trace message and try again?
//
//		// for now, just return an error
//
//		status = dqr::DQERR_ERR;
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
		status = dqr::DQERR_OK;
		break;
	case 0x0003:	// not compressed. Assume RV32 for now
		if ((inst & 0x1f) == 0x1f) {
			fprintf(stderr,"Error: getInstructionByAddress(): cann't decode instructions longer than 32 bits\n");
			status = dqr::DQERR_ERR;
			break;
		}

		inst = inst | (((uint32_t)sp->code[index+1]) << 16);

		status = dqr::DQERR_OK;
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

NexusMessage::NexusMessage()
{
	msgNum         = 0;
	tcode          = dqr::TCODE_UNDEFINED;
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
	case dqr::TCODE_DEBUG_STATUS:
		sprintf(dst+n,"DEBUG STATUS (%d)",tcode);
		break;
	case dqr::TCODE_DEVICE_ID:
		sprintf(dst+n,"DEVICE ID (%d)",tcode);
		break;
	case dqr::TCODE_OWNERSHIP_TRACE:
		n += sprintf(dst+n,"OWNERSHIP TRACE (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," process: %d",ownership.process);
		}
		break;
	case dqr::TCODE_DIRECT_BRANCH:
		n += sprintf(dst+n,"DIRECT BRANCH (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," I-CNT: %d",directBranch.i_cnt);
		}
		break;
	case dqr::TCODE_INDIRECT_BRANCH:
		n += sprintf(dst+n,"INDIRECT BRANCH (%d)",tcode);

		if (level >= 2) {
			switch (indirectBranch.b_type) {
			case dqr::BTYPE_INDIRECT:
				bt = "Indirect";
				break;
			case dqr::BTYPE_EXCEPTION:
				bt = "Exception";
				break;
			case dqr::BTYPE_HARDWARE:
				bt = "Hardware";
				break;
			case dqr::BTYPE_UNDEFINED:
				bt = "Undefined";
				break;
			default:
				bt = "Bad Branch Type";
				break;
			}

			sprintf(dst+n," Branch Type: %s (%d) I-CNT: %d U-ADDR: 0x%08x ",bt,indirectBranch.b_type,indirectBranch.i_cnt,indirectBranch.u_addr);
		}
		break;
	case dqr::TCODE_DATA_WRITE:
		sprintf(dst+n,"DATA WRITE (%d)",tcode);
		break;
	case dqr::TCODE_DATA_READ:
		sprintf(dst+n,"DATA READ (%d)",tcode);
		break;
	case dqr::TCODE_DATA_ACQUISITION:
		sprintf(dst+n,"DATA ACQUISITION (%d)",tcode);
		break;
	case dqr::TCODE_ERROR:
		n += sprintf(dst+n,"ERROR (%d)",tcode);
		if (level >= 2) {
			sprintf(dst+n," Error Type %d",error.etype);
		}
		break;
	case dqr::TCODE_SYNC:
		n += sprintf(dst+n,"SYNC (%d)",tcode);

		if (level >= 2) {
			switch (sync.sync) {
			case dqr::SYNC_EVTI:
				sr = "EVTI";
				break;
			case dqr::SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case dqr::SYNC_T_CNT:
				sr = "T Count";
				break;
			case dqr::SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case dqr::SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case dqr::SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case dqr::SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case dqr::SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case dqr::SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case dqr::SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case dqr::SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s I-CNT: %d F-Addr: 0x%08x",sync.sync,sr,sync.i_cnt,sync.f_addr);
		}
		break;
	case dqr::TCODE_CORRECTION:
		sprintf(dst+n,"Correction (%d)",tcode);
		break;
	case dqr::TCODE_DIRECT_BRANCH_WS:
		n += sprintf(dst+n,"DIRECT BRANCH WS (%d)",tcode);

		if (level >= 2) {
			switch (directBranchWS.sync) {
			case dqr::SYNC_EVTI:
				sr = "EVTI";
				break;
			case dqr::SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case dqr::SYNC_T_CNT:
				sr = "T Count";
				break;
			case dqr::SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case dqr::SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case dqr::SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case dqr::SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case dqr::SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case dqr::SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case dqr::SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case dqr::SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s I-CNT: %d F-Addr: 0x%08x",directBranchWS.sync,sr,directBranchWS.i_cnt,directBranchWS.f_addr);
		}
		break;
	case dqr::TCODE_INDIRECT_BRANCH_WS:
		n += sprintf(dst+n,"INDIRECT BRANCH WS (%d)",tcode);

		if (level >= 2) {
			switch (indirectBranchWS.sync) {
			case dqr::SYNC_EVTI:
				sr = "EVTI";
				break;
			case dqr::SYNC_EXIT_RESET:
				sr = "Exit Reset";
				break;
			case dqr::SYNC_T_CNT:
				sr = "T Count";
				break;
			case dqr::SYNC_EXIT_DEBUG:
				sr = "Exit Debug";
				break;
			case dqr::SYNC_I_CNT_OVERFLOW:
				sr = "I-Count Overflow";
				break;
			case dqr::SYNC_TRACE_ENABLE:
				sr = "Trace Enable";
				break;
			case dqr::SYNC_WATCHPINT:
				sr = "Watchpoint";
				break;
			case dqr::SYNC_FIFO_OVERRUN:
				sr = "FIFO Overrun";
				break;
			case dqr::SYNC_EXIT_POWERDOWN:
				sr = "Exit Powerdown";
				break;
			case dqr::SYNC_MESSAGE_CONTENTION:
				sr = "Message Contention";
				break;
			case dqr::SYNC_NONE:
				sr = "None";
				break;
			default:
				sr = "Bad Sync Reason";
				break;
			}

			switch (indirectBranchWS.b_type) {
			case dqr::BTYPE_INDIRECT:
				bt = "Indirect";
				break;
			case dqr::BTYPE_EXCEPTION:
				bt = "Exception";
				break;
			case dqr::BTYPE_HARDWARE:
				bt = "Hardware";
				break;
			case dqr::BTYPE_UNDEFINED:
				bt = "Undefined";
				break;
			default:
				bt = "Bad Branch Type";
				break;
			}

			sprintf(dst+n," Reason: (%d) %s Branch Type %s (%d) I-CNT: %d F-Addr: 0x%08x",indirectBranchWS.sync,sr,bt,indirectBranchWS.b_type,indirectBranchWS.i_cnt,indirectBranchWS.f_addr);
		}
		break;
	case dqr::TCODE_DATA_WRITE_WS:
		sprintf(dst+n,"DATA WRITE WS (%d)",tcode);
		break;
	case dqr::TCODE_DATA_READ_WS:
		sprintf(dst+n,"DATA READ WS (%d)",tcode);
		break;
	case dqr::TCODE_WATCHPOINT:
		sprintf(dst+n,"TCode: WATCHPOINT (%d)",tcode);
		break;
	case dqr::TCODE_OUTPUT_PORTREPLACEMENT:
		sprintf(dst+n,"OUTPUT PORT REPLACEMENT (%d)",tcode);
		break;
	case dqr::TCODE_INPUT_PORTREPLACEMENT:
		sprintf(dst+n,"INPUT PORT REPLACEMENT (%d)",tcode);
		break;
	case dqr::TCODE_AUXACCESS_READ:
		sprintf(dst+n,"AUX ACCESS READ (%d)",tcode);
		break;
	case dqr::TCODE_AUXACCESS_WRITE:
		n += sprintf(dst+n,"AUX ACCESS WRITE (%d)",tcode);

		if (level >= 2) {
			sprintf(dst+n," Addr: 0x%08x Data: %0x08x",auxAccessWrite.addr,auxAccessWrite.data);
		}
		break;
	case dqr::TCODE_AUXACCESS_READNEXT:
		sprintf(dst+n,"AUX ACCESS READNEXT (%d)",tcode);
		break;
	case dqr::TCODE_AUXACCESS_WRITENEXT:
		sprintf(dst+n,"AUX ACCESS WRITENEXT (%d)",tcode);
		break;
	case dqr::TCODE_AUXACCESS_RESPONSE:
		sprintf(dst+n,"AUXACCESS RESPOINSE (%d)",tcode);
		break;
	case dqr::TCODE_RESURCEFULL:
		sprintf(dst+n,"RESOURCE FULL (%d)",tcode);
		break;
	case dqr::TCODE_INDIRECTBRANCHHISOTRY:
		sprintf(dst+n,"INDIRECT BRANCH HISTORY (%d)",tcode);
		break;
	case dqr::TCODE_INDIRECTBRANCHHISORY_WS:
		sprintf(dst+n,"INDIRECT BRANCH HISTORY WS (%d)",tcode);
		break;
	case dqr::TCODE_REPEATBRANCH:
		sprintf(dst+n,"REPEAT BRANCH (%d)",tcode);
		break;
	case dqr::TCODE_REPEATINSTRUCITON:
		sprintf(dst+n,"REPEAT INSTRUCTION (%d)",tcode);
		break;
	case dqr::TCODE_REPEATSINSTURCIONT_WS:
		sprintf(dst+n,"REPEAT INSTRUCTIN WS (%d)",tcode);
		break;
	case dqr::TCODE_CORRELATION:
		n += sprintf(dst+n,"CORRELATION (%d)",tcode);

		if (level >= 2) {
			sprintf(dst+n," EVCODE: %d I-CNT: %d",correlation.evcode,correlation.i_cnt);
		}
		break;
	case dqr::TCODE_INCIRCUITTRACE:
		sprintf(dst+n,"INCIRCUITTRACE (%d)",tcode);
		break;
	case dqr::TCODE_UNDEFINED:
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
	case dqr::TCODE_DEBUG_STATUS:
		std::cout << "unsupported debug status trace message\n";
		break;
	case dqr::TCODE_DEVICE_ID:
		std::cout << "unsupported device id trace message\n";
		break;
	case dqr::TCODE_OWNERSHIP_TRACE:
//bks
		std::cout << "  # Trace Message(" << msgNum << "): Ownership, process=" << ownership.process; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case dqr::TCODE_DIRECT_BRANCH:
//		cout << "(" << traceNum << ") ";
//		cout << "  | Direct Branch | TYPE=DBM, ICNT=" << i_cnt << ", TS=0x" << hex << timestamp << dec; // << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Direct Branch, ICNT=" << i_cnt; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case dqr::TCODE_INDIRECT_BRANCH:
//		cout << "(" << traceNum << ") ";
//		cout << "  | Indirect Branch | TYPE=IBM, BTYPE=" << b_type << ", ICNT=" << i_cnt << ", UADDR=0x" << hex << u_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Indirect Branch, BTYPE=" << b_type << ", ICNT=" << i_cnt << ", UADDR=0x" << hex << u_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case dqr::TCODE_DATA_WRITE:
		std::cout << "unsupported data write trace message\n";
		break;
	case dqr::TCODE_DATA_READ:
		std::cout << "unsupported data read trace message\n";
		break;
	case dqr::TCODE_DATA_ACQUISITION:
		std::cout << "unsupported data acquisition trace message\n";
		break;
	case dqr::TCODE_ERROR:
//bks		cout << "  # Trace Message(" << msgNum << "): Error, ETYPE=" << (uint32_t)etype; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case dqr::TCODE_SYNC:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Sync | TYPE=SYNC, SYNC=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Sync, SYNCREASON=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case dqr::TCODE_CORRECTION:
		std::cout << "unsupported correction trace message\n";
		break;
	case dqr::TCODE_DIRECT_BRANCH_WS:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Direct Branch With Sync | TYPE=DBWS, SYNC=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Direct Branch With Sync, SYNCTYPE=" << sync << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case dqr::TCODE_INDIRECT_BRANCH_WS:
//		cout << "(" << traceNum << "," << syncNum << ") ";
//		cout << "  | Indirect Branch With Sync | TYPE=IBWS, SYNC=" << sync << ", BTYPE=" << b_type << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << ", TS=0x" << timestamp << dec;// << endl;
//bks		cout << "  # Trace Message(" << msgNum << "): Indirect Branch With sync, SYNCTYPE=" << sync << ", BTYPE=" << b_type << ", ICNT=" << i_cnt << ", FADDR=0x" << hex << f_addr << dec; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	case dqr::TCODE_DATA_WRITE_WS:
		std::cout << "unsupported data write with sync trace message\n";
		break;
	case dqr::TCODE_DATA_READ_WS:
		std::cout << "unsupported data read with sync trace message\n";
		break;
	case dqr::TCODE_WATCHPOINT:
		std::cout << "unsupported watchpoint trace message\n";
		break;
	case dqr::TCODE_AUXACCESS_WRITE:
//bks		cout << "  # Trace Message(" << msgNum << "): Auxillary Access Write, address=" << hex << auxAddr << dec << ", data=" << hex << data << dec; // << ", TS=0x" << hex << timestamp << dec; // << endl;
		break;
	case dqr::TCODE_CORRELATION:
//bks		cout << "  # Trace Message(" << msgNum << "): Correlation, EVCODE=" << (uint32_t)evcode << ", CDF=" << (int)cdf << ", ICNT=" << i_cnt << "\n"; // << ", TS=0x" << timestamp << dec;// << endl;
		break;
	default:
		std::cout << "Error: NexusMessage::dump(): Unknown TCODE " << std::hex << tcode << std::dec << "msgnum: " << msgNum << std::endl;
	}
}

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
		tf.open(filename, std::ios::in | std::ios::binary);
	}
	else {
		tf.open(filename, std::ios::in);
	}

	if (!tf) {
		printf("Error: SliceFileParder(): could not open file %s for input",filename);
		status = dqr::DQERR_OPEN;
	}
	else {
		status = dqr::DQERR_OK;
	}
}

void SliceFileParser::dump()
{
	//msg and msgSlices

	for (int i = 0; i < msgSlices; i++) {
		std::cout << std::setw(2) << i+1;
		std::cout << " | ";
		std::cout << std::setfill('0');
		std::cout << std::setw(2) << std::hex << int(msg[i] >> 2) << std::dec;
		std::cout << std::setfill(' ');
		std::cout << " | ";
		std::cout << int((msg[i] >> 1) & 1);
		std::cout << " ";
		std::cout << int(msg[i] & 1);
		std::cout << std::endl;
	}
}

dqr::DQErr SliceFileParser::parseDirectBranch(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t i_cnt;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message has been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode     = dqr::TCODE_DIRECT_BRANCH;
	nm.directBranch.i_cnt     = (int)i_cnt;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = timestamp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseDirectBranchWS(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t i_cnt;
	uint64_t sync;
	uint64_t f_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed length sync reason field

	rc = parseFixedField(4,&sync);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode                 = dqr::TCODE_DIRECT_BRANCH_WS;
	nm.directBranchWS.sync   = (dqr::SyncReason)sync;
	nm.directBranchWS.i_cnt  = i_cnt;
	nm.directBranchWS.f_addr = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseIndirectBranch(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t b_type;
	uint64_t i_cnt;
	uint64_t u_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed lenght b-type

	rc = parseFixedField(2,&b_type);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&u_addr);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}
		haveTimestamp = true;
	}

	nm.tcode                 = dqr::TCODE_INDIRECT_BRANCH;
	nm.indirectBranch.b_type = (dqr::BType)b_type;
	nm.indirectBranch.i_cnt  = (int)i_cnt;
	nm.indirectBranch.u_addr = u_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseIndirectBranchWS(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t sync;
	uint64_t b_type;
	uint64_t i_cnt;
	uint64_t f_addr;
	uint64_t timestamp;
	bool     haveTimestamp;

	// parse the fixed length sync field

	rc = parseFixedField(4,&sync);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the fixed length b-type

	rc = parseFixedField(2,&b_type);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	// parse the variable length the i-cnt

	rc = parseVarField(&i_cnt);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode                   = dqr::TCODE_INDIRECT_BRANCH_WS;
	nm.indirectBranchWS.sync   = (dqr::SyncReason)sync;
	nm.indirectBranchWS.b_type = (dqr::BType)b_type;
	nm.indirectBranchWS.i_cnt  = (int)i_cnt;
	nm.indirectBranchWS.f_addr = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseSync(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t i_cnt;
	uint64_t timestamp;
	uint64_t sync;
	uint64_t f_addr;
	bool     haveTimestamp;

	// parse the variable length the i-cnt

	rc = parseFixedField(4,&sync);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&i_cnt);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	rc = parseVarField(&f_addr);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		timestamp = 0;
	}
	else {
		rc = parseVarField(&timestamp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.tcode         = dqr::TCODE_SYNC;
	nm.sync.sync     = (dqr::SyncReason)sync;
	nm.sync.i_cnt    = (int)i_cnt;
	nm.sync.f_addr   = f_addr;
	nm.haveTimestamp = haveTimestamp;
	nm.timestamp     = timestamp;

	numTraceMsgs += 1;
	numSyncMsgs  += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseCorrelation(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = dqr::TCODE_CORRELATION;

	// parse the 4-bit evcode field

	rc = parseFixedField(4,&tmp);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	nm.correlation.evcode = tmp;

	// parse the 2-bit cdf field

	rc = parseFixedField(2,&tmp);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (tmp != 0) {
		printf("Error: DQErr SliceFileParser::parseCorrelation(): Expected EVCODE to be 0\n");

		status = dqr::DQERR_ERR;

		return status;
	}

	nm.correlation.cdf = tmp;

	// parse the variable length i-cnt field

	rc = parseVarField(&tmp);
	if (rc != dqr::DQERR_OK) {
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
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseError(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = dqr::TCODE_ERROR;

	// parse the 4 bit ETYPE field

	rc = parseFixedField(4,&tmp);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	nm.error.etype = (uint8_t)tmp;

	// parse the variable sized padd field

	rc = parseVarField(&tmp);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	if (eom == true) {
		haveTimestamp = false;
		tmp = 0;
	}
	else {
		rc = parseVarField(&tmp); // this field is optional - check err
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseOwnershipTrace(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = dqr::TCODE_OWNERSHIP_TRACE;

	// parse the variable length process ID field

	rc = parseVarField(&tmp);
	if (rc != dqr::DQERR_OK) {
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
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseAuxAccessWrite(NexusMessage &nm)
{
	dqr::DQErr    rc;
	uint64_t tmp;
	bool     haveTimestamp;

	nm.tcode = dqr::TCODE_AUXACCESS_WRITE;

	// parse the ADDR field

	rc = parseVarField(&tmp);
	if (rc != dqr::DQERR_OK) {
		status = rc;

		return status;
	}

	nm.auxAccessWrite.addr = (uint32_t)tmp;

	// parse the data field

	rc = parseVarField(&tmp);
	if (rc != dqr::DQERR_OK) {
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
		if (rc != dqr::DQERR_OK) {
			status = rc;

			return status;
		}

		// check if entire message have been consumed

		if (eom != true) {
			status = dqr::DQERR_BM;

			return status;
		}

		haveTimestamp = true;
	}

	nm.haveTimestamp = haveTimestamp;
	nm.timestamp = tmp;

	numTraceMsgs += 1;

	nm.msgNum = numTraceMsgs;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseFixedField(int width, uint64_t *val)
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

		status = dqr::DQERR_EOM;

		return dqr::DQERR_EOM;
	}

	if (b+width > 6) {
		// don't support fixed width > 6 bits or that cross slice boundary

		status = dqr::DQERR_ERR;

		return dqr::DQERR_ERR;
	}

	uint8_t v;

	// strip off upper and lower bits not part of field

	v = msg[i] << (6-(b+width));
	v = v >> ((6-(b+width))+b+2);

	*val = uint64_t(v);

//	printf("-> bitIndex: %d, value: %x\n",bitIndex,v);

	if ((msg[i] & 0x03) == dqr::MSEO_END) {
		eom = true;
	}

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::parseVarField(uint64_t *val)
{
	assert(val != nullptr);

	int i;
	int b;
	int width = 0;

	i = bitIndex / 6;
	b = bitIndex % 6;

	if (i >= msgSlices) {
		// read past end of message

		status = dqr::DQERR_EOM;

		return dqr::DQERR_EOM;
	}

	uint64_t v;

	// strip off upper and lower bits not part of field

	width = 6-b;

//	printf("parseVarField(): bitIndex:%d, i:%d, b:%d, width: %d\n",bitIndex,i,b,width);

	v = msg[i] >> (b+2);

	while ((msg[i] & 0x03) == dqr::MSEO_NORMAL) {
		i += 1;
		if (i >= msgSlices) {
			// read past end of message

			status = dqr::DQERR_ERR;

			return dqr::DQERR_ERR;
		}

		v = v | ((msg[i] >> 2) << width);
		width += 6;
	}

	if ((msg[i] & 0x03) == dqr::MSEO_END) {
		eom = true;
	}

	bitIndex += width;
	*val = v;

//	printf("-> bitIndex: %d, value: %llx\n",bitIndex,v);

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::readBinaryMsg()
{
	do {
		tf.read((char*)&msg[0],sizeof msg[0]);
		if (!tf) {
			status = dqr::DQERR_EOF;

			return dqr::DQERR_EOF;
		}
	} while ((msg[0] & 0x3) == dqr::MSEO_END);

	// make sure this is start of nexus message

	if ((msg[0] & 0x3) != dqr::MSEO_NORMAL) {
		status = dqr::DQERR_ERR;

		std::cout << "Error: SliceFileParser::readBinaryMsg(): expected start of message; got" << std::hex << static_cast<uint8_t>(msg[0] & 0x3) << std::dec << std::endl;

		return dqr::DQERR_ERR;
	}

	bool done = false;

	for (int i = 1; !done; i++) {
		if (i >= (int)(sizeof msg / sizeof msg[0])) {
			std::cout << "Error: SliceFileParser::readBinaryMsg(): msg buffer overflow" << std::endl;

			status = dqr::DQERR_ERR;

			return dqr::DQERR_ERR;
		}

		tf.read((char*)&msg[i],sizeof msg[0]);
		if (!tf) {
			status = dqr::DQERR_ERR;

			std::cout << "error reading stream\n";

			return dqr::DQERR_ERR;
		}

		if ((msg[i] & 0x03) == dqr::MSEO_END) {
			done = true;
			msgSlices = i+1;
		}
	}

	eom = false;

	bitIndex = 0;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::readNextByte(uint8_t *byte)
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
			status = dqr::DQERR_EOF;

			return dqr::DQERR_EOF;
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
		status = dqr::DQERR_EOF;

		return dqr::DQERR_EOF;
	}

	int hd;
	uint8_t hn;

	hd = atoh(c);
	if (hd < 0) {
		status = dqr::DQERR_ERR;

		return dqr::DQERR_ERR;
	}

	hn = hd << 4;

	hd = atoh(c);
	if (hd < 0) {
		status = dqr::DQERR_ERR;

		return dqr::DQERR_ERR;
	}

	hn = hn | hd;

	*byte = hn;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::readAscMsg()
{
	std::cout << "Error: SliceFileP{arser::readAscMsg(): not implemented" << std::endl;
	status = dqr::DQERR_ERR;

	return dqr::DQERR_ERR;

	// strip off idle bytes

	do {
		if (readNextByte(&msg[0]) != dqr::DQERR_OK) {
			return status;
		}
	} while ((msg[0] & 0x3) == dqr::MSEO_END);

	// make sure this is start of nexus message

	if ((msg[0] & 0x3) != dqr::MSEO_NORMAL) {
		status = dqr::DQERR_ERR;

		std::cout << "Error: SliceFileParser::readBinaryMsg(): expected start of message; got" << std::hex << static_cast<uint8_t>(msg[0] & 0x3) << std::dec << std::endl;

		return dqr::DQERR_ERR;
	}

	// now get bytes

	bool done = false;

	for (int i = 1; !done; i++) {
		if (i >= (int)(sizeof msg / sizeof msg[0])) {
			std::cout << "Error: SliceFileParser::readBinaryMsg(): msg buffer overflow" << std::endl;

			status = dqr::DQERR_ERR;

			return dqr::DQERR_ERR;
		}

		if (readNextByte(&msg[i]) != dqr::DQERR_OK) {
			return status;
		}

		if ((msg[i] & 0x03) == dqr::MSEO_END) {
			done = true;
			msgSlices = i+1;
		}
	}

	eom = false;

	bitIndex = 0;

	return dqr::DQERR_OK;
}

dqr::DQErr SliceFileParser::nextTraceMsg(NexusMessage &nm)	// generator to return trace messages one at a time
{

	assert(status == dqr::DQERR_OK);

	dqr::DQErr    rc;
	uint64_t val;
	uint8_t  tcode;

	// read from file, store in object, compute and fill out full fields, such as address and more later
	// need some place to put it. An object

	if (binary) {
		rc = readBinaryMsg();
		if (rc != dqr::DQERR_OK) {
			if (rc != dqr::DQERR_EOF) {
				std::cout << "Error: (): readBinaryMsg() returned error " << rc << std::endl;
			}

			status = rc;

			return status;
		}
	}
	else {	// text trace messages
		rc = readAscMsg();
		if (rc != dqr::DQERR_OK) {
			if (rc != dqr::DQERR_EOF) {
				std::cout << "Error: (): readTxtMsg() returned error " << rc << std::endl;
			}

			status = rc;

			return status;
		}
	}

// crow		dump();

	rc = parseFixedField(6, &val);
	if (rc != dqr::DQERR_OK) {
		std::cout << "Error: (): could not read tcode\n";

		status = rc;

		return status;
	}

	tcode = (uint8_t)val;

	switch (tcode) {
	case dqr::TCODE_DEBUG_STATUS:
		std::cout << "unsupported debug status trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_DEVICE_ID:
		std::cout << "unsupported device id trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_OWNERSHIP_TRACE:
		status = parseOwnershipTrace(nm);
		break;
	case dqr::TCODE_DIRECT_BRANCH:
//		cout << " | direct branch";
		status = parseDirectBranch(nm);
		break;
	case dqr::TCODE_INDIRECT_BRANCH:
//		cout << "| indirect branch";
		status = parseIndirectBranch(nm);
		break;
	case dqr::TCODE_DATA_WRITE:
		std::cout << "unsupported data write trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_DATA_READ:
		std::cout << "unsupported data read trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_DATA_ACQUISITION:
		std::cout << "unsupported data acquisition trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_ERROR:
		status = parseError(nm);
		break;
	case dqr::TCODE_SYNC:
//		cout << "| sync";
		status = parseSync(nm);
		break;
	case dqr::TCODE_CORRECTION:
		std::cout << "unsupported correction trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_DIRECT_BRANCH_WS:
//		cout << "| direct branch with sync\n";
		status = parseDirectBranchWS(nm);
		break;
	case dqr::TCODE_INDIRECT_BRANCH_WS:
//		cout << "| indirect branch with sync";
		status = parseIndirectBranchWS(nm);
		break;
	case dqr::TCODE_DATA_WRITE_WS:
		std::cout << "unsupported data write with sync trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_DATA_READ_WS:
		std::cout << "unsupported data read with sync trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_WATCHPOINT:
		std::cout << "unsupported watchpoint trace message\n";
		status = dqr::DQERR_ERR;
		break;
	case dqr::TCODE_CORRELATION:
		status = parseCorrelation(nm);
		break;
	case dqr::TCODE_AUXACCESS_WRITE:
		status = parseAuxAccessWrite(nm);
		break;
	default:
		std::cout << "Error: nextTraceMsg(): Unknown TCODE " << std::hex << tcode << std::dec << std::endl;
		status = dqr::DQERR_ERR;
	}

	return status;
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
  			  status = dqr::DQERR_ERR;
  			  return;
  		  }
  	  }
    }

    if (codeSectionLst == nullptr) {
    	printf("Error: no code sections found\n");

    	status = dqr::DQERR_ERR;
    	return;
    }

   	info = new (std::nothrow) disassemble_info;

   	assert(info != nullptr);

   	disassemble_func = disassembler(bfd_get_arch(abfd), bfd_big_endian(abfd),bfd_get_mach(abfd), abfd);
   	if (disassemble_func == nullptr) {
   		printf("Error: Disassmbler::Disassembler(): could not create disassembler\n");

   		delete [] info;
   		info = nullptr;

   		status = dqr::DQERR_ERR;
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

    status = dqr::DQERR_OK;
}

int Disassembler::lookupInstructionByAddress(bfd_vma vma,uint32_t *ins,int *ins_size)
{
	assert(ins != nullptr);

	uint32_t inst;
	int size;
	int rc;

	// need to support multiple code sections and do some error checking on the address!

	if (codeSectionLst == nullptr) {
		status = dqr::DQERR_ERR;

		return 1;
	}

	section *sp;

	sp = codeSectionLst->getSectionByAddress((dqr::ADDRESS)vma);

	if (sp == nullptr) {
		status = dqr::DQERR_ERR;

		return 1;
	}

	inst = (uint32_t)sp->code[(vma - sp->startAddr)/2];

	rc = decodeInstructionSize(inst, size);
	if (rc != 0) {
		status = dqr::DQERR_ERR;

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

int Disassembler::getSrcLines(dqr::ADDRESS addr, const char **filename, const char **functionname, unsigned int *linenumber, const char **lineptr)
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

	struct fileReader::fileList *fl;

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

int Disassembler::Disassemble(dqr::ADDRESS addr)
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

NexusMessageSync::NexusMessageSync()
{
	firstMsgNum = 0;
	lastMsgNum  = 0;
	index = 0;
}


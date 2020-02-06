/*
 * Copyright 2019 Sifive, Inc.
 *
 * main.cpp
 *
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
#include "trace.hpp"

// class trace methods

int Trace::decodeInstructionSize(uint32_t inst, int &inst_size)
{
  return disassembler->decodeInstructionSize(inst,inst_size);
}

int Trace::decodeInstruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,int32_t &immeadiate,bool &is_branch)
{
	return disassembler->decodeInstruction(instruction,inst_size,inst_type,immeadiate,is_branch);
}

Trace::Trace(char *tf_name,bool binaryFlag,char *ef_name,int numAddrBits,uint32_t addrDispFlags,int srcBits,uint32_t freq)
{
  sfp          = nullptr;
  elfReader    = nullptr;
  symtab       = nullptr;
  disassembler = nullptr;

  assert(tf_name != nullptr);

  itcPrint = nullptr;

  srcbits = srcBits;

  analytics.setSrcBits(srcBits);

  sfp = new (std::nothrow) SliceFileParser(tf_name,binaryFlag,srcbits);

  assert(sfp != nullptr);

  if (sfp->getErr() != TraceDqr::DQERR_OK) {
	printf("Error: cannot open trace file '%s' for input\n",tf_name);
	delete sfp;
	sfp = nullptr;

	status = TraceDqr::DQERR_ERR;

	return;
  }

  if (ef_name != nullptr ) {
	// create elf object

//    printf("ef_name:%s\n",ef_name);

     elfReader = new (std::nothrow) ElfReader(ef_name);

    assert(elfReader != nullptr);

    if (elfReader->getStatus() != TraceDqr::DQERR_OK) {
    	if (sfp != nullptr) {
    		delete sfp;
    		sfp = nullptr;
    	}

    	delete elfReader;
    	elfReader = nullptr;

    	status = TraceDqr::DQERR_ERR;

    	return;
    }

    // create disassembler object

    bfd *abfd;
    abfd = elfReader->get_bfd();

	disassembler = new (std::nothrow) Disassembler(abfd);

	assert(disassembler != nullptr);

	if (disassembler->getStatus() != TraceDqr::DQERR_OK) {
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

		status = TraceDqr::DQERR_ERR;

		return;
	}

    // get symbol table

    symtab = elfReader->getSymtab();
    if (symtab == nullptr) {
    	delete elfReader;
    	elfReader = nullptr;

    	delete sfp;
    	sfp = nullptr;

    	status = TraceDqr::DQERR_ERR;

    	return;
    }
  }

  for (int i = 0; (size_t)i < sizeof lastFaddr / sizeof lastFaddr[0]; i++ ) {
	lastFaddr[i] = 0;
  }

  for (int i = 0; (size_t)i < sizeof currentAddress / sizeof currentAddress[0]; i++ ) {
	currentAddress[i] = 0;
  }

  for (int i = 0; (size_t)i < sizeof i_cnt / sizeof i_cnt[0]; i++ ) {
	i_cnt[i] = 0;
  }

  for (int i = 0; (size_t)i < sizeof state / sizeof state[0]; i++ ) {
	state[i] = TRACE_STATE_GETFIRSTYNCMSG;
  }

  readNewTraceMessage = true;
  currentCore = 0;	// as good as eny!

  for (int i = 0; (size_t)i < sizeof lastTime / sizeof lastTime[0]; i++) {
	  lastTime[i] = 0;
  }

  startMessageNum  = 0;
  endMessageNum    = 0;

  for (int i = 0; (size_t)i < (sizeof messageSync / sizeof messageSync[0]); i++) {
	  messageSync[i] = nullptr;
  }

  instructionInfo.address = 0;
  instructionInfo.instruction = 0;
  instructionInfo.instSize = 0;

  if (numAddrBits != 0 ) {
	  instructionInfo.addrSize = numAddrBits;
  }
  else {
	  instructionInfo.addrSize = elfReader->getBitsPerAddress();
  }

  instructionInfo.addrDispFlags = addrDispFlags;

  instructionInfo.addrPrintWidth = (instructionInfo.addrSize + 3) / 4;

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

  NexusMessage::targetFrequency = freq;

  status = TraceDqr::DQERR_OK;
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

	if (itcPrint  != nullptr) {
		delete itcPrint;
		itcPrint = nullptr;
	}
}

int Trace::getArchSize()
{
	if (elfReader == nullptr) {
		return 0;
	}

	return elfReader->getArchSize();
}

int Trace::getAddressSize()
{
	if (elfReader == nullptr) {
		return 0;
	}

	return elfReader->getBitsPerAddress();
}

TraceDqr::DQErr Trace::setTraceRange(int start_msg_num,int stop_msg_num)
{
	if (start_msg_num < 0) {
		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	if (stop_msg_num < 0) {
		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	if ((stop_msg_num != 0) && (start_msg_num > stop_msg_num)) {
		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	startMessageNum = start_msg_num;
	endMessageNum = stop_msg_num;

	for (int i = 0; (size_t)i < sizeof state / sizeof state[0]; i++) {
		state[i] = TRACE_STATE_GETSTARTTRACEMSG;
	}

	return TraceDqr::DQERR_OK;
}

TraceDqr::ADDRESS Trace::computeAddress()
{
	switch (nm.tcode) {
	case TraceDqr::TCODE_DEBUG_STATUS:
		break;
	case TraceDqr::TCODE_DEVICE_ID:
		break;
	case TraceDqr::TCODE_OWNERSHIP_TRACE:
		break;
	case TraceDqr::TCODE_DIRECT_BRANCH:
//		currentAddress = target of branch.
		break;
	case TraceDqr::TCODE_INDIRECT_BRANCH:
		currentAddress[currentCore] = currentAddress[currentCore] ^ (nm.indirectBranch.u_addr << 1);	// note - this is the next address!
		break;
	case TraceDqr::TCODE_DATA_WRITE:
		break;
	case TraceDqr::TCODE_DATA_READ:
		break;
	case TraceDqr::TCODE_DATA_ACQUISITION:
		break;
	case TraceDqr::TCODE_ERROR:
		break;
	case TraceDqr::TCODE_SYNC:
		currentAddress[currentCore] = nm.sync.f_addr << 1;
		break;
	case TraceDqr::TCODE_CORRECTION:
		break;
	case TraceDqr::TCODE_DIRECT_BRANCH_WS:
		currentAddress[currentCore] = nm.directBranchWS.f_addr << 1;
		break;
	case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
		currentAddress[currentCore] = nm.indirectBranchWS.f_addr << 1;
		break;
	case TraceDqr::TCODE_DATA_WRITE_WS:
		break;
	case TraceDqr::TCODE_DATA_READ_WS:
		break;
	case TraceDqr::TCODE_WATCHPOINT:
		break;
	case TraceDqr::TCODE_CORRELATION:
		break;
	default:
		break;
	}

	std::cout << "New address 0x" << std::hex << currentAddress[currentCore] << std::dec << std::endl;

	return currentAddress[currentCore];
}

int Trace::Disassemble(TraceDqr::ADDRESS addr)
{
	assert(disassembler != nullptr);

	int   rc;
	TraceDqr::DQErr s;

	rc = disassembler->Disassemble(addr);

	s = disassembler->getStatus();

	if (s != TraceDqr::DQERR_OK ) {
	  status = s;
	  return 0;
	}

	// the two lines below copy each structure completely. This is probably
	// pretty inefficient, and just returning pointers and using pointers
	// would likely be better

	instructionInfo = disassembler->getInstructionInfo();
	sourceInfo = disassembler->getSourceInfo();

	return rc;
}

const char *Trace::getSymbolByAddress(TraceDqr::ADDRESS addr)
{
	return symtab->getSymbolByAddress(addr);
}

const char *Trace::getNextSymbolByAddress()
{
	return symtab->getNextSymbolByAddress();
}

TraceDqr::DQErr Trace::setITCPrintOptions(int buffSize,int channel)
{
	if (itcPrint != nullptr) {
		delete itcPrint;
		itcPrint = nullptr;
	}

	itcPrint = new ITCPrint(1 << srcbits,buffSize,channel);

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr Trace::haveITCPrintData(int numMsgs[DQR_MAXCORES], bool havePrintData[DQR_MAXCORES])
{
	if (itcPrint == nullptr) {
		return TraceDqr::DQERR_ERR;
	}

	itcPrint->haveITCPrintData(numMsgs, havePrintData);

	return TraceDqr::DQERR_OK;
}

bool Trace::getITCPrintMsg(int core, char *dst, int dstLen, TraceDqr::TIMESTAMP &startTime, TraceDqr::TIMESTAMP &endTime)
{
	if (itcPrint == nullptr) {
		return false;
	}

	return itcPrint->getITCPrintMsg(core,dst,dstLen,startTime,endTime);
}

bool Trace::flushITCPrintMsg(int core, char *dst, int dstLen,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime)
{
	if (itcPrint == nullptr) {
		return false;
	}

	return itcPrint->flushITCPrintMsg(core,dst,dstLen,startTime,endTime);
}

std::string Trace::getITCPrintStr(int core, bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime)
{
	std::string s = "";

	if (itcPrint == nullptr) {
		haveData = false;
	}
	else {
		haveData = itcPrint->getITCPrintStr(core,s,startTime,endTime);
	}

	return s;
}

std::string Trace::getITCPrintStr(int core, bool &haveData,double &startTime,double &endTime)
{
	std::string s = "";
	TraceDqr::TIMESTAMP sts, ets;

	if (itcPrint == nullptr) {
		haveData = false;
	}
	else {
		haveData = itcPrint->getITCPrintStr(core,s,sts,ets);

		if (haveData != false) {
			if (NexusMessage::targetFrequency != 0) {
				startTime = ((double)sts)/NexusMessage::targetFrequency;
				endTime = ((double)ets)/NexusMessage::targetFrequency;
			}
			else {
				startTime = sts;
				endTime = ets;
			}
		}
	}

	return s;
}

std::string Trace::flushITCPrintStr(int core, bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime)
{
	std::string s = "";

	if (itcPrint == nullptr) {
		haveData = false;
	}
	else {
		haveData = itcPrint->flushITCPrintStr(core,s,startTime,endTime);
	}

	return s;
}

std::string Trace::flushITCPrintStr(int core, bool &haveData,double &startTime,double &endTime)
{
	std::string s = "";
	TraceDqr::TIMESTAMP sts, ets;

	if (itcPrint == nullptr) {
		haveData = false;
	}
	else {
		haveData = itcPrint->flushITCPrintStr(core,s,sts,ets);

		if (haveData != false) {
			if (NexusMessage::targetFrequency != 0) {
				startTime = ((double)sts)/NexusMessage::targetFrequency;
				endTime = ((double)ets)/NexusMessage::targetFrequency;
			}
			else {
				startTime = sts;
				endTime = ets;
			}
		}
	}

	return s;
}

TraceDqr::DQErr Trace::NextInstruction(Instruction *instInfo,NexusMessage *msgInfo,Source *srcInfo,int *flags)
{
	TraceDqr::DQErr ec;

	Instruction  *instInfop = nullptr;
	NexusMessage *msgInfop  = nullptr;
	Source       *srcInfop  = nullptr;

	Instruction  **instInfopp = nullptr;
	NexusMessage **msgInfopp  = nullptr;
	Source       **srcInfopp  = nullptr;

	if (instInfo != nullptr) {
		instInfopp = &instInfop;
	}

	if (msgInfo != nullptr) {
		msgInfopp = &msgInfop;
	}

	if (srcInfo != nullptr) {
		srcInfopp = &srcInfop;
	}

	ec = NextInstruction(instInfopp, msgInfopp, srcInfopp);

	*flags = 0;

	if (ec == TraceDqr::DQERR_OK) {
		if (instInfo != nullptr) {
			if (instInfop != nullptr) {
				*instInfo = *instInfop;
				*flags |= TraceDqr::TRACE_HAVE_INSTINFO;
			}
		}

		if (msgInfo != nullptr) {
			if (msgInfop != nullptr) {
				*msgInfo = *msgInfop;
				*flags |= TraceDqr::TRACE_HAVE_MSGINFO;
			}
		}

		if (srcInfo != nullptr) {
			if (srcInfop != nullptr) {
				*srcInfo = *srcInfop;
				*flags |= TraceDqr::TRACE_HAVE_SRCINFO;
			}
		}
	}

	return ec;
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

TraceDqr::DQErr Trace::NextInstruction(Instruction **instInfo, NexusMessage **msgInfo, Source **srcInfo)
{
	assert(sfp != nullptr);

	TraceDqr::DQErr rc;

//	printf("Instinfo: %08llx, MsgInfo: %08x, srcInfo: %08x\n",instInfo,msgInfo,srcInfo);

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

//		need to set readNewTraceMessage where it is needed! That includes
//		staying in the same state that expects to get another message!!

		if (readNewTraceMessage != false) {
//			rc = linkedNexusMessage::nextTraceMessage(nm);
			rc = sfp->readNextTraceMsg(nm,analytics);

			if (rc != TraceDqr::DQERR_OK) {
				// have an error. either eof, or error

				status = sfp->getErr();

				if (status != TraceDqr::DQERR_EOF) {
					if ( messageSync[currentCore] != nullptr) {
						printf("Error: Trace file does not contain %d trace messages. %d message found\n",startMessageNum,messageSync[currentCore]->lastMsgNum);
					}
					else {
						printf("Error: Trace file does not contain any trace messages, or is unreadable\n");
					}
				}

				return status;
			}

			readNewTraceMessage = false;
			currentCore = nm.coreId;
		}

		switch (state[currentCore]) {
		case TRACE_STATE_GETSTARTTRACEMSG:
//			printf("state TRACE_STATE_GETSTARTTRACEMSG\n");

			// home new nexus message to process

			if (startMessageNum <= 1) {
				// if starting at beginning, just switch to normal state for starting

				state[currentCore] = TRACE_STATE_GETFIRSTYNCMSG;
				break;
			}

			if (messageSync[currentCore] == nullptr) {
				messageSync[currentCore] = new NexusMessageSync;
			}

			switch (nm.tcode) {
			case TraceDqr::TCODE_SYNC:
			case TraceDqr::TCODE_DIRECT_BRANCH_WS:
			case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
				messageSync[currentCore]->msgs[0] = nm;
				messageSync[currentCore]->index = 1;

				messageSync[currentCore]->firstMsgNum = nm.msgNum;
				messageSync[currentCore]->lastMsgNum = nm.msgNum;

				if (nm.msgNum >= startMessageNum) {
					state[currentCore] = TRACE_STATE_COMPUTESTARTINGADDRESS;
				}
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH:
			case TraceDqr::TCODE_INDIRECT_BRANCH:
				if (messageSync[currentCore]->index == 0) {
					if (nm.msgNum >= startMessageNum) {
						// can't start at this trace message because we have not seen a sync yet
						// so we cannot compute the address

						state[currentCore] = TRACE_STATE_ERROR;

						printf("Error: cannot start at trace message %d because no preceeding sync\n",startMessageNum);

						status = TraceDqr::DQERR_ERR;
						return status;
					}

					// first message. Not a sync, so ignore, but read another message
					readNewTraceMessage = true;
				}
				else {
					// stuff it in the list

					messageSync[currentCore]->msgs[messageSync[currentCore]->index] = nm;
					messageSync[currentCore]->index += 1;

					// don't forget to check for messageSync->msgs[] overrun!!

					if (messageSync[currentCore]->index >= (int)(sizeof messageSync[currentCore]->msgs / sizeof messageSync[currentCore]->msgs[0])) {
						status = TraceDqr::DQERR_ERR;
						state[currentCore] = TRACE_STATE_ERROR;

						return status;
					}

					messageSync[currentCore]->lastMsgNum = nm.msgNum;

					if (nm.msgNum >= startMessageNum) {
						state[currentCore] = TRACE_STATE_COMPUTESTARTINGADDRESS;
					}
				}
				break;
			case TraceDqr::TCODE_CORRELATION:
				// we are leaving trace mode, so we no longer know address we are at until
				// we see a sync message, so set index to 0 to start over

				messageSync[currentCore]->index = 0;
				break;
			case TraceDqr::TCODE_AUXACCESS_WRITE:
			case TraceDqr::TCODE_OWNERSHIP_TRACE:
			case TraceDqr::TCODE_ERROR:
				// these message types we just stuff in the list incase we are interested in the
				// information later

				messageSync[currentCore]->msgs[messageSync[currentCore]->index] = nm;
				messageSync[currentCore]->index += 1;

				// don't forget to check for messageSync->msgs[] overrun!!

				if (messageSync[currentCore]->index >= (int)(sizeof messageSync[currentCore]->msgs / sizeof messageSync[currentCore]->msgs[0])) {
					status = TraceDqr::DQERR_ERR;
					state[currentCore] = TRACE_STATE_ERROR;

					return status;
				}

				messageSync[currentCore]->lastMsgNum = nm.msgNum;

				if (nm.msgNum >= startMessageNum) {
					state[currentCore] = TRACE_STATE_COMPUTESTARTINGADDRESS;
				}
				break;
			default:
				state[currentCore] = TRACE_STATE_ERROR;

				status = TraceDqr::DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_COMPUTESTARTINGADDRESS:
//			printf("state TRACE_STATE_COMPUTSTARTINGADDRESS\n");

			// compute address from trace message queued up in messageSync->msgs

			for (int i = 0; i < messageSync[currentCore]->index; i++) {
				switch (messageSync[currentCore]->msgs[i].tcode) {
				case TraceDqr::TCODE_DIRECT_BRANCH:
					// need to get the direct branch instruction so we can compute the next address.
					// Instruction should be at currentAddress - 1 or -2, which is the last known address)
					// plus the i-cnt in this trace message (i-cnt is the number of 16 byte blocks in this
					// span of insturctions between last trace message and this trace message) - 1 if the
					// last instruction in the block is 16 bits, or -2 if the last instruction is 32 bits.
					// The problem is we don't know, and checking the length bits in the last two
					// 16 bit words will not always work. So we just compute instruction sizes from
					// the last known address to this trace message to find the last instruciton.

					TraceDqr::ADDRESS addr;

					addr = currentAddress[currentCore];

					i_cnt[currentCore] = messageSync[currentCore]->msgs[i].directBranch.i_cnt;

					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = lastTime[currentCore] ^ messageSync[currentCore]->msgs[i].timestamp;
					}

					while (i_cnt[currentCore] > 0) {
						status = elfReader->getInstructionByAddress(addr,inst);
						if (status != TraceDqr::DQERR_OK) {
							printf("Error: getInstructionByAddress failed\n");

							printf("Addr: %08x\n",addr);

							state[currentCore] = TRACE_STATE_ERROR;

							return status;
						}

						// figure out how big the instruction is

						int rc;

						rc = decodeInstructionSize(inst,inst_size);
						if (rc != 0) {
							printf("Error: Cann't decode size of instruction %04x\n",inst);

							state[currentCore] = TRACE_STATE_ERROR;

							status = TraceDqr::DQERR_ERR;
							return status;
						}

						switch (inst_size) {
						case 16:
							i_cnt[currentCore] -= 1;
							if (i_cnt[currentCore] > 0) {
								addr += 2;
							}
							break;
						case 32:
							i_cnt[currentCore] -= 2;
							if (i_cnt[currentCore] > 0) {
								addr += 4;
							}
							break;
						default:
							printf("Error: unsupported instruction size: %d\n",inst_size);

							state[currentCore] = TRACE_STATE_ERROR;

							status = TraceDqr::DQERR_ERR;
							return status;
						}
					}

					decodeInstruction(inst,inst_size,inst_type,immeadiate,is_branch);

					if (is_branch == false) {
						status = TraceDqr::DQERR_ERR;

						state[currentCore] = TRACE_STATE_ERROR;

						return status;
					}

					switch (inst_type) {
					case TraceDqr::INST_JAL:
					case TraceDqr::INST_BEQ:
					case TraceDqr::INST_BNE:
					case TraceDqr::INST_BLT:
					case TraceDqr::INST_BGE:
					case TraceDqr::INST_BLTU:
					case TraceDqr::INST_BGEU:
					case TraceDqr::INST_C_J:
					case TraceDqr::INST_C_JAL:
					case TraceDqr::INST_C_BEQZ:
					case TraceDqr::INST_C_BNEZ:
						currentAddress[currentCore] = addr + immeadiate;
						break;
					default:
						printf("Error: Bad instruction type in state TRACE_STATE_COMPUTESTARTINGADDRESS. TCODE_DIRECT_BRANCH: %d\n",inst_type);

						state[currentCore] = TRACE_STATE_ERROR;
						status = TraceDqr::DQERR_ERR;
						return status;
					}
					break;
				case TraceDqr::TCODE_INDIRECT_BRANCH:
					lastFaddr[currentCore] = lastFaddr[currentCore] ^ (messageSync[currentCore]->msgs[i].indirectBranch.u_addr << 1);
					currentAddress[currentCore] = lastFaddr[currentCore];
					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = lastTime[currentCore] ^ messageSync[currentCore]->msgs[i].timestamp;
					}
					break;
				case TraceDqr::TCODE_SYNC:
					lastFaddr[currentCore] = messageSync[currentCore]->msgs[i].sync.f_addr << 1;
					currentAddress[currentCore] = lastFaddr[currentCore];
					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = messageSync[currentCore]->msgs[i].timestamp;
					}
					break;
				case TraceDqr::TCODE_DIRECT_BRANCH_WS:
					lastFaddr[currentCore] = messageSync[currentCore]->msgs[i].directBranchWS.f_addr << 1;
					currentAddress[currentCore] = lastFaddr[currentCore];
					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = messageSync[currentCore]->msgs[i].timestamp;
					}
					break;
				case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
					lastFaddr[currentCore] = messageSync[currentCore]->msgs[i].indirectBranchWS.f_addr << 1;
					currentAddress[currentCore] = lastFaddr[currentCore];
					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = messageSync[currentCore]->msgs[i].timestamp;
					}
					break;
				case TraceDqr::TCODE_CORRELATION:
					printf("Error: should never see this!!\n");
					state[currentCore] = TRACE_STATE_ERROR;

					status = TraceDqr::DQERR_ERR;
					return status;
				case TraceDqr::TCODE_AUXACCESS_WRITE:
				case TraceDqr::TCODE_OWNERSHIP_TRACE:
				case TraceDqr::TCODE_ERROR:
					// just skip these for now. Later we will add additional support for them,
					// such as handling the error or setting the process ID from the message
					if (messageSync[currentCore]->msgs[i].haveTimestamp) {
						lastTime[currentCore] = lastTime[currentCore] ^ messageSync[currentCore]->msgs[i].timestamp;
					}
					break;
				default:
					state[currentCore] = TRACE_STATE_ERROR;

					status = TraceDqr::DQERR_ERR;
					return status;
				}
			}

			readNewTraceMessage = true;
			state[currentCore] = TRACE_STATE_GETNEXTMSG;

			if ((msgInfo != nullptr) && (messageSync[currentCore]->index > 0)) {
				messageInfo = messageSync[currentCore]->msgs[messageSync[currentCore]->index-1];
				messageInfo.currentAddress = currentAddress[currentCore];
				messageInfo.time = lastTime[currentCore];

				if (messageInfo.processITCPrintData(itcPrint) == false) {
					*msgInfo = &messageInfo;

					status = TraceDqr::DQERR_OK;
					return status;
				}
			}
			break;
		case TRACE_STATE_GETFIRSTYNCMSG:
//			printf("state TRACE_STATE_GETFIRSTSYNCMSG\n");

			// read trace messages until a sync is found. Should be the first message normally

			// only exit this state when sync type message is found

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state[currentCore] = TRACE_STATE_DONE;
				status = TraceDqr::DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TraceDqr::TCODE_SYNC:
				nm.sync.i_cnt = 0; // just making sure on first sync message
				lastFaddr[currentCore] = nm.sync.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETSECONDMSG;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH_WS:
				nm.directBranchWS.i_cnt = 0; // just making sure on first sync message
				lastFaddr[currentCore] = nm.directBranchWS.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETSECONDMSG;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
				nm.indirectBranchWS.i_cnt = 0; // just making sure on first sync message
				lastFaddr[currentCore] = nm.indirectBranchWS.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETSECONDMSG;
				break;
			default:
				// if we haven't found a sync message yet, keep the state the same so it will
				// kepp looking for a sync. Could also set he process ID if seen and handle
				// any TCODE_ERROR message

				// until we find our first sync, we don't know the time (need a full timestamp)

				readNewTraceMessage = true;

				nm.timestamp = 0;
				lastTime[currentCore] = 0;
				break;
			}

			// here we return the trace messages before we have actually started tracing
			// this could be at the start of a trace, or after leaving a trace bec ause of
			// a correlation message

			if (msgInfo != nullptr) {
				messageInfo = nm;
				messageInfo.currentAddress = currentAddress[currentCore];
				messageInfo.time = lastTime[currentCore];

				if (messageInfo.processITCPrintData(itcPrint) == false) {
					*msgInfo = &messageInfo;
				}
			}

			status = TraceDqr::DQERR_OK;
			return status;
		case TRACE_STATE_GETSECONDMSG:
//			printf("state TRACE_STATE_GETSECONDMSG\n");

			// only message with i-cnt will release from this state

			// return any message without an i-cnt

			// do not return message with i-cnt; process them when i-cnt expires

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state[currentCore] = TRACE_STATE_DONE;
				status = TraceDqr::DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TraceDqr::TCODE_SYNC:
				i_cnt[currentCore] = nm.sync.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH_WS:
				i_cnt[currentCore] = nm.directBranchWS.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
				i_cnt[currentCore] = nm.indirectBranchWS.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH:
				i_cnt[currentCore] = nm.directBranch.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH:
				i_cnt[currentCore] = nm.indirectBranch.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_CORRELATION:
				i_cnt[currentCore] = nm.correlation.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_AUXACCESS_WRITE:
			case TraceDqr::TCODE_OWNERSHIP_TRACE:
			case TraceDqr::TCODE_ERROR:
				// these message have no address or i.cnt info, so we still need to get
				// another message.

				// might want to keep track of press, but will add that later

				// for now, return message;

				if (nm.haveTimestamp) {
					lastTime[currentCore] = lastTime[currentCore] ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				return status;
			default:
				printf("Error: bad tcode type in sate TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH\n");

				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_RETIREMESSAGE:
//			printf("state TRACE_STATE_RETIREMESSAGE\n");

			// Process message being retired (currently in nm) i_cnt has gone to 0

			switch (nm.tcode) {
			case TraceDqr::TCODE_SYNC:
				lastFaddr[currentCore] = nm.sync.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETNEXTMSG;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH_WS:
				lastFaddr[currentCore] = nm.directBranchWS.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETNEXTMSG;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
				lastFaddr[currentCore] = nm.indirectBranchWS.f_addr << 1;
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETNEXTMSG;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH:
				// note: inst is set in the state before this state (TRACE_STATE_GETNEXTINSTRUCTION)
				// instr must be valid to get to this point, so use it for computing new address

				decodeInstruction(inst,inst_size,inst_type,immeadiate,is_branch);

				if (nm.haveTimestamp) {
					lastTime[currentCore] = lastTime[currentCore] ^ nm.timestamp;
				}

				switch (inst_type) {
				case TraceDqr::INST_JAL:
				case TraceDqr::INST_BEQ:
				case TraceDqr::INST_BNE:
				case TraceDqr::INST_BLT:
				case TraceDqr::INST_BGE:
				case TraceDqr::INST_BLTU:
				case TraceDqr::INST_BGEU:
				case TraceDqr::INST_C_J:
				case TraceDqr::INST_C_JAL:
				case TraceDqr::INST_C_BEQZ:
				case TraceDqr::INST_C_BNEZ:
					currentAddress[currentCore] = currentAddress[currentCore] + immeadiate;

					if (msgInfo != nullptr) {
						messageInfo = nm;
						messageInfo.time = lastTime[currentCore];
						messageInfo.currentAddress = currentAddress[currentCore];

						if (messageInfo.processITCPrintData(itcPrint) == false) {
							*msgInfo = &messageInfo;
						}
					}

					// Do not return info yet. Cycle again untill we fill in
					// an inst data struct as well

					break;
				default:
					printf("inst: %08x, size: %d, type: %d, imeadiate: %08x, is_branch: %d\n",inst,inst_size,inst_type,immeadiate,is_branch);
					printf("Error: Bad instruction type in state TRACE_STATE_RETIREMESSAGE:TCODE_DIRECT_BRANCH: %d\n",inst_type);

					state[currentCore] = TRACE_STATE_ERROR;
					status = TraceDqr::DQERR_ERR;
					return status;
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETNEXTMSG;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH:
				lastFaddr[currentCore] = lastFaddr[currentCore] ^ (nm.indirectBranch.u_addr << 1);
				currentAddress[currentCore] = lastFaddr[currentCore];
				if (nm.haveTimestamp) {
					lastTime[currentCore] = lastTime[currentCore] ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETNEXTMSG;
				break;
			case TraceDqr::TCODE_CORRELATION:

				// correlation has i_cnt, but no address info

				// leaving trace mode - need to get next sync

				if (nm.haveTimestamp) {
					lastTime[currentCore] = lastTime[currentCore] ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
//					messageInfo.currentAddress = currentAddress;
					messageInfo.currentAddress = lastFaddr[currentCore] + nm.correlation.i_cnt*2;

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				readNewTraceMessage = true;
				state[currentCore] = TRACE_STATE_GETFIRSTYNCMSG;
				break;
			case TraceDqr::TCODE_AUXACCESS_WRITE:
			case TraceDqr::TCODE_OWNERSHIP_TRACE:
			case TraceDqr::TCODE_ERROR:
				// these messages have no address or i-cnt info and should have been
				// instantly retired when they were read.

				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			default:
				printf("Error: bad tcode type in sate TRACE_STATE_GETNEXTMSG.TCODE_DIRECT_BRANCH\n");

				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			}

			status = TraceDqr::DQERR_OK;
			return status;
		case TRACE_STATE_GETNEXTMSG:
//			printf("state TRACE_STATE_GETNEXTMSG\n");

			// exit this state when message with i-cnt is read

			if ((endMessageNum != 0) && (nm.msgNum > endMessageNum)) {
				printf("stopping at trace message %d\n",endMessageNum);

				state[currentCore] = TRACE_STATE_DONE;
				status = TraceDqr::DQERR_DONE;
				return status;
			}

			switch (nm.tcode) {
			case TraceDqr::TCODE_DIRECT_BRANCH:
				i_cnt[currentCore] = nm.directBranch.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH:
				i_cnt[currentCore] = nm.indirectBranch.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_SYNC:
				i_cnt[currentCore] = nm.sync.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_DIRECT_BRANCH_WS:
				i_cnt[currentCore] = nm.directBranchWS.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_INDIRECT_BRANCH_WS:
				i_cnt[currentCore] = nm.indirectBranchWS.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_CORRELATION:
				i_cnt[currentCore] = nm.correlation.i_cnt;
				state[currentCore] = TRACE_STATE_GETNEXTINSTRUCTION;
				break;
			case TraceDqr::TCODE_AUXACCESS_WRITE:
			case TraceDqr::TCODE_DATA_ACQUISITION:
			case TraceDqr::TCODE_OWNERSHIP_TRACE:
			case TraceDqr::TCODE_ERROR:
				// retire these instantly by returning them through msgInfo

				if (nm.haveTimestamp) {
					lastTime[currentCore] = lastTime[currentCore] ^ nm.timestamp;
				}

				if (msgInfo != nullptr) {
					messageInfo = nm;
					messageInfo.time = lastTime[currentCore];
					messageInfo.currentAddress = currentAddress[currentCore];

					if (messageInfo.processITCPrintData(itcPrint) == false) {
						*msgInfo = &messageInfo;
					}
				}

				// leave state along. Need to get another message with an i-cnt!

				readNewTraceMessage = true;

				return status;
			default:
				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			}
			break;
		case TRACE_STATE_GETNEXTINSTRUCTION:
//			printf("Trace::NextInstruction():TRACE_STATE_GETNEXTINSTRUCTION\n");

			// get instruction at addr

			TraceDqr::ADDRESS addr;

			addr = currentAddress[currentCore];

//		    where to put file object? Should above be part of file object? Make a section object??

//		    need to have a list of sections of code. lookup section based on address, use that section for symbol
//			and line number lookup.
//          also, read in file by line and keep an array of lines. Need to handle the #line and #file directrives
//			check if objdump follows #line and #file preprocessor directives (sure it will)

			status = elfReader->getInstructionByAddress(addr,inst);
			if (status != TraceDqr::DQERR_OK) {
				printf("Error: getInstructionByAddress failed\n");

				printf("Addr2: %08x\n",addr);

				state[currentCore] = TRACE_STATE_ERROR;

				return status;
			}

			// figure out how big the instruction is

			int rc;

			rc = decodeInstructionSize(inst,inst_size);
			if (rc != 0) {
				printf("Error: Cann't decode size of instruction %04x\n",inst);

				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			}

			status = analytics.updateInstructionInfo(currentCore,inst,inst_size);
			if (status != TraceDqr::DQERR_OK) {
				state[currentCore] = TRACE_STATE_ERROR;
				return status;
			}

			switch (inst_size) {
			case 16:
				i_cnt[currentCore] -= 1;
				if (i_cnt[currentCore] > 0) {
					currentAddress[currentCore] += 2;
				}
				break;
			case 32:
				i_cnt[currentCore] -= 2;
				if (i_cnt[currentCore] > 0) {
					currentAddress[currentCore] += 4;
				}
				break;
			default:
				printf("Error: unsupported instruction size: %d\n",inst_size);
				state[currentCore] = TRACE_STATE_ERROR;
				status = TraceDqr::DQERR_ERR;
				return status;
			}

			// calling disassemble() fills in the instructionInfo object

			Disassemble(addr);

			if (instInfo != nullptr) {
				instructionInfo.coreId = currentCore;
				*instInfo = &instructionInfo;
			}

			if (srcInfo != nullptr) {
				sourceInfo.coreId = currentCore;
				*srcInfo = &sourceInfo;
			}

			if (i_cnt[currentCore] <= 0) {
				// update addr based on saved nexus message
				// get next nexus message

				state[currentCore] = TRACE_STATE_RETIREMESSAGE;

				// note: break below runs that state machine agaain and does not fall through
				// to code below if ()

				break;
			}

			status = TraceDqr::DQERR_OK;
			return status;
		case TRACE_STATE_DONE:
//			printf("Trace::NextInstruction():TRACE_STATE_DONE\n");
			status = TraceDqr::DQERR_DONE;
			return status;
		case TRACE_STATE_ERROR:
//			printf("Trace::NextInstruction():TRACE_STATE_ERROR\n");

			status = TraceDqr::DQERR_ERR;
			return status;
		default:
			printf("Error: Trace::NextInstruction():unknown\n");

			state[currentCore] = TRACE_STATE_ERROR;
			status = TraceDqr::DQERR_ERR;
			return status;
		}
	}

	status = TraceDqr::DQERR_OK;
	return TraceDqr::DQERR_OK;
}

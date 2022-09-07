/* Copyright 2022 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include "dqr.hpp"
#include "trace.hpp"

static inline uint16_t bswap_uint16( uint16_t val )
{
    return (val << 8) | (val >> 8);
}

static inline uint64_t bswap_uint64( uint64_t val )
{
    val = ((val << 8)  & 0xFF00FF00FF00FF00ULL ) | ((val >> 8)  & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}

VCD::VCD(const char *vcd_name,const char *ef_name,const char *odExe)
{
	TraceDqr::DQErr rc;

	elfReader = nullptr;
	disassembler = nullptr;
	vcdBuff = nullptr;

	totalPCDRecords = 0;
	nextPCDRecord = 0;

	TraceSettings ts;
	ts.propertyToPFName(vcd_name);
	ts.propertyToEFName(ef_name);
	ts.propertyToObjdumpName(odExe);

	rc = configure(ts);
	if (rc != TraceDqr::DQERR_OK) {
		cleanUp();

		status = TraceDqr::DQERR_ERR;
	}
	else {
		status = TraceDqr::DQERR_OK;
	}
}

VCD::VCD(const char *pf_name)
{
	TraceDqr::DQErr rc;

	elfReader = nullptr;
	disassembler = nullptr;
	vcdBuff = nullptr;

	if (pf_name == nullptr) {
		printf("Error: VCD(): pf_name argument null\n");

		status = TraceDqr::DQERR_ERR;
		return;
	}

	propertiesParser properties(pf_name);

	rc = properties.getStatus();
	if (rc != TraceDqr::DQERR_OK) {
		printf("Error: VCD(): new propertiesParser(%s) from file failed with %d\n",pf_name,rc);

		cleanUp();

		status = rc;
		return;
	}

	TraceSettings settings;

	rc = settings.addSettings(&properties);
	if (rc != TraceDqr::DQERR_OK) {
		printf("Error: VCD(): addSettings() failed\n");

		cleanUp();

		status = rc;
		return;
	}

	rc = configure(settings);
	if (rc != TraceDqr::DQERR_OK) {
		cleanUp();

		status = rc;
		return;
	}

	status = TraceDqr::DQERR_OK;
}

VCD::~VCD()
{
	cleanUp();
}

TraceDqr::DQErr VCD::configure(class TraceSettings &settings)
{
	TraceDqr::DQErr rc;

	status = TraceDqr::DQERR_OK;

	elfReader = nullptr;
	disassembler = nullptr;
	vcdBuff = nullptr;

	haveLookaheadVRec = false;
	numVCDRecords = 0;
	currentVCDRecord = 0;

	if (settings.pfName == nullptr) {
		printf("Error: VCD::configure(): No trace file name specified\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	pathType = settings.pathType;

	if (settings.pfName != nullptr) {
#ifdef WINDOWS
                vcd_fd = open(settings.pfName,O_RDONLY | O_BINARY);
#else
                vcd_fd = open(settings.pfName,O_RDONLY);
#endif
		if (vcd_fd < 0) {
			printf("Error: VCD::configure(): Could not open PCD file %s for input\n",settings.pfName);

			status = TraceDqr::DQERR_ERR;
			return TraceDqr::DQERR_ERR;
		}

		// git size of PCD file

		off_t size;

		size = lseek(vcd_fd,(off_t)0,SEEK_END);
		lseek(vcd_fd,(off_t)0,SEEK_SET);

		totalPCDRecords = (int)(size/(off_t)(sizeof lookaheadVRec.ts + sizeof lookaheadVRec.flags + sizeof lookaheadVRec.pc));
	}
	else {
		printf("Error: VCD::configure(): Must specify a VCD file\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	vcdBuff = new uint8_t [1000*(sizeof(uint64_t)+sizeof(uint16_t)+sizeof(uint64_t))];
	if (vcdBuff == nullptr) {
		printf("Error: VCD::configure(): Could not allocate vcdBuff\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	if (settings.efName != nullptr ) {

		// create elf object - this also forks off objdump and parses the elf file

		char *objdump;

		objdump = nullptr;

		if (settings.odName != nullptr) {
			int len;

			len = strlen(settings.odName);

			objdump = new char[len+1];

			strcpy(objdump,settings.odName);
		}
		else {
			objdump = new char [sizeof DEFAULTOBJDUMPNAME + 1];
			strcpy(objdump,DEFAULTOBJDUMPNAME);
		}

		elfReader = new (std::nothrow) ElfReader(settings.efName,objdump);

		delete [] objdump;
		objdump = nullptr;

		if (elfReader == nullptr) {
			printf("Error: VCD::Configure(): Could not create ElfReader object\n");

			status = TraceDqr::DQERR_ERR;
			return TraceDqr::DQERR_ERR;
		}

	    if (elfReader->getStatus() != TraceDqr::DQERR_OK) {
	    	status = TraceDqr::DQERR_ERR;
	    	return TraceDqr::DQERR_ERR;
	    }

		archSize = elfReader->getArchSize();

	    // get symbol table

	    Symtab *symtab;
	    Section *sections;

	    symtab = elfReader->getSymtab();
	    if (symtab == nullptr) {
	    	status = TraceDqr::DQERR_ERR;
	    	return TraceDqr::DQERR_ERR;
	    }

	    sections = elfReader->getSections();
	    if (sections == nullptr) {
	    	status = TraceDqr::DQERR_ERR;
	    	return TraceDqr::DQERR_ERR;
	    }

	    // create disassembler object

		disassembler = new (std::nothrow) Disassembler(symtab,sections,elfReader->getArchSize());
		if (disassembler == nullptr) {
			printf("Error: VCD::Configure(): Could not create disassembler object\n");

			status = TraceDqr::DQERR_ERR;
			return TraceDqr::DQERR_ERR;
		}

		if (disassembler->getStatus() != TraceDqr::DQERR_OK) {
			status = TraceDqr::DQERR_ERR;
			return TraceDqr::DQERR_ERR;
		}

		rc = disassembler->setPathType(settings.pathType);
		if (rc != TraceDqr::DQERR_OK) {
			status = rc;
			return rc;
		}
	}
	else {
		printf("Error: VCD::configure(): Must specify elf file name\n");
		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	instructionInfo.CRFlag = TraceDqr::isNone;
	instructionInfo.brFlags = TraceDqr::BRFLAG_none;

	instructionInfo.address = 0;
	instructionInfo.instruction = 0;
	instructionInfo.instSize = 0;

	if (settings.numAddrBits != 0 ) {
		instructionInfo.addrSize = settings.numAddrBits;
	}
	else if (elfReader == nullptr) {
		instructionInfo.addrSize = 0;
	}
	else {
		instructionInfo.addrSize = elfReader->getBitsPerAddress();
	}

	instructionInfo.addrDispFlags = settings.addrDispFlags;

	instructionInfo.addrPrintWidth = (instructionInfo.addrSize + 3) / 4;

	instructionInfo.addressLabel = nullptr;
	instructionInfo.addressLabelOffset = 0;

	instructionInfo.timestamp = 0;
	instructionInfo.caFlags = TraceDqr::CAFLAG_NONE;
	instructionInfo.pipeCycles = 0;
	instructionInfo.VIStartCycles = 0;
	instructionInfo.VIFinishCycles = 0;

	sourceInfo.sourceFile = nullptr;
	sourceInfo.sourceFunction = nullptr;
	sourceInfo.sourceLineNum = 0;
	sourceInfo.sourceLine = nullptr;

	for (int i = 0; (size_t)i < sizeof enterISR / sizeof enterISR[0]; i++) {
		enterISR[i] = TraceDqr::isNone;
	}

	if ((settings.cutPath != nullptr) || (settings.srcRoot != nullptr)) {
		rc = subSrcPath(settings.cutPath,settings.srcRoot);
		if (rc != TraceDqr::DQERR_OK) {
			status = rc;
			return status;
		}
	}

	for (int i = 0; (size_t)i < sizeof currentTime / sizeof currentTime[0]; i++) {
		currentTime[i] = 0;
	}

	return TraceDqr::DQERR_OK;
}

void VCD::cleanUp()
{
	if (vcd_fd >= 0) {
		close(vcd_fd);
		vcd_fd = -1;
	}

	if (vcdBuff != nullptr) {
		delete [] vcdBuff;
		vcdBuff = nullptr;
	}

	numVCDRecords = 0;

	if (elfReader != nullptr) {
		delete elfReader;
		elfReader = nullptr;
	}

	if (disassembler != nullptr) {
		delete disassembler;
		disassembler = nullptr;
	}
}

TraceDqr::DQErr VCD::getNextVRec(VRec &vrec)
{
	if (vcd_fd < 0) {
		printf("Error: VCD::readNextVRec(): Invalid VCD File Descriptor\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	int r;

	if (currentVCDRecord >= numVCDRecords) {
		// need to read another buff

		r = read(vcd_fd,vcdBuff,1000 * (sizeof vrec.ts + sizeof vrec.flags + sizeof vrec.pc));
		if (r < 0) {
			printf("Error: VCD::getNextVCDRec(): Error reading vcd file\n");
			status = TraceDqr::DQERR_ERR;
			return TraceDqr::DQERR_ERR;
		}

		numVCDRecords = r / (sizeof vrec.ts + sizeof vrec.flags + sizeof vrec.pc);

		if (numVCDRecords == 0) {
			status = TraceDqr::DQERR_EOF;
			return TraceDqr::DQERR_EOF;
		}

		currentVCDRecord = 0;
	}

	vrec.ts = bswap_uint64(*(uint64_t*)&vcdBuff[currentVCDRecord*(sizeof vrec.ts+sizeof vrec.flags + sizeof vrec.pc)]);
	vrec.flags = bswap_uint16(*(uint16_t*)&vcdBuff[currentVCDRecord*(sizeof vrec.ts+sizeof vrec.flags + sizeof vrec.pc)+sizeof vrec.ts]);
	vrec.pc = bswap_uint64(*(uint64_t*)&vcdBuff[currentVCDRecord*(sizeof vrec.ts+sizeof vrec.flags + sizeof vrec.pc)+sizeof vrec.ts+sizeof vrec.flags]);

	currentVCDRecord += 1;

	nextPCDRecord += 1;

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr VCD::subSrcPath(const char *cutPath,const char *newRoot)
{
	if (disassembler != nullptr) {
		TraceDqr::DQErr rc;

		rc = disassembler->subSrcPath(cutPath,newRoot);

		status = rc;
		return rc;
	}

	status = TraceDqr::DQERR_ERR;
	return TraceDqr::DQERR_ERR;
}

TraceDqr::DQErr VCD::computeBranchFlags(int core,TraceDqr::ADDRESS currentAddr,uint32_t currentInst, TraceDqr::ADDRESS nextAddr,int &crFlag,TraceDqr::BranchFlags &brFlag)
{
	int inst_size;
	TraceDqr::InstType inst_type;
	int32_t immediate;
	bool isBranch;
	int rc;
	TraceDqr::Reg rs1;
	TraceDqr::Reg rd;

	brFlag = TraceDqr::BRFLAG_none;
	crFlag = enterISR[core];
	enterISR[core] = 0;

	// figure out how big the instruction is
	// Note: immediate will already be adjusted - don't need to mult by 2 before adding to address

	rc = Disassembler::decodeInstruction(currentInst,archSize,inst_size,inst_type,rs1,rd,immediate,isBranch);
	if (rc != 0) {
		printf("Error: computeBranchFlags(): Cannot decode instruction %04x\n",currentInst);

		status = TraceDqr::DQERR_ERR;

		return status;
	}

	switch (inst_type) {
	case TraceDqr::INST_UNKNOWN:
		if ((nextAddr != 0) && ((currentAddr + inst_size/8) != nextAddr)) {
			enterISR[core] = TraceDqr::isInterrupt;
		}
		break;
	case TraceDqr::INST_JAL:
		// rd = pc+4 (rd can be r0)
		// pc = pc + (sign extended immediate offset)
		// plain unconditional jumps use rd -> r0
		// inferrable unconditional

		if ((rd == TraceDqr::REG_1) || (rd == TraceDqr::REG_5)) { // rd == link
			crFlag |= TraceDqr::isCall;
		}

		if ((nextAddr != 0) && (currentAddr + immediate != nextAddr)) {
			enterISR[core] = TraceDqr::isInterrupt;
		}
		break;
	case TraceDqr::INST_JALR:
		// rd = pc+4 (rd can be r0)
		// pc = pc + ((sign extended immediate offset) + rs) & 0xffe
		// plain unconditional jumps use rd -> r0
		// not inferrable unconditional

		if ((rd == TraceDqr::REG_1) || (rd == TraceDqr::REG_5)) { // rd == link
			if ((rs1 != TraceDqr::REG_1) && (rs1 != TraceDqr::REG_5)) { // rd == link; rs1 != link
				crFlag |= TraceDqr::isCall;
			}
			else if (rd != rs1) { // rd == link; rs1 == link; rd != rs1
				crFlag |= TraceDqr::isSwap;
			}
			else { // rd == link; rs1 == link; rd == rs1
				crFlag |= TraceDqr::isCall;
			}
		}
		else if ((rs1 == TraceDqr::REG_1) || (rs1 == TraceDqr::REG_5)) { // rd != link; rs1 == link
			crFlag |= TraceDqr::isReturn;
		}

		// without more info, don't know if enterISR should be set or not, so we leave it alone for now
		// could include register info and then we could tell
		break;
	case TraceDqr::INST_BEQ:
	case TraceDqr::INST_BNE:
	case TraceDqr::INST_BLT:
	case TraceDqr::INST_BGE:
	case TraceDqr::INST_BLTU:
	case TraceDqr::INST_BGEU:
	case TraceDqr::INST_C_BEQZ:
	case TraceDqr::INST_C_BNEZ:
		// pc = pc + (sign extend immediate offset) (BLTU and BGEU are not sign extended)
		// inferrable conditional

		if (nextAddr != 0) {
			if (nextAddr == (currentAddr + inst_size / 8)) {
				brFlag = TraceDqr::BRFLAG_notTaken;
			}
			else if (nextAddr == (currentAddr + immediate)) {
				brFlag = TraceDqr::BRFLAG_taken;
			}
			else {
				enterISR[core] = TraceDqr::isInterrupt;
			}
		}
		break;
	case TraceDqr::INST_C_J:
		// pc = pc + (signed extended immediate offset)
		// inferrable unconditional

		if ((nextAddr != 0) && (currentAddr + immediate != nextAddr)) {
			enterISR[core] = TraceDqr::isInterrupt;
		}
		break;
	case TraceDqr::INST_C_JAL:
		// btm, htm same

		// x1 = pc + 2
		// pc = pc + (signed extended immediate offset)
		// inferrable unconditional

		if ((rd == TraceDqr::REG_1) || (rd == TraceDqr::REG_5)) { // rd == link
			crFlag |= TraceDqr::isCall;
		}

		if ((nextAddr != 0) && (currentAddr + immediate != nextAddr)) {
			enterISR[core] = TraceDqr::isInterrupt;
		}
		break;
	case TraceDqr::INST_C_JR:
		// pc = pc + rs1
		// not inferrable unconditional

		if ((rs1 == TraceDqr::REG_1) || (rs1 == TraceDqr::REG_5)) {
			crFlag |= TraceDqr::isReturn;
		}

		// without more info, don't know if enterISR should be set or not, so we leave it alone for now
		// could include register info and then we could tell
		break;
	case TraceDqr::INST_C_JALR:
		// x1 = pc + 2
		// pc = pc + rs1
		// not inferrble unconditional

		if (rs1 == TraceDqr::REG_5) {
			crFlag |= TraceDqr::isSwap;
		}
		else {
			crFlag |= TraceDqr::isCall;
		}

		// without more info, don't know if enterISR should be set or not, so we leave it alone for now
		// could include register info and then we could tell
		break;
	case TraceDqr::INST_EBREAK:
	case TraceDqr::INST_ECALL:
		crFlag |= TraceDqr::isException;

		// without more info, don't know if enterISR should be set or not, so we leave it alone for now
		// could include register info and then we could tell
		break;
	case TraceDqr::INST_MRET:
	case TraceDqr::INST_SRET:
	case TraceDqr::INST_URET:
		crFlag |= TraceDqr::isExceptionReturn;

		// without more info, don't know if enterISR should be set or not, so we leave it alone for now
		// could include register info and then we could tell
		break;
	default:
		if ((nextAddr != 0) && ((currentAddr + inst_size/8) != nextAddr)) {
			enterISR[core] = TraceDqr::isInterrupt;
		}
		break;
	}

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr VCD::getTraceFileOffset(int &size,int &offset)
{
	size = totalPCDRecords;
	offset = nextPCDRecord;

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr VCD::buildInstructionFromVRec(VRec *vrec,uint32_t inst,TraceDqr::BranchFlags brFlags,int crFlag)
{
	// at this point we have two srecs for same core

	TraceDqr::DQErr rc;

	rc = Disassemble(vrec->pc);
	if (rc != TraceDqr::DQERR_OK) {
		return TraceDqr::DQERR_ERR;
	}

	if (vrec->flags & VCDFLAG_PIPE) {
		instructionInfo.caFlags = TraceDqr::CAFLAG_PIPE0;
	}
	else {
		instructionInfo.caFlags = TraceDqr::CAFLAG_PIPE1;
	}

	instructionInfo.brFlags = brFlags;
	instructionInfo.CRFlag = crFlag;

	instructionInfo.timestamp = vrec->ts;

	instructionInfo.r0Val = 0;
	instructionInfo.r1Val = 0;
	instructionInfo.wVal = 0;

	if (currentTime[0] == 0) {
		instructionInfo.pipeCycles = 0;
	}
	else {
		instructionInfo.pipeCycles = vrec->ts - currentTime[0];
	}

	currentTime[0] = vrec->ts;

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr VCD::Disassemble(uint64_t pc)
{
	TraceDqr::DQErr ec;

	if (disassembler == nullptr) {
		printf("Error: VCD::Disassemble(): No disassembler object\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	ec = disassembler->disassemble(pc);
	if (ec != TraceDqr::DQERR_OK ) {
		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	// the two lines below copy each structure completely. This is probably
	// pretty inefficient, and just returning pointers and using pointers
	// would likely be better

	instructionInfo = disassembler->getInstructionInfo();
	sourceInfo = disassembler->getSourceInfo();

	instructionInfo.coreId = 0;
	sourceInfo.coreId = 0;

	return TraceDqr::DQERR_OK;
}

TraceDqr::DQErr VCD::NextInstruction(Instruction *instInfo,Source *srcInfo, int *flags)
{
	TraceDqr::DQErr ec;

	Instruction  *instInfop = nullptr;
	Source       *srcInfop  = nullptr;

	Instruction  **instInfopp = nullptr;
	Source       **srcInfopp  = nullptr;

	if (instInfo != nullptr) {
		instInfopp = &instInfop;
	}

	if (srcInfo != nullptr) {
		srcInfopp = &srcInfop;
	}

	ec = NextInstruction(instInfopp,srcInfopp);

	*flags = 0;

	if (ec == TraceDqr::DQERR_OK) {
		if (instInfo != nullptr) {
			if (instInfop != nullptr) {
				*instInfo = *instInfop;
				*flags |= TraceDqr::TRACE_HAVE_INSTINFO;
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

TraceDqr::DQErr VCD::NextInstruction(Instruction **instInfo,Source **srcInfo)
{
	TraceDqr::DQErr rc;
	int crFlag = 0;
	TraceDqr::BranchFlags brFlags = TraceDqr::BRFLAG_none;
	VRec nextVRec;

	if (instInfo != nullptr) {
		*instInfo = nullptr;
	}

	if (srcInfo != nullptr) {
		*srcInfo = nullptr;
	}

//first, see if addresses are ever out-of-order!!
//
//	Q must have either one or two look-aheads (depending on time-stamps/pipes). Just make it two lookaheads.
//
//	while q not full:
//	  read next vrec
//	  add to q in heap type fassion - order the three instructions
//
//	extract top of q

//->	need to know the destination of indirect branchs!!!! That must be stuffed into the pcd file!!!
//
//->	need to sort the addresses into proper order!! Sort is not by address value alone!
//
//->	need a three deep q. Fill it by adding vrecs as needed (first time, add three. After that, add 1).
//	every time a rec is added, cmp timestamps. If ==, need to check addresses and stuff to establish order??

	// see if we have a cached lookahead VRec

	if (haveLookaheadVRec) {
		nextVRec = lookaheadVRec;
		haveLookaheadVRec = false;
	}
	else {
		// read next record for any core

		do {
			rc = getNextVRec(nextVRec);
			if (rc != TraceDqr::DQERR_OK) {
				return rc;
			}
		} while (nextVRec.pc == 0);
	}

	// read next record for look ahead

	bool eof;
	eof = false;

	do {
		rc = getNextVRec(lookaheadVRec);
		if (rc != TraceDqr::DQERR_OK) {
			// still have one valid record, so process it

			eof = true;
		}
	} while ((eof == false) && (lookaheadVRec.pc == 0));

	TraceDqr::ADDRESS lookaheadPC;

	if (eof == false) {
		haveLookaheadVRec = true;
		lookaheadPC = lookaheadVRec.pc;
	}
	else {
		haveLookaheadVRec = false;
		lookaheadPC = 0;
	}

	uint32_t inst;

	rc = elfReader->getInstructionByAddress(nextVRec.pc,inst);
	if (rc != TraceDqr::DQERR_OK) {
		printf("Error VCD::NextInstruction(): Could not get instruction at address 0x%08llx\n",nextVRec.pc);

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	rc = computeBranchFlags(0,nextVRec.pc,inst,lookaheadPC,crFlag,brFlags);
	if (rc != TraceDqr::DQERR_OK) {
		printf("Error: VCD:NextInstruction(): could not compute branch flags\n");

		status = TraceDqr::DQERR_ERR;
		return TraceDqr::DQERR_ERR;
	}

	rc = buildInstructionFromVRec(&nextVRec,inst,brFlags,crFlag);

	if (instInfo != nullptr) {
		*instInfo = &instructionInfo;
	}

	if (srcInfo != nullptr) {
		*srcInfo = &sourceInfo;
	}

	return TraceDqr::DQERR_OK;
}

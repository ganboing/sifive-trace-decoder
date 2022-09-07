/* Copyright 2022 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef TRACE_HPP_
#define TRACE_HPP_

// private definitions

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>

#ifdef DO_TIMES
class Timer {
public:
	Timer();
	~Timer();

	double start();
	double etime();

private:
	double startTime;
};
#endif // DO_TIMES

void sanePath(TraceDqr::pathType pt,const char *src,char *dst);

class cachedInstInfo {
public:
	cachedInstInfo(const char *file,int cutPathIndex,const char *func,int linenum,const char *lineTxt,const char *instText,TraceDqr::RV_INST inst,int instSize,const char *addresslabel,int addresslabeloffset);
	~cachedInstInfo();

	void dump();

	const char *filename;
	int         cutPathIndex;
	const char *functionname;
	int         linenumber;
	const char *lineptr;

	TraceDqr::RV_INST instruction;
	int               instsize;

	char             *instructionText;

	const char       *addressLabel;
	int               addressLabelOffset;
};

// class Section: work with elf file sections

class SrcFile {
public:
	SrcFile(char *fName,SrcFile *nxt);
	~SrcFile();

	class SrcFile *next;
	char *file;
};

class SrcFileRoot {
public:
	SrcFileRoot();
	~SrcFileRoot();

	char *addFile(char *fName);
	void dump();

private:
	SrcFile *fileRoot;
};

class Section {
public:

	enum {
		sect_CONTENTS = 1 << 0,
		sect_ALLOC = 1 << 1,
		sect_LOAD = 1 << 2,
		sect_READONLY = 1 << 3,
		sect_DATA = 1 << 4,
		sect_CODE = 1 << 5,
		sect_THREADLOCAL = 1 << 6,
		sect_DEBUGGING = 1 << 7,
		sect_OCTETS = 1 << 8
	};

	Section();
   ~Section();

	Section *getSectionByAddress(TraceDqr::ADDRESS addr);
	Section *getSectionByName(char *secName);

	cachedInstInfo *setCachedInfo(TraceDqr::ADDRESS addr,const char *file,int cutPathIndex,const char *func,int linenum,const char *lineTxt,const char *instTxt,TraceDqr::RV_INST inst,int instSize,const char *addresslabel,int addresslabeloffset);
	cachedInstInfo *getCachedInfo(TraceDqr::ADDRESS addr);

	void dump();

	Section     *next;
	char         name[256];
	TraceDqr::ADDRESS startAddr;
	TraceDqr::ADDRESS endAddr;
	uint32_t     flags;
	uint32_t     size;	// size of section
	uint32_t     offset; // offset of section in elf file
	uint32_t     align;
	uint16_t    *code;
	char       **fName; // file name - array of pointers
	uint32_t    *line;  // line number
	char       **diss;  // disassembly text - array of pointers

	cachedInstInfo **cachedInfo; // array of pointers
};

// class fileReader: Helper class to handler list of source code files

class fileReader {
public:
	struct funcList {
		funcList *next;
		char *func;
	};
	struct fileList {
		fileList     *next;
		char         *name;
		int           cutPathIndex;
		funcList     *funcs;
		unsigned int  lineCount;
		char        **lines;
	};

	fileReader(/*paths?*/);
	~fileReader();

	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);
	fileList *findFile(const char *file);

private:
	char *cutPath;
	char *newRoot;

	fileList *readFile(const char *file);

	fileList *lastFile;
	fileList *files;
};

// class Symtab: Interface class between bfd symbols and what is needed for dqr

struct Sym {
	enum {
		symNone = 0,
		symLocal = 1 << 0,
		symGlobal = 1 << 1,
		symWeak = 1 << 2,
		symConstructor = 1 << 3,
		symIndirect = 1 << 4,
		symIndirectFunc = 1 << 5,
		symDebug = 1 << 6,
		symDynamic = 1 << 7,
		symFunc = 1 << 8,
		symFile = 1 << 9,
		symObj = 1 << 10
	};

	struct Sym *next;
	char *name;
	uint32_t flags;
	class Section *section;
	uint64_t address;
	uint64_t size;
	struct Sym *srcFile;
};

class Symtab {
public:
	             Symtab(Sym *syms);
	            ~Symtab();
	TraceDqr::DQErr lookupSymbolByAddress(TraceDqr::ADDRESS addr,Sym *&sym);
	void         dump();

	TraceDqr::DQErr getStatus() { return status; }

private:
	TraceDqr::DQErr status;

	TraceDqr::ADDRESS cachedSymAddr;
	int cachedSymSize;
	int cachedSymIndex;

	long      numSyms;
    Sym      *symLst;
    Sym     **symPtrArray;

    TraceDqr::DQErr fixupFunctionSizes();
};

// find : Interface class between dqr and bfd

class ObjDump {
public:
		ObjDump(const char *elfName,const char *objDumpPath,int &archSize,Section *&codeSectionLst,Sym *&syms,SrcFileRoot &srcFileRoot);
		~ObjDump();

		TraceDqr::DQErr getStatus() {return status;}

private:
		enum objDumpTokenType {
		    odtt_error,
		    odtt_eol,
		    odtt_eof,
		    odtt_colon,
		    odtt_lt,
		    odtt_gt,
		    odtt_lp,
		    odtt_rp,
		    odtt_comma,
		    odtt_string,
		    odtt_number,
		};

		enum elfType {
			elfType_unknown,
		    elfType_64_little,
		    elfType_32_little,
		};

		enum line_t {
			line_t_label,
			line_t_diss,
			line_t_path,
			line_t_func,
		};

		TraceDqr::DQErr status;

		int stdoutPipe;
		FILE *fpipe;

		bool pipeEOF;
		char pipeBuffer[2048];
		int  pipeIndex = 0;
		int  endOfBuffer = 0;

		pid_t objdumpPid;

		TraceDqr::DQErr execObjDump(const char *elfName,const char *objdumpPath);
		TraceDqr::DQErr fillPipeBuffer();
		objDumpTokenType getNextLex(char *lex);
		bool isWSLookahead();
		bool isStringAHexNumber(char *s,uint64_t &n);
		bool isStringADecNumber(char *s,uint64_t &n);
		objDumpTokenType getRestOfLine(char *lex);
		TraceDqr::DQErr parseSection(objDumpTokenType &nextType,char *nextLex,Section *&codeSection);
		TraceDqr::DQErr parseSectionList(objDumpTokenType &nextType,char *nextLex,Section *&codeSectionLst);
		TraceDqr::DQErr parseFileLine(uint32_t &line);
		TraceDqr::DQErr parseFuncName();
		TraceDqr::DQErr parseFileOrLabelOrDisassembly(line_t &lineType,char *text,int &length,uint32_t &value);
		TraceDqr::DQErr parseDisassembly(bool &isLabel,int &instSize,uint32_t &inst,char *disassembly);
		TraceDqr::DQErr parseDisassemblyList(objDumpTokenType &nextType,char *nextLex,Section *codeSectionLst,SrcFileRoot &srcFileRoot);
		TraceDqr::DQErr parseFixedField(uint32_t &flags);
		TraceDqr::DQErr parseSymbol(bool &haveSym,char *secName,char *symName,uint32_t &symFlags,uint64_t &symSize);
		TraceDqr::DQErr parseSymbolTable(objDumpTokenType &nextType,char *nextLex,Sym *&syms,Section *&codeSectionLst);
		TraceDqr::DQErr parseElfName(char *elfName,enum elfType &et);
		TraceDqr::DQErr parseObjdump(int &archSize,Section *&codeSectionLst,Sym *&syms,SrcFileRoot &srcFileRoot);
};

class ElfReader {
public:
        	   ElfReader(const char *elfname,const char *odExe);
	          ~ElfReader();
	TraceDqr::DQErr getStatus() { return status; }
	TraceDqr::DQErr getInstructionByAddress(TraceDqr::ADDRESS addr, TraceDqr::RV_INST &inst);
	Symtab    *getSymtab();
	Section   *getSections() { return codeSectionLst; }
	int        getArchSize() { return archSize; }
	int        getBitsPerAddress() { return bitsPerAddress; }

	TraceDqr::DQErr parseNLSStrings(TraceDqr::nlStrings *nlsStrings);

	TraceDqr::DQErr dumpSyms();

private:
	TraceDqr::DQErr  status;
	char       *elfName;
	int         archSize;
	int         bitsPerAddress;
	Section	   *codeSectionLst;
	Symtab     *symtab;
	SrcFileRoot srcFileRoot;

	TraceDqr::DQErr fixupSourceFiles(Section *sections,Sym *syms);
};

class TsList {
public:
	TsList();
	~TsList();

	class TsList *prev;
	class TsList *next;
	bool terminated;
	TraceDqr::TIMESTAMP startTime;
	TraceDqr::TIMESTAMP endTime;
	char *message;
};

class ITCPrint {
public:
	ITCPrint(int itcPrintOpts,int numCores,int buffSize,int channel,TraceDqr::nlStrings *nlsStrings);
	~ITCPrint();
	bool print(uint8_t core, uint32_t address, uint32_t data);
	bool print(uint8_t core, uint32_t address, uint32_t data, TraceDqr::TIMESTAMP tstamp);
	void haveITCPrintData(int numMsgs[DQR_MAXCORES], bool havePrintData[DQR_MAXCORES]);
	bool getITCPrintMsg(uint8_t core, char *dst, int dstLen, TraceDqr::TIMESTAMP &startTime, TraceDqr::TIMESTAMP &endTime);
	bool flushITCPrintMsg(uint8_t core, char *dst, int dstLen, TraceDqr::TIMESTAMP &startTime, TraceDqr::TIMESTAMP &endTime);
	bool getITCPrintStr(uint8_t core, std::string &s, TraceDqr::TIMESTAMP &startTime, TraceDqr::TIMESTAMP &endTime);
	bool flushITCPrintStr(uint8_t core, std::string &s, TraceDqr::TIMESTAMP &starTime, TraceDqr::TIMESTAMP &endTime);
	int  getITCPrintMask();
	int  getITCFlushMask();
	bool haveITCPrintMsgs();

private:
	int  roomInITCPrintQ(uint8_t core);
	TsList *consumeTerminatedTsList(int core);
	TsList *consumeOldestTsList(int core);

	int itcOptFlags;
	int numCores;
	int buffSize;
	int printChannel;
	TraceDqr::nlStrings *nlsStrings;
	char **pbuff;
	int *pbi;
	int *pbo;
	int *numMsgs;
	class TsList **tsList;
	class TsList *freeList;
};

// class SliceFileParser: Class to parse binary or ascii nexus messages into a NexusMessage object
class SliceFileParser {
public:
             SliceFileParser(char *filename,int srcBits);
             ~SliceFileParser();
  TraceDqr::DQErr readNextTraceMsg(NexusMessage &nm,class Analytics &analytics,bool &haveMsg);
  TraceDqr::DQErr getFileOffset(int &size,int &offset);

  TraceDqr::DQErr getErr() { return status; };
  void       dump();

  TraceDqr::DQErr getNumBytesInSWTQ(int &numBytes);

private:
  TraceDqr::DQErr status;

  // add other counts for each message type

  int           srcbits;
  std::ifstream tf;
  int           tfSize;
  int           SWTsock;
  int           bitIndex;
  int           msgSlices;
  uint32_t      msgOffset;
  int           pendingMsgIndex;
  uint8_t       msg[64];
  bool          eom;

  int           bufferInIndex;
  int           bufferOutIndex;
  uint8_t       sockBuffer[2048];

  TraceDqr::DQErr readBinaryMsg(bool &haveMsg);
  TraceDqr::DQErr bufferSWT();
  TraceDqr::DQErr readNextByte(uint8_t *byte);
  TraceDqr::DQErr parseVarField(uint64_t *val,int *width);
  TraceDqr::DQErr parseFixedField(int width, uint64_t *val);
  TraceDqr::DQErr parseDirectBranch(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseIndirectBranch(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseDirectBranchWS(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseIndirectBranchWS(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseSync(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseCorrelation(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseAuxAccessWrite(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseDataAcquisition(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseOwnershipTrace(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseError(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseIndirectHistory(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseIndirectHistoryWS(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseResourceFull(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseICT(NexusMessage &nm,Analytics &analytics);
  TraceDqr::DQErr parseICTWS(NexusMessage &nm,Analytics &analytics);
};

class propertiesParser {
public:
	propertiesParser(const char *srcData);
	~propertiesParser();

	TraceDqr::DQErr getStatus() { return status; }

	void            rewind();
	TraceDqr::DQErr getNextProperty(char **name,char **value);

private:
	TraceDqr::DQErr status;

	struct line {
		char *name;
		char *value;
		char *line;
	};

	int   size;
	int   numLines;
	int   nextLine;
	line *lines;
	char *propertiesBuff;

	TraceDqr::DQErr getNextToken(char *inputText,int &startIndex,int &endIndex);
};

// class TraceSettings. Used to initialize trace objects

class TraceSettings {
public:
	TraceSettings();
	~TraceSettings();

	TraceDqr::DQErr addSettings(propertiesParser *properties);

	TraceDqr::DQErr propertyToTFName(const char *value);
	TraceDqr::DQErr propertyToEFName(const char *value);
	TraceDqr::DQErr propertyToPFName(const char *value);
	TraceDqr::DQErr propertyToSrcBits(const char *value);
	TraceDqr::DQErr propertyToNumAddrBits(const char *value);
	TraceDqr::DQErr propertyToITCPrintOpts(const char *value);
	TraceDqr::DQErr propertyToITCPrintBufferSize(const char *value);
	TraceDqr::DQErr propertyToITCPrintChannel(const char *value);
	TraceDqr::DQErr propertyToITCPerfEnable(const char *value);
	TraceDqr::DQErr propertyToITCPerfChannel(const char *value);
	TraceDqr::DQErr propertyToITCPerfMarkerValue(const char *value);
	TraceDqr::DQErr propertyToSrcRoot(const char *value);
	TraceDqr::DQErr propertyToSrcCutPath(const char *value);
	TraceDqr::DQErr propertyToCAName(const char *value);
	TraceDqr::DQErr propertyToCAType(const char *value);
	TraceDqr::DQErr propertyToPathType(const char *value);
	TraceDqr::DQErr propertyToFreq(const char *value);
	TraceDqr::DQErr propertyToTSSize(const char *value);
	TraceDqr::DQErr propertyToAddrDispFlags(const char *value);
	TraceDqr::DQErr propertyToCTFEnable(const char *value);
	TraceDqr::DQErr propertyToEventConversionEnable(const char *value);
	TraceDqr::DQErr propertyToStartTime(const char *value);
	TraceDqr::DQErr propertyToHostName(const char *value);
	TraceDqr::DQErr propertyToObjdumpName(const char *value);

	char *odName;
	char *tfName;
	char *efName;
	char *caName;
	char *pfName;
	TraceDqr::CATraceType caType;
	int srcBits;
	int numAddrBits;
	int itcPrintOpts;
	int itcPrintBufferSize;
	int itcPrintChannel;
	char *cutPath;
	char *srcRoot;
	TraceDqr::pathType pathType;
	uint32_t freq;
	uint32_t addrDispFlags;
	int64_t  startTime;
	int tsSize;
	bool CTFConversion;
	bool eventConversionEnable;
	char *hostName;
	bool filterControlEvents;

	bool itcPerfEnable;
	int itcPerfChannel;
	uint32_t itcPerfMarkerValue;

private:
	TraceDqr::DQErr propertyToBool(const char *src,bool &value);
};

// class CTFConverter: class to convert nexus messages to CTF file

class CTFConverter {
public:
	CTFConverter(char *elf,char *rtd,int numCores,int arch_size,uint32_t freq,int64_t t,char *hostName);
	~CTFConverter();

	TraceDqr::DQErr getStatus() { return status; }

	TraceDqr::DQErr writeCTFMetadata();
	TraceDqr::DQErr addCall(int core,TraceDqr::ADDRESS srcAddr,TraceDqr::ADDRESS dstAddr,TraceDqr::TIMESTAMP eventTS);
	TraceDqr::DQErr addRet(int core,TraceDqr::ADDRESS srcAddr,TraceDqr::ADDRESS dstAddr,TraceDqr::TIMESTAMP eventTS);

	TraceDqr::DQErr computeEventSizes(int core,int &size);

	TraceDqr::DQErr writeStreamHeaders(int core,uint64_t ts_begin,uint64_t ts_end,int size);
//	TraceDqr::DQErr writeStreamEventContext(int core);
	TraceDqr::DQErr writeTracePacketHeader(int core);
	TraceDqr::DQErr writeStreamPacketContext(int core,uint64_t ts_begin,uint64_t ts_end,int size);
//	TraceDqr::DQErr writeStreamEventHeader(int core,int index);
	TraceDqr::DQErr flushEvents(int core);
	TraceDqr::DQErr writeEvent(int core,int index);
	TraceDqr::DQErr writeBinInfo(int core,uint64_t timestamp);
	TraceDqr::DQErr computeBinInfoSize(int &size);

private:
	struct __attribute__ ((packed)) event {
//		CTF::event_type eventType;
		struct __attribute__ ((packed)) {
			uint16_t event_id;
			union __attribute__ ((packed)) {
				struct  __attribute__ ((packed)) {
					uint32_t timestamp;
				} compact;
				struct  __attribute__ ((packed)) {
					uint32_t event_id;
					uint64_t timestamp;
				} extended;
			};
		} event_header;
		union  __attribute__ ((packed)) {
			struct  __attribute__ ((packed)) {
				uint64_t pc;
				uint64_t pcDst;
			} call;
			struct  __attribute__ ((packed)) {
				uint64_t pc;
				uint64_t pcDst;
			} ret;
			struct  __attribute__ ((packed)) {
				uint64_t pc;
				int cause;
			} exception;
			struct  __attribute__ ((packed)) {
				uint64_t pc;
				int cause;
			} interrupt;
			struct __attribute__ ((packed)) {
				uint64_t _baddr;
				uint64_t _memsz;
				char _path[512];
				uint8_t _is_pic;
				uint8_t _has_build_id;
				uint8_t _has_debug_link;
			} binInfo;
		};
	};

	struct __attribute__ ((packed)) event_context {
		uint32_t _vpid;
		uint32_t _vtid;
		uint8_t _procname[17];
	};

	TraceDqr::DQErr status;
	int numCores;
	int fd[DQR_MAXCORES];
//	uint8_t uuid[16];
	int metadataFd;
	int packetSeqNum;
	uint32_t frequency;
	int archSize;
	event *eventBuffer[DQR_MAXCORES];
	int eventIndex[DQR_MAXCORES];
	event_context eventContext[DQR_MAXCORES];
	char *elfName;

	bool headerFlag[DQR_MAXCORES];
};

// class PerfConverter: class to convert nexus data acquistion messages into perf files

class PerfConverter {
public:
	PerfConverter(char *elf,char *rtd,Disassembler *disassembler,int numCores,uint32_t channel,uint32_t marker,uint32_t freq);
	~PerfConverter();

	TraceDqr::DQErr processITCPerf(int coreId,TraceDqr::TIMESTAMP ts,uint32_t addr,uint32_t data,bool &consumed);

	TraceDqr::DQErr getStatus() { return status; }

private:
	enum perfConverterState {
		perfStateSync,
		perfStateGetCntType,
		perfStateGetCntrMask,
		perfStateGetCntrRecord,
		perfStateGetCntrDef,
		perfStateGetCntrCode,
		perfStateGetCntrEventData,
		perfStateGetCntrInfo,
		perfStateGetAddr,
		perfStateGetCallSite,
		perfStateGetCnts,
		perfStateError,
	};

	enum perfCountType {
		perfCount_Raw = 0,
		perfCount_Delta = 1,
		perfCount_DeltaXOR = 2,
	};

	enum perfRecordType {
		perfRecord_FuncEnter = 0,
		perfRecord_FuncExit = 1,
		perfRecord_Manual = 2,
		perfRecord_ISR = 3,
	};

	enum perfClass_t {
		pt_0Index,
		pt_1Index,
		pt_2Index,
		pt_3Index,
		pt_4Index,
		pt_5Index,
		pt_6Index,
		pt_7Index,
		pt_8Index,
		pt_9Index,
		pt_10Index,
		pt_11Index,
		pt_12Index,
		pt_13Index,
		pt_14Index,
		pt_15Index,
		pt_16Index,
		pt_17Index,
		pt_18Index,
		pt_19Index,
		pt_20Index,
		pt_21Index,
		pt_22Index,
		pt_23Index,
		pt_24Index,
		pt_25Index,
		pt_26Index,
		pt_27Index,
		pt_28Index,
		pt_29Index,
		pt_30Index,
		pt_31Index,
		pt_addressIndex,
		pt_fnIndex,
		pt_numPerfTypes
	};

	TraceDqr::DQErr status;

	uint32_t frequency;

	int perfFDs[pt_numPerfTypes];
	int perfFD;

	char *elfNamePath;

	char perfNameGen[512];

	class Disassembler *disassembler;

	uint32_t perfChannel;
	perfConverterState state[DQR_MAXCORES];

	uint32_t markerValue;
	uint32_t cntrMaskIndex[DQR_MAXCORES];
	uint32_t cntrMask[DQR_MAXCORES];
	bool     valuePending[DQR_MAXCORES];
	uint8_t cntType[DQR_MAXCORES];
	uint32_t cntrType[DQR_MAXCORES];
	uint32_t cntrCode[DQR_MAXCORES];
	uint64_t cntrEventData[DQR_MAXCORES];
	uint8_t recordType[DQR_MAXCORES];
	uint32_t savedLow32[DQR_MAXCORES];
	TraceDqr::ADDRESS lastAddress[DQR_MAXCORES];
	TraceDqr::ADDRESS cntrAddress[DQR_MAXCORES];
	uint64_t *lastCount[DQR_MAXCORES];

	TraceDqr::DQErr emitPerfAddr(int core,TraceDqr::TIMESTAMP ts,TraceDqr::ADDRESS pc);
	TraceDqr::DQErr emitPerfFnEntry(int core,TraceDqr::TIMESTAMP ts,TraceDqr::ADDRESS fnAddr,TraceDqr::ADDRESS callSite);
	TraceDqr::DQErr emitPerfFnExit(int core,TraceDqr::TIMESTAMP ts,TraceDqr::ADDRESS fnAddr,TraceDqr::ADDRESS callSite);
	TraceDqr::DQErr emitPerfCntr(int core,TraceDqr::TIMESTAMP ts,TraceDqr::ADDRESS pc,int cntrIndex,uint64_t cntrVal);
	TraceDqr::DQErr emitPerfCntType(int core,TraceDqr::TIMESTAMP ts,int cntType);
	TraceDqr::DQErr emitPerfCntrMask(int core,TraceDqr::TIMESTAMP ts,uint32_t cntrMask);
	TraceDqr::DQErr emitPerfCntrDef(int core,TraceDqr::TIMESTAMP ts,int cntrIndex,uint32_t cntrType,uint32_t cntrCode,uint64_t eventData,uint32_t cntrInfo);
};

// class EventConverter: class to convert nexus messages to Event files

class EventConverter {
public:
	EventConverter(char *elf,char *rtd,class Disassembler *disassembler,int numCores,uint32_t freq);
	~EventConverter();

	TraceDqr::DQErr getStatus() { return status; }

	TraceDqr::DQErr emitExtTrigEvent(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,int id);
	TraceDqr::DQErr emitWatchpoint(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,int id);
	TraceDqr::DQErr emitCallRet(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,TraceDqr::ADDRESS pcDest,int crFlags);
	TraceDqr::DQErr emitException(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,int cause);
	TraceDqr::DQErr emitInterrupt(int coe,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,int cause);
	TraceDqr::DQErr emitContext(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc,int context);
	TraceDqr::DQErr emitPeriodic(int core,TraceDqr::TIMESTAMP ts,int ckdf,TraceDqr::ADDRESS pc);
	TraceDqr::DQErr emitControl(int core,TraceDqr::TIMESTAMP ts,int ckdf,int control,TraceDqr::ADDRESS pc);

private:

	TraceDqr::DQErr status;
	int numCores;
	uint32_t frequency;
	char eventNameGen[512];
	int eventFDs[CTF::et_numEventTypes];
	int eventFD;

	char *elfNamePath;

	class Disassembler *disassembler;

	const char *getInterruptCauseText(int cause);
	const char *getExceptionCauseText(int cause);
	const char *getControlText(int control);
};

// class Disassembler: class to help in the dissasemblhy of instrucitons

class Disassembler {
public:
	 Disassembler(Symtab *stp,Section *sp,int archsize);
	~Disassembler();

    TraceDqr::DQErr disassemble(TraceDqr::ADDRESS addr);

    TraceDqr::DQErr getSrcLines(TraceDqr::ADDRESS addr,const char **filename,int *cutPathIndex,const char **functionname,unsigned int *linenumber,const char **lineptr);

	TraceDqr::DQErr getFunctionName(TraceDqr::ADDRESS addr,const char *&function,int &offset);

	static TraceDqr::DQErr   decodeInstructionSize(uint32_t inst, int &inst_size);
	static int   decodeInstruction(uint32_t instruction,int archSize,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);

	Instruction getInstructionInfo() { return instruction; }
	Source      getSourceInfo() { return source; }

	TraceDqr::DQErr setPathType(TraceDqr::pathType pt);
	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);

	TraceDqr::DQErr getStatus() {return status;}

private:
	TraceDqr::DQErr   status;

	int               archSize;

	Section	         *sectionLst;		// owned by elfReader - don't delete
	Symtab           *symtab;			// owned by elfReader - don't delete

	// cached section information

	TraceDqr::ADDRESS cachedAddr;
	Section          *cachedSecPtr;
	int               cachedIndex;

	Instruction instruction;
	Source      source;

	class fileReader *fileReader;

	TraceDqr::pathType pType;

	TraceDqr::DQErr getDissasembly(TraceDqr::ADDRESS addr,char *&dissText);
	TraceDqr::DQErr cacheSrcInfo(TraceDqr::ADDRESS addr);

	TraceDqr::DQErr lookupInstructionByAddress(TraceDqr::ADDRESS addr,uint32_t &ins,int &insSize);
	TraceDqr::DQErr findNearestLine(TraceDqr::ADDRESS addr,const char *&file,int &line);

	TraceDqr::DQErr getInstruction(TraceDqr::ADDRESS addr,Instruction &instruction);

	// need to make all the decode function static. Might need to move them to public?

	static int decodeRV32Q0Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV32Q1Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV32Q2Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV32Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);

	static int decodeRV64Q0Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV64Q1Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV64Q2Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	static int decodeRV64Instruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
};

class AddrStack {
public:
	AddrStack(int size = 2048);
	~AddrStack();
	void reset();
	int push(TraceDqr::ADDRESS addr);
	TraceDqr::ADDRESS pop();
	int getNumOnStack() { return stackSize - sp; }

private:
	int stackSize;
	int sp;
	TraceDqr::ADDRESS *stack;
};

class Count {
public:
	Count();
	~Count();

	void resetCounts(int core);

	TraceDqr::CountType getCurrentCountType(int core);
	TraceDqr::DQErr setICnt(int core,int count);
	TraceDqr::DQErr setHistory(int core,uint64_t hist);
	TraceDqr::DQErr setHistory(int core,uint64_t hist,int count);
	TraceDqr::DQErr setTakenCount(int core,int takenCnt);
	TraceDqr::DQErr setNotTakenCount(int core,int notTakenCnt);
	TraceDqr::DQErr setCounts(NexusMessage *nm);
	int consumeICnt(int core,int numToConsume);
	int consumeHistory(int core,bool &taken);
	int consumeTakenCount(int core);
	int consumeNotTakenCount(int core);

	int getICnt(int core) { return i_cnt[core]; }
	uint32_t getHistory(int core) { return history[core]; }
	int getNumHistoryBits(int core) { return histBit[core]; }
	uint32_t getTakenCount(int core) { return takenCount[core]; }
	uint32_t getNotTakenCount(int core) { return notTakenCount[core]; }
	uint32_t isTaken(int core) { return (history[core] & (1 << histBit[core])) != 0; }

	int push(int core,TraceDqr::ADDRESS addr) { return stack[core].push(addr); }
	TraceDqr::ADDRESS pop(int core) { return stack[core].pop(); }
	void resetStack(int core) { stack[core].reset(); }
	int getNumOnStack(int core) { return stack[core].getNumOnStack(); }


	void dumpCounts(int core);

//	int getICnt(int core);
//	int adjustICnt(int core,int delta);
//	bool isHistory(int core);
//	bool takenHistory(int core);

private:
	int i_cnt[DQR_MAXCORES];
    uint64_t history[DQR_MAXCORES];
    int histBit[DQR_MAXCORES];
    int takenCount[DQR_MAXCORES];
    int notTakenCount[DQR_MAXCORES];
    AddrStack stack[DQR_MAXCORES];
};

#endif /* TRACE_HPP_ */


// Improvements:
//
// Disassembler class:
//  Should be able to creat disassembler object without elf file
//  Should have a diasassemble method that takes an address and an instruciotn, not just an address
//  Should be able us use a block of memory for the code, not from an elf file
//  Use new methods to cleanup verilator nextInstruction()

// move some stuff in instruction object to a separate object pointed to from instruciton object so that coppies
// of the object don't need to copy it all (regfile is an example). Create accessor method to get. Destructor should
// delete all

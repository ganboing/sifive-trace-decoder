/* Copyright 2022 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef DQR_HPP_
#define DQR_HPP_

// PUBLIC definitions

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <cassert>

//#define DO_TIMES	1

#define DQR_MAXCORES	16

#define DEFAULTOBJDUMPNAME	"riscv64-unknown-elf-objdump"

extern int globalDebugFlag;
extern const char * const DQR_VERSION;

class CTF {
public:
	struct trace_packet_header {
		uint32_t magic;
		uint8_t uuid[16];				// optional?
		uint32_t stream_id;
		uint64_t stream_instance_id;	// optional?
	};

	enum event_type {
		event_tracePoint = 0,
		event_funcEntry = 1,
		event_funcExit = 2,
		event_stateDumpStart = 3,
		event_stateDumpBinInfo = 4,
		event_stateDumpEnd = 7,
		event_extended = 0xffff
	};

	// metadata file needs to have an env struct!!

	typedef uint64_t uint64_clock_monotonic_t;

	struct stream_packet_context {
		uint64_clock_monotonic_t timestamp_begin;
		uint64_clock_monotonic_t timestamp_end;
		uint64_t content_size;
		uint64_t packet_size;
		uint64_t packet_seq_num;
		uint64_t events_discarded;	// or unsigned long?
		uint32_t cpu_id;
	};

	struct stream_packet_header_extended {
		uint16_t id;	// this should be 0xffff for extended headers
		uint32_t extended_id;
		uint64_clock_monotonic_t extended_timestamp;
	};

	struct stream_event_context {
		uint32_t _vpid;
		uint32_t _vtid;
		uint8_t  _procname[17];
	};

	struct stream_event_callret {
		uint64_t src;
		uint64_t dst;
	};

	enum event_t {
		et_controlIndex,
		et_extTriggerIndex,
		et_callRetIndex,
		et_exceptionIndex,
		et_interruptIndex,
		et_hContextIndex,
		et_sContextIndex,
		et_privContextIndex,
		et_watchpointIndex,
		et_periodicIndex,
		et_numEventTypes
	};
};

class TraceDqr {
public:
  typedef uint32_t RV_INST;

  typedef uint64_t ADDRESS;
  typedef uint64_t TIMESTAMP;
  typedef int RCode;

  enum {
	TRACE_HAVE_INSTINFO = 0x01,
	TRACE_HAVE_SRCINFO  = 0x02,
	TRACE_HAVE_MSGINFO  = 0x04,
	TRACE_HAVE_ITCPRINTINFO = 0x08,
  };

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
  	TCODE_RESOURCEFULL            = 27,
  	TCODE_INDIRECTBRANCHHISTORY   = 28,
  	TCODE_INDIRECTBRANCHHISTORY_WS = 29,
  	TCODE_REPEATBRANCH            = 30,
  	TCODE_REPEATINSTRUCTION       = 31,
  	TCODE_REPEATINSTRUCTION_WS    = 32,
  	TCODE_CORRELATION             = 33,
  	TCODE_INCIRCUITTRACE          = 34,
	TCODE_INCIRCUITTRACE_WS       = 35,

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
	SYNC_PC_SAMPLE          = 15,
  	SYNC_NONE
  } SyncReason;

  typedef enum {
	ICT_CONTROL       = 0,
	ICT_EXT_TRIG      = 8,
	ICT_INFERABLECALL = 9,
	ICT_EXCEPTION     = 10,
	ICT_INTERRUPT     = 11,
	ICT_CONTEXT       = 13,
	ICT_WATCHPOINT    = 14,
	ICT_PC_SAMPLE     = 15,
	ICT_NONE
  } ICTReason;

  typedef enum {
    ICT_CONTROL_NONE = 0,
	ICT_CONTROL_TRACE_ON = 2,
	ICT_CONTROL_TRACE_OFF = 3,
	ICT_CONTROL_EXIT_DEBUG = 4,
	ICT_CONTROL_ENTER_DEBUG = 5,
	ICT_CONTROL_EXIT_RESET = 6,
	ICT_CONTROL_ENTER_RESET = 8,
  } ICTControl;

  typedef enum {
	  ITC_OPT_NONE = 0,
	  ITC_OPT_PRINT = 1,
	  ITC_OPT_NLS = 2,
  } ITCOptions;

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

  enum InstType {
		INST_UNKNOWN = 0,
		INST_JAL,
		INST_JALR,
		INST_BEQ,
		INST_BNE,
		INST_BLT,
		INST_BGE,
		INST_BLTU,
		INST_BGEU,
		INST_C_J,
		INST_C_JAL,
		INST_C_JR,
		INST_C_JALR,
		INST_C_BEQZ,
		INST_C_BNEZ,
		INST_EBREAK,
		INST_C_EBREAK,
		INST_ECALL,
		INST_MRET,
		INST_SRET,
		INST_URET,
		// the following intTypes are generic and do not specify an actual instruction
		INST_SCALER,
		INST_VECT_ARITH,
		INST_VECT_LOAD,
		INST_VECT_STORE,
		INST_VECT_AMO,
		INST_VECT_AMO_WW,
		INST_VECT_CONFIG,
	};

	enum CountType{
		COUNTTYPE_none,
		COUNTTYPE_i_cnt,
		COUNTTYPE_history,
		COUNTTYPE_taken,
		COUNTTYPE_notTaken
	};

	enum Reg {
		REG_0 = 0,
		REG_1 = 1,
		REG_2 = 2,
		REG_3 = 3,
		REG_4 = 4,
		REG_5 = 5,
		REG_6 = 6,
		REG_7 = 7,
		REG_8 = 8,
		REG_9 = 9,
		REG_10 = 10,
		REG_11 = 11,
		REG_12 = 12,
		REG_13 = 13,
		REG_14 = 14,
		REG_15 = 15,
		REG_16 = 16,
		REG_17 = 17,
		REG_18 = 18,
		REG_19 = 19,
		REG_20 = 20,
		REG_21 = 21,
		REG_22 = 22,
		REG_23 = 23,
		REG_24 = 24,
		REG_25 = 25,
		REG_26 = 26,
		REG_27 = 27,
		REG_28 = 28,
		REG_29 = 29,
		REG_30 = 30,
		REG_31 = 31,
		REG_unknown,
	};

	enum TraceType {
		TRACETYPE_unknown = 0,
		TRACETYPE_BTM,
		TRACETYPE_HTM,
		TRACETYPE_HTMNOOPT,
		TRACETYPE_EVENT,
		TRACETYPE_PATH,
		TRACETYPE_VCD,
	};

	enum CallReturnFlag {
		isNone            = 0,
		isCall            = (1<<0),
		isReturn          = (1<<1),
		isSwap            = (1<<2),
		isInterrupt       = (1<<3),
		isException       = (1<<4),
		isExceptionReturn = (1<<5),
	 };

	enum BranchFlags {
		BRFLAG_none = 0,
		BRFLAG_unknown,
		BRFLAG_taken,
		BRFLAG_notTaken,
	};

	enum tsType {
		TS_full,
		TS_rel,
	};

	enum pathType {
		PATH_RAW,
		PATH_TO_WINDOWS,
		PATH_TO_UNIX,
	};

	enum CATraceType {
		CATRACE_NONE,
		CATRACE_INSTRUCTION,
		CATRACE_VECTOR,
	};

	enum CAVectorTraceFlags {
		CAVFLAG_V0      = 0x20,
		CAVFLAG_V1      = 0x10,
		CAVFLAG_VISTART = 0x08,
		CAVFLAG_VIARITH = 0x04,
		CAVFLAG_VISTORE = 0x02,
		CAVFLAG_VILOAD  = 0x01,
	};

	enum CATraceFlags {
		CAFLAG_NONE   = 0x00,
		CAFLAG_PIPE0  = 0x01,
		CAFLAG_PIPE1  = 0x02,
		CAFLAG_SCALER = 0x04,
		CAFLAG_VSTART = 0x08,
		CAFLAG_VSTORE = 0x10,
		CAFLAG_VLOAD  = 0x20,
		CAFLAG_VARITH = 0x40,
	};

	struct nlStrings{
		int nf;
		int signedMask;
		char *format;
	};

	enum elfType {
	  elfType_unknown,
	  elfType_64_little,
	  elfType_32_little,
	  elfType_32_binary,
	  elfType_64_binary,
	};

	enum prvType {
	  prv_U_mode  = (0 << 4) | 0,
	  prv_S_mode  = (0 << 4) | 1,
	  prv_M_mode  = (0 << 4) | 3,
	  prv_VU_mode = (1 << 4) | 0,
	  prv_VS_mode = (1 << 4) | 1,
	  prv_unknown = 0xff
	};
};

// class Instruction: work with an instruction

#ifdef SWIG
	%ignore Instruction::addressToText(char *dst,size_t len,int labelLevel);
	%ignore Instruction::instructionToText(char *dst,size_t len,int labelLevel);
	%ignore Instruction::addressLabel;
#endif // SWIG

class Instruction {
public:
	void addressToText(char *dst,size_t len,int labelLevel);
	std::string addressToString(int labelLevel);
	std::string addressLabelToString();
//	void opcodeToText();
	void instructionToText(char *dst,size_t len,int labelLevel);
	std::string instructionToString(int labelLevel);

	static int        addrSize;
	static uint32_t   addrDispFlags;
	static int        addrPrintWidth;

	uint8_t           coreId;
        uint8_t           prv;
	uint32_t          pid;

	int               CRFlag;
	int               brFlags; // this is an int instead of TraceDqr::BancheFlags because it is easier to work with in java

	TraceDqr::ADDRESS address;
	int               instSize;
	TraceDqr::RV_INST instruction;
	char             *instructionText;

#ifdef SWIG
	%immutable		addressLabel;
#endif // SWIG
	const char       *addressLabel;
	int               addressLabelOffset;

	TraceDqr::TIMESTAMP timestamp;

	uint32_t            caFlags;
	uint32_t            pipeCycles;
	uint32_t            VIStartCycles;
	uint32_t            VIFinishCycles;

	uint8_t             qDepth;
	uint8_t             arithInProcess;
	uint8_t             loadInProcess;
	uint8_t             storeInProcess;

	uint32_t r0Val;
	uint32_t r1Val;
	uint32_t wVal;
};

// class Source: Helper class for source code information for an address

#ifdef SWIG
	%ignore Source::sourceFile;
	%ignore Source::sourceFunction;
	%ignore Source::sourceLine;
#endif // SWIG

class Source {
public:
	std::string  sourceFileToString();
	std::string  sourceFileToString(std::string path);
	std::string  sourceLineToString();
	std::string  sourceFunctionToString();
	uint8_t      coreId;
        uint8_t      prv;
	uint32_t     pid;
#ifdef SWIG
	%immutable sourceFile;
	%immutable sourceFunction;
	%immutable sourceLine;
#endif // SWIG
	const char  *sourceFile;
	int          cutPathIndex;
	const char  *sourceFunction;
	const char  *sourceLine;
	unsigned int sourceLineNum;

private:
	const char *stripPath(const char *path);
};

// class NexusMessage: class to hold Nexus messages and convert them to text

#ifdef SWIG
	%ignore NexusMessage::messageToText(char *dst,size_t dst_len,int level);
#endif // SWIG

class NexusMessage {
public:
	NexusMessage();
	bool processITCPrintData(class ITCPrint *itcPrint);
	void messageToText(char *dst,size_t dst_len,int level);
	std::string messageToString(int detailLevel);
	double seconds();

	void dumpRawMessage();
	void dump();

	static uint32_t targetFrequency;

	int                 msgNum;
	TraceDqr::TCode     tcode;
    bool       	        haveTimestamp;
    TraceDqr::TIMESTAMP timestamp;
    TraceDqr::ADDRESS   currentAddress;
    TraceDqr::TIMESTAMP time;

    uint8_t             coreId;
    uint8_t		prv;
    uint32_t		pid;

    union {
    	struct {
    		int i_cnt;
    	} directBranch;
    	struct {
    		int          i_cnt;
    		TraceDqr::ADDRESS u_addr;
    		TraceDqr::BType   b_type;
    	} indirectBranch;
    	struct {
    		int             i_cnt;
    		TraceDqr::ADDRESS    f_addr;
    		TraceDqr::SyncReason sync;
    	} directBranchWS;
    	struct {
    		int             i_cnt;
    		TraceDqr::ADDRESS    f_addr;
    		TraceDqr::BType      b_type;
    		TraceDqr::SyncReason sync;
    	} indirectBranchWS;
    	struct {
    		int             i_cnt;
    		TraceDqr::ADDRESS    u_addr;
    		TraceDqr::BType      b_type;
    		uint64_t		history;
    	} indirectHistory;
    	struct {
    		int             i_cnt;
    		TraceDqr::ADDRESS    f_addr;
    		TraceDqr::BType      b_type;
    		uint64_t		history;
    		TraceDqr::SyncReason sync;
    	} indirectHistoryWS;
    	struct {
    		TraceDqr::RCode rCode;
    		union {
    			int i_cnt;
    			uint64_t history;
    			uint32_t takenCount;
    			uint32_t notTakenCount;
    		};
    	} resourceFull;
    	struct {
    		int             i_cnt;
    		TraceDqr::ADDRESS    f_addr;
    		TraceDqr::SyncReason sync;
    	} sync;
    	struct {
    		uint8_t etype;
    	} error;
    	struct {
    		uint64_t history;
    		int     i_cnt;
    		uint8_t cdf;
    		uint8_t evcode;
    	} correlation;
    	struct {
    		uint32_t data;
    		uint32_t addr;
    	} auxAccessWrite;
    	struct {
    		uint32_t idTag;
    		uint32_t data;
    	} dataAcquisition;
    	struct {
		int pid;
		uint8_t v;
		uint8_t prv;
		uint8_t tag;
    	} ownership;
    	struct {
    		TraceDqr::ICTReason cksrc;
    		uint8_t ckdf;
    		TraceDqr::ADDRESS ckdata[2];
    	} ict;
    	struct {
    		TraceDqr::ICTReason cksrc;
    		uint8_t ckdf;
    		TraceDqr::ADDRESS ckdata[2];
    	} ictWS;
    };

    uint32_t offset;
    uint8_t  rawData[32];

    int getI_Cnt();
    TraceDqr::ADDRESS    getU_Addr();
    TraceDqr::ADDRESS    getF_Addr();
    TraceDqr::ADDRESS    getNextAddr() {return currentAddress;};
    TraceDqr::ADDRESS    getICTCallReturnTarget();
    TraceDqr::BType      getB_Type();
	TraceDqr::SyncReason getSyncReason();
	uint8_t  getEType();
	uint8_t  getCKDF();
	TraceDqr::ICTReason  getCKSRC();
	TraceDqr::ADDRESS getCKData(int i);
	uint8_t  getCDF();
	uint8_t  getEVCode();
	uint32_t getData();
	uint32_t getAddr();
	uint32_t getIdTag();
	uint32_t getRCode();
	uint64_t getRData();
	uint64_t getHistory();
	uint32_t getProcessId();
};

#ifdef SWIG
	%ignore Analytics::toText(char *dst,size_t dst_len,int level);
#endif // SWIG

class Analytics {
public:
	Analytics();
	~Analytics();

	TraceDqr::DQErr updateTraceInfo(NexusMessage &nm,uint32_t bits,uint32_t meso_bits,uint32_t ts_bits,uint32_t addr_bits);
	TraceDqr::DQErr updateInstructionInfo(uint32_t core_id,uint32_t inst,int instSize,int crFlags,TraceDqr::BranchFlags brFlags);
	int currentTraceMsgNum() { return num_trace_msgs_all_cores; }
	void setSrcBits(int sbits) { srcBits = sbits; }
	void toText(char *dst,int dst_len,int detailLevel);
	std::string toString(int detailLevel);

private:
	TraceDqr::DQErr status;
#ifdef DO_TIMES
	class Timer *etimer;
#endif // DO_TIMES

	uint32_t cores;

	int srcBits;

	uint32_t num_trace_msgs_all_cores;
	uint32_t num_trace_mseo_bits_all_cores;
	uint32_t num_trace_bits_all_cores;
	uint32_t num_trace_bits_all_cores_max;
	uint32_t num_trace_bits_all_cores_min;

	uint32_t num_inst_all_cores;
	uint32_t num_inst16_all_cores;
	uint32_t num_inst32_all_cores;

	uint32_t num_branches_all_cores;

	struct {
		uint32_t num_inst;
		uint32_t num_inst16;
		uint32_t num_inst32;

		uint32_t num_trace_msgs;
		uint32_t num_trace_syncs;
		uint32_t num_trace_dbranch;
		uint32_t num_trace_ibranch;
		uint32_t num_trace_dataacq;
		uint32_t num_trace_dbranchws;
		uint32_t num_trace_ibranchws;
		uint32_t num_trace_ihistory;
		uint32_t num_trace_ihistoryws;
		uint32_t num_trace_takenhistory;
		uint32_t num_trace_resourcefull;
		uint32_t num_trace_correlation;
		uint32_t num_trace_auxaccesswrite;
		uint32_t num_trace_ownership;
		uint32_t num_trace_error;
		uint32_t num_trace_incircuittraceWS;
		uint32_t num_trace_incircuittrace;

		uint32_t trace_bits;
		uint32_t trace_bits_max;
		uint32_t trace_bits_min;
		uint32_t trace_bits_mseo;

		uint32_t max_hist_bits;
		uint32_t min_hist_bits;
		uint32_t max_notTakenCount;
		uint32_t min_notTakenCount;
		uint32_t max_takenCount;
		uint32_t min_takenCount;

		uint32_t trace_bits_sync;
		uint32_t trace_bits_dbranch;
		uint32_t trace_bits_ibranch;
		uint32_t trace_bits_dataacq;
		uint32_t trace_bits_dbranchws;
		uint32_t trace_bits_ibranchws;
		uint32_t trace_bits_ihistory;
		uint32_t trace_bits_ihistoryws;
		uint32_t trace_bits_resourcefull;
		uint32_t trace_bits_correlation;
		uint32_t trace_bits_auxaccesswrite;
		uint32_t trace_bits_ownership;
		uint32_t trace_bits_error;
		uint32_t trace_bits_incircuittraceWS;
		uint32_t trace_bits_incircuittrace;

		uint32_t num_trace_ts;
		uint32_t num_trace_uaddr;
		uint32_t num_trace_faddr;
		uint32_t num_trace_ihistory_taken_branches;
		uint32_t num_trace_ihistory_nottaken_branches;
		uint32_t num_trace_resourcefull_i_cnt;
		uint32_t num_trace_resourcefull_hist;
		uint32_t num_trace_resourcefull_takenCount;
		uint32_t num_trace_resourcefull_notTakenCount;
		uint32_t num_trace_resourcefull_taken_branches;
		uint32_t num_trace_resourcefull_nottaken_branches;

		uint32_t num_taken_branches;
		uint32_t num_notTaken_branches;
		uint32_t num_calls;
		uint32_t num_returns;
		uint32_t num_swaps;
		uint32_t num_exceptions;
		uint32_t num_exception_returns;
		uint32_t num_interrupts;

		uint32_t trace_bits_ts;
		uint32_t trace_bits_ts_max;
		uint32_t trace_bits_ts_min;

		uint32_t trace_bits_uaddr;
		uint32_t trace_bits_uaddr_max;
		uint32_t trace_bits_uaddr_min;

		uint32_t trace_bits_faddr;
		uint32_t trace_bits_faddr_max;
		uint32_t trace_bits_faddr_min;

		uint32_t trace_bits_hist;
	} core[DQR_MAXCORES];
};

class CATraceRec {
public:
	CATraceRec();
	void dump();
	void dumpWithCycle();
	int consumeCAInstruction(uint32_t &pipe,uint32_t &cycles);
	int consumeCAVector(uint32_t &record,uint32_t &cycles);
	int offset;
	TraceDqr::ADDRESS address;
	uint32_t data[32];
};

class CATrace {
public:
	CATrace(char *caf_name,TraceDqr::CATraceType catype);
	~CATrace();
	TraceDqr::DQErr consume(uint32_t &caFlags,TraceDqr::InstType iType,uint32_t &pipeCycles,uint32_t &viStartCycles,uint32_t &viFinishCycles,uint8_t &qDepth,uint8_t &arithDepth,uint8_t &loadDepth,uint8_t &storeDepth);

	TraceDqr::DQErr rewind();
	TraceDqr::ADDRESS getCATraceStartAddr();

	TraceDqr::DQErr getStatus() {return status;}

private:
	struct CATraceQItem {
		uint32_t cycle;
		uint8_t record;
		uint8_t qDepth;
		uint8_t arithInProcess;
		uint8_t loadInProcess;
		uint8_t storeInProcess;
	};

	TraceDqr::DQErr status;

	TraceDqr::CATraceType caType;
	int      caBufferSize;
	uint8_t *caBuffer;
	int      caBufferIndex;
	int      blockRecNum;

	TraceDqr::ADDRESS startAddr;
	//uint32_t baseCycles;
	CATraceRec catr;
	int       traceQSize;
	int       traceQOut;
	int       traceQIn;
	CATraceQItem *caTraceQ;

	int roomQ();
	TraceDqr::DQErr packQ();

	TraceDqr::DQErr addQ(uint32_t data,uint32_t t);

	void dumpCAQ();

	TraceDqr::DQErr parseNextVectorRecord(int &newDataStart);
	TraceDqr::DQErr parseNextCATraceRec(CATraceRec &car);
	TraceDqr::DQErr dumpCurrentCARecord(int level);
	TraceDqr::DQErr consumeCAInstruction(uint32_t &pipe,uint32_t &cycles);
	TraceDqr::DQErr consumeCAPipe(int &QStart,uint32_t &cycles,uint32_t &pipe);
	TraceDqr::DQErr consumeCAVector(int &QStart,TraceDqr::CAVectorTraceFlags type,uint32_t &cycles,uint8_t &qInfo,uint8_t &arithInfo,uint8_t &loadInfo,uint8_t &storeInfo);
};

class ObjFile {
public:
	ObjFile(char *ef_name,const char *odExe);
	~ObjFile();
	void cleanUp();

	TraceDqr::DQErr getStatus() {return status;}
	TraceDqr::DQErr sourceInfo(TraceDqr::ADDRESS addr,Instruction &instInfo,Source &srcInfo);
	TraceDqr::DQErr setPathType(TraceDqr::pathType pt);

	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);
	TraceDqr::DQErr parseNLSStrings(TraceDqr::nlStrings (&nlsStrings)[32]);

	TraceDqr::DQErr dumpSyms();

private:
	TraceDqr::DQErr        status;
    char                  *cutPath;
    char                  *newRoot;
	class ElfReader       *elfReader;
	class Disassembler    *disassembler;
};

class pidMap {
public:
	pidMap();
	~pidMap();

	uint32_t pid;
	char *name;
	int shortNameIndex;
};

// class Trace: high level class that performs the raw trace data to dissasemble and decorated instruction trace

#ifdef SWIG
	%ignore Trace::NextInstruction(Instruction **instInfo,NexusMessage **msgInfo,Source **srcInfo);
	%ignore Trace::getITCPrintMsg(int core,char *dst,int dstLen,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);
	%ignore Trace::flushITCPrintMsg(int core,char *dst,int dstLen,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);
	%ignore Trace::analyticsToText(char *dst,int dst_len,int detailLevel);
	%ignore Trace::getITCPrintStr(int core,bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endtime);
	%ignore Trace::flushITCPrintStr(int core,bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endtime);
#endif // SWIG

class Trace {
public:
    Trace(char *tf_name,char *ef_name,int numAddrBits,uint32_t addrDispFlags,int srcBits,const char *odExe,uint32_t freq = 0);
    Trace(char *pf_name);
    ~Trace();
    void cleanUp();
    static const char *version();
    TraceDqr::DQErr setTraceType(TraceDqr::TraceType tType);
    TraceDqr::DQErr setErrorMode(bool tolerate);
	TraceDqr::DQErr setTSSize(int size);
	TraceDqr::DQErr setITCPrintOptions(int intFlags,int buffSize,int channel);
	TraceDqr::DQErr setPathType(TraceDqr::pathType pt);
	TraceDqr::DQErr setCATraceFile(char *caf_name,TraceDqr::CATraceType catype);
	TraceDqr::DQErr enableCTFConverter(int64_t startTime,char *hostName);
	TraceDqr::DQErr enableEventConverter(int pIndex);
	TraceDqr::DQErr enablePerfConverter(int perfChannel,uint32_t markerValue);

	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);

	enum TraceFlags {
		TF_INSTRUCTION = 0x01,
		TF_ADDRESS     = 0x02,
		TF_DISSASEMBLE = 0x04,
		TF_TIMESTAMP   = 0x08,
		TF_TRACEINFO   = 0x10,
	};
	TraceDqr::DQErr getStatus() { return status; }
	TraceDqr::DQErr NextInstruction(Instruction **instInfo, NexusMessage **msgInfo, Source **srcInfo);
	TraceDqr::DQErr NextInstruction(Instruction *instInfo, NexusMessage *msgInfo, Source *srcInfo, int *flags);

	TraceDqr::DQErr getTraceFileOffset(int &size,int &offset);

	TraceDqr::DQErr haveITCPrintData(int numMsgs[DQR_MAXCORES], bool havePrintData[DQR_MAXCORES]);
	bool        getITCPrintMsg(int core,char *dst, int dstLen,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);
	bool        flushITCPrintMsg(int core,char *dst, int dstLen,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);
	std::string getITCPrintStr(int core, bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);
	std::string flushITCPrintStr(int core, bool &haveData,TraceDqr::TIMESTAMP &startTime,TraceDqr::TIMESTAMP &endTime);

	std::string getITCPrintStr(int core, bool &haveData,double &startTime,double &endTime);
	std::string flushITCPrintStr(int core, bool &haveData,double &startTime,double &endTime);

	pidMap *getPidMap(int &num_pids);

//	const char *getSymbolByAddress(TraceDqr::ADDRESS addr);
	TraceDqr::DQErr Disassemble(TraceDqr::ADDRESS addr);
	int	    getSrcBits();
	int         getArchSize(int pid);
	int         getAddressSize(int pid);
	bool        isLinuxTrace();
	void        analyticsToText(char *dst,int dst_len,int detailLevel) {analytics.toText(dst,dst_len,detailLevel); }
	std::string analyticsToString(int detailLevel) { return analytics.toString(detailLevel); }
	TraceDqr::TIMESTAMP processTS(TraceDqr::tsType tstype, TraceDqr::TIMESTAMP lastTs, TraceDqr::TIMESTAMP newTs);
	int         getITCPrintMask();
	int         getITCFlushMask();
	TraceDqr::DQErr getInstructionByAddress(TraceDqr::ADDRESS addr, Instruction *instInfo,Source *srcInfo,int *flags);
	TraceDqr::DQErr getInstructionByAddress(TraceDqr::ADDRESS addr,TraceDqr::RV_INST &inst);

	TraceDqr::DQErr getNumBytesInSWTQ(int &numBytes);

private:
	enum state {
		TRACE_STATE_SYNCCATE,
		TRACE_STATE_GETFIRSTSYNCMSG,
		TRACE_STATE_RETIREMESSAGE,
		TRACE_STATE_GETNEXTMSG,
		TRACE_STATE_GETNEXTINSTRUCTION,
		TRACE_STATE_DONE,
		TRACE_STATE_ERROR
	};

	TraceDqr::DQErr        status;
	bool                   tolerateErrors;
	TraceDqr::TraceType    traceType;
	class SliceFileParser *sfp;
	bool                   linuxTrace;
        int                    numProcesses;
	struct process        *processes;
	int                    numPids;
	int                   *pidList;
	class KMem            *kMem; // all proceses use the same kMem object
	char                  *objdump;
	char                  *rtdName;
	char                  *vdsoName;
	char                  *mfNameList;
	char                  *cutPath;
	char                  *newRoot;
	class ITCPrint        *itcPrint;
	TraceDqr::nlStrings   *nlsStrings;
	TraceDqr::ADDRESS      currentAddress[DQR_MAXCORES];
	TraceDqr::ADDRESS      lastFaddr[DQR_MAXCORES];
	TraceDqr::TIMESTAMP    lastTime[DQR_MAXCORES];
	class Count            *counts;
	enum state             state[DQR_MAXCORES];
	bool                   readNewTraceMessage;
	int                    currentCore;
	bool                   eventConvert;

	bool prevMsgWasSync[DQR_MAXCORES];

	int		       currentPid[DQR_MAXCORES];
	uint8_t                currentPrv[DQR_MAXCORES];
	int		       currentProcessIndex[DQR_MAXCORES]; // index into process array, not pid array
	class Disassembler    *currentDisassembler[DQR_MAXCORES];
	class ElfReader       *currentElfReader[DQR_MAXCORES];

	int              srcbits;
	bool             bufferItc;
	int              enterISR[DQR_MAXCORES];

	int              startMessageNum;
	int              endMessageNum;

	uint32_t         eventFilterMask;

	int              tsSize;
	int              bitsPerAddress;
	TraceDqr::pathType pathType;

	uint32_t         freq;
	int              archSize;

	Analytics        analytics;

//	need current message number and list of messages??

	NexusMessage     nm;

	NexusMessage     messageInfo;
	Instruction      instructionInfo;
	Source           sourceInfo;

	int              syncCount;
	TraceDqr::ADDRESS caSyncAddr;
	class CATrace   *caTrace;
	TraceDqr::TIMESTAMP lastCycle[DQR_MAXCORES];
	int               eCycleCount[DQR_MAXCORES];

	TraceDqr::DQErr configure(class TraceSettings &settings);
	void resetTrace(int core);

	int decodeInstructionSize(uint32_t inst, int &inst_size);
	int decodeInstruction(uint32_t instruction,int &inst_size,TraceDqr::InstType &inst_type,TraceDqr::Reg &rs1,TraceDqr::Reg &rd,int32_t &immediate,bool &is_branch);
	TraceDqr::DQErr getCRBRFlags(TraceDqr::ICTReason cksrc,TraceDqr::ADDRESS addr,int &crFlag,int &brFlag);
	TraceDqr::DQErr nextAddr(TraceDqr::ADDRESS addr,TraceDqr::ADDRESS &nextAddr,int &crFlag);
	TraceDqr::DQErr nextAddr(int currentCore,TraceDqr::ADDRESS addr,TraceDqr::ADDRESS &pc,NexusMessage *nm,int &crFlag,TraceDqr::BranchFlags &brFlag);
	TraceDqr::DQErr nextCAAddr(TraceDqr::ADDRESS &addr,TraceDqr::ADDRESS &savedAddr);

	TraceDqr::ADDRESS computeAddress();
	TraceDqr::DQErr processTraceMessage(NexusMessage &nm,TraceDqr::ADDRESS &pc,TraceDqr::ADDRESS &faddr,TraceDqr::TIMESTAMP &ts,bool &consumed);
	TraceDqr::DQErr parsePidList(const char *pids);
	TraceDqr::DQErr buildElfProcess(const char *elfName);
	TraceDqr::DQErr buildMFProcesses(const char *nameList);
	TraceDqr::DQErr buildProcess(process *process,int pid,const char *mapFileName);
	TraceDqr::DQErr parseMappingFile(const char *fName,class addressMap **am,int &codeSections);
	TraceDqr::DQErr signExtendAddr(TraceDqr::ADDRESS &addr);
	TraceDqr::DQErr dumpTraceMessage(NexusMessage *tmsg);
	TraceDqr::DQErr dumpTraceMessages();
	int processPidPriv(int core,int pid,uint8_t v,uint8_t prv);
};

class SRec {
public:
	void dump();
	bool validLine;
	int line;
	uint8_t coreId;
	uint32_t cycles;
	int valid;
	bool haveFRF;
	bool haveVRF;
	TraceDqr::ADDRESS frfAddr;
	TraceDqr::ADDRESS pc;
	bool wvf;
	int wReg;
	uint32_t wVal;
	int r1Reg;
	int r2Reg;
	uint32_t r1Val;
	uint32_t r2Val;
	TraceDqr::RV_INST inst;
	TraceDqr::RV_INST dasm;
};

#ifdef SWIG
	%ignore Simulator::NextInstruction(Instruction **instInfo,NexusMessage **msgInfo,Source **srcInfo);
#endif // SWIG

class Simulator {
public:
	Simulator(char *f_name,char *e_name,const char *odExe);
	~Simulator();

	void cleanUp();
	TraceDqr::DQErr getStatus() {return status;}

	TraceDqr::DQErr getTraceFileOffset(int &size,int &offset);
	TraceDqr::DQErr Disassemble(SRec *srec);

	TraceDqr::DQErr NextInstruction(Instruction **instInfo,Source **srcInfo);
	TraceDqr::DQErr NextInstruction(Instruction *instInfo,Source *srcInfo, int *flags);

	void analyticsToText(char *dst,int dst_len,int detailLevel) {/*analytics.toText(dst,dst_len,detailLevel);*/ }
//	std::string analyticsToString(int detailLevel) { /* return analytics.toString(detailLevel);*/ }
	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);

private:
	TraceDqr::DQErr status;

	int archSize;

	char *vf_name;
	char *lineBuff;
	char **lines;
	int numLines;
	int nextLine;

	bool haveLookaheadSrec;
	SRec lookaheadSrec;

	Instruction  instructionInfo;
	Source       sourceInfo;

	class ElfReader    *elfReader; // need this class to create disassembler class
	class Symtab       *symtab;
	class Section      *sections;
	class Disassembler *disassembler;
	char               *cutPath;
	char               *newRoot;

	uint32_t instructionBuffer[2];

	uint64_t currentTime[DQR_MAXCORES];
	int  enterISR[DQR_MAXCORES];

	TraceDqr::DQErr readFile(char *file);
	TraceDqr::DQErr parseFile();
	TraceDqr::DQErr parseLine(int l,int core,SRec *srec);
	TraceDqr::DQErr getNextSrec(int nextLine,int core,SRec &srec);

//	need to rename nextAddr to compueFlags or something
	TraceDqr::DQErr computeBranchFlags(int core,TraceDqr::ADDRESS currentAddr,uint32_t currentInst,TraceDqr::ADDRESS &nextAddr,int &crFlag,TraceDqr::BranchFlags &brFlag);
	TraceDqr::DQErr buildInstructionFromSrec(SRec *srec,TraceDqr::BranchFlags brFlags,int crFlag);
};

#ifdef SWIG
	%ignore VCD::NextInstruction(Instruction **instInfo,Source **srcInfo);
	%ignore VCD::analyticsToText(char *dst,int dst_len,int detailLevel);
#endif // SWIG

class VCD {
public:
	VCD(const char *vcd_name,const char *ef_name,const char *odExe);
	VCD(const char *pf_name);
	~VCD();

	void cleanUp();
	TraceDqr::DQErr getStatus() {return status;}

	TraceDqr::DQErr Disassemble(uint64_t pc);

	TraceDqr::DQErr NextInstruction(Instruction **instInfo,Source **srcInfo);
	TraceDqr::DQErr NextInstruction(Instruction *instInfo,Source *srcInfo, int *flags);

	TraceDqr::DQErr getTraceFileOffset(int &size,int &offset);

	void analyticsToText(char *dst,int dst_len,int detailLevel) {/*analytics.toText(dst,dst_len,detailLevel);*/ }
//	std::string analyticsToString(int detailLevel) { /* return analytics.toString(detailLevel);*/ }
	TraceDqr::DQErr subSrcPath(const char *cutPath,const char *newRoot);

private:
	struct VRec {
		uint64_t ts;
		uint16_t flags;
		uint64_t pc;
	};

	enum VCDFlags {
		VCDFLAG_PIPE = 0x01
	};

	TraceDqr::DQErr status;

	int archSize;

	bool haveLookaheadVRec;
	VRec lookaheadVRec;

	Instruction  instructionInfo;
	Source       sourceInfo;

	int   vcd_fd;
	int   totalPCDRecords;
	int   nextPCDRecord;

	int   numVCDRecords;
	int   currentVCDRecord;
	uint8_t *vcdBuff;
	class ElfReader    *elfReader; // need this class to create disassembler class
	class Disassembler *disassembler;

	TraceDqr::pathType pathType;

	uint64_t currentTime[DQR_MAXCORES];
	int  enterISR[DQR_MAXCORES];

	TraceDqr::DQErr configure(class TraceSettings &settings);

	TraceDqr::DQErr getNextVRec(VRec &srec);

	TraceDqr::DQErr computeBranchFlags(int core,TraceDqr::ADDRESS currentAddr,uint32_t currentInst,TraceDqr::ADDRESS nextAddr,int &crFlag,TraceDqr::BranchFlags &brFlag);
	TraceDqr::DQErr buildInstructionFromVRec(VRec *vrec,uint32_t instruction,TraceDqr::BranchFlags brFlags,int crFlag);
};

#endif /* DQR_HPP_ */

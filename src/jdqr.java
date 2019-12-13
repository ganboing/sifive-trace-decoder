// runme.java

public class jdqr {
  static {
    System.loadLibrary("dqr");
  }

  public static void main(String argv[]) {
    Trace t = new Trace("foo.rtd",true,"coremark.elf",32,dqr.AddrDisp.ADDRDISP_WIDTHAUTO.swigValue(),0);
    if (t == null) {
      System.out.println("t is null");
      System.exit(1);
    }

    if (t.getStatus() != dqr.DQErr.DQERR_OK) {
      System.out.println("getSatus() is not OK\n");
      System.exit(1);
    }

    Instruction instInfo = new Instruction();
//    SWIGTYPE_p_p_Instruction instInfo_p = new SWIGTYPE_p_p_Instruction();

    NexusMessage msgInfo = new NexusMessage();;
//    SWIGTYPE_p_p_NexusMessage msgInfo_p = new SWIGTYPE_p_p_NexusMessage();

    Source srcInfo = new Source();
//    SWIGTYPE_p_p_Source srcInfo_p = new SWIGTYPE_p_p_Source();

    dqr.DQErr ec = dqr.DQErr.DQERR_OK;

//    System.out.println("instInfo.cptr =" + String.format("%08x",Instruction.getCPtr(instInfo)));
//    System.out.println("instInfo_p.cptr = " + Long.toHexString(SWIGTYPE_p_p_Instruction.getCPtr(instInfo_p)));

//    instInfo_p = new SWIGTYPE_p_p_Instruction(Instruction.getCPtr(instInfo),false);
//    SWIGTYPE_p_p_Instruction.SWIGTYPE_p_p_Instruction(Instruction.getCPtr(instInfo));
//    System.out.printf("instInfo_p.cptr = %08x\n",SWIGTYPE_p_p_Instruction.getCPtr(instInfo_p));

    SWIGTYPE_p_int flags = tracedecoder.new_intp();

//    byte b[] = new byte[128];

    boolean func_flag = true;
    boolean file_flag = true;
    boolean dasm_flag = true;
    boolean src_flag = true;
    boolean itcPrint_flag = true;
    boolean trace_flag = true;
    int srcBits = 0;
    long lastAddress = 0;
    int lastInstSize = 0;
    String lastSrcFile = new String();
    String lastSrcLine = new String();
    int lastSrcLineNum = 0;
    int instLevel = 1;
    int msgLevel = 2;
    boolean firstPrint = true;
    String stripPath = "foo";

    while (ec == dqr.DQErr.DQERR_OK) {
	tracedecoder.intp_assign(flags,0);

      ec = t.NextInstruction(instInfo,msgInfo,srcInfo,flags);

      if (ec == dqr.DQErr.DQERR_OK) {
        if ((tracedecoder.intp_value(flags) & dqr.TRACE_HAVE_SRCINFO) != 0) {
          String sourceFile = srcInfo.sourceFileToString(stripPath);
	  String sourceLine = srcInfo.sourceLineToString();
          int sourceLineNum = (int)srcInfo.getSourceLineNum();

          if ((lastSrcFile.compareTo(sourceFile) != 0) || (lastSrcLine.compareTo(sourceLine) != 0) || (lastSrcLineNum != sourceLineNum)) {
            lastSrcFile = sourceFile;
            lastSrcLine = sourceLine;
            lastSrcLineNum = sourceLineNum;

            if (file_flag) {
              if (sourceFile != null && sourceFile.length() != 0) {
                if (firstPrint == false) {
                  System.out.printf("%n");
                }

                if (srcBits > 0) {
                  System.out.printf("[%d] File: %s:%d\n",srcInfo.getCoreId(),sourceFile,sourceLineNum);
                }
                else {
                  System.out.printf("File: %s:%d\n",sourceFile,sourceLineNum);
                }

                firstPrint = false;
              }
            }

            if (src_flag) {
              if (sourceLine != null && sourceLine.length() != 0) {
                if (srcBits > 0) {
                  System.out.printf("[%d] Source: %s\n",srcInfo.getCoreId(),sourceLine);
                }
                else {
                  System.out.printf("Source: %s\n",sourceLine);
                }
                firstPrint = false;
              }
            }
          }
        }

        if ((dasm_flag == true) && ((tracedecoder.intp_value(flags) & dqr.TRACE_HAVE_INSTINFO) != 0)) {

          long address = instInfo.getAddress().longValue();

          if (func_flag) {
            if (srcBits > 0) {
              System.out.printf("[%d] ",instInfo.getCoreId());
            }

            if (address != (lastAddress + lastInstSize / 8)) {
              String addressLabel = instInfo.getAddressLabel();
              if (addressLabel != null && addressLabel.length() != 0) {
                System.out.printf("<%s",addressLabel);
                if (instInfo.getAddressLabelOffset() != 0) {
                  System.out.printf("+%x",instInfo.getAddressLabelOffset());
                }
                System.out.printf(">%n");
              }
            }
	    
            lastAddress = address;
            lastInstSize = instInfo.getInstSize();
          }

          if (srcBits > 0) {
            System.out.printf("[%d] ", instInfo.getCoreId());
          }

          String dst = String.format("    %s:",instInfo.addressToString(0));
          System.out.print(dst);

          int n = dst.length();

          for (int i = n; i < 20; i++) {
            System.out.print(" ");
          }

          dst = instInfo.instructionToString(instLevel);
          System.out.printf("  %s",dst);

          System.out.printf("%n");

          firstPrint = false;
        }

	if ((trace_flag || itcPrint_flag) && ((tracedecoder.intp_value(flags) & dqr.TRACE_HAVE_MSGINFO) != 0)) {
          SWIGTYPE_p_int msgFlags = tracedecoder.new_intp();

          String msgStr = msgInfo.messageToString(msgLevel,msgFlags);

          if (trace_flag) {
	    if (!firstPrint) {
	      System.out.printf("%n");
	    }

            if (srcBits > 0) {
              System.out.printf("[%d] ",msgInfo.getCoreId());
            }

            System.out.printf("Trace: %s",msgStr);

            System.out.printf("%n");

            firstPrint = false;
          }

          if (itcPrint_flag && ((tracedecoder.intp_value(msgFlags) & dqr.TRACE_HAVE_ITCPRINT) != 0)) {
            if (firstPrint == false) {
              System.out.printf("%n");
            }

            if (srcBits > 0) {
              System.out.printf("[%d] ",msgInfo.getCoreId());
            }

	    String itcprint = msgInfo.itcprintToString();

	    System.out.println(itcprint);

	    firstPrint = false;
          }
        }
      }
    }

    t.displayAnalytics(1);
  }
}

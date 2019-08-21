# itcprint.c

The itcprint.c source file contains two functions that can be used to inject printf style output into the trace stream.

## int itcprintf(const char *format, ... )

Formats a string with provided parameters, then calls itcprinstr() to inject it into the trace output.

## int itcprintstr(char* string)

Simply injects a fixed string into the trace stream.  This string can then be seen in the trace output from the trace decoder.
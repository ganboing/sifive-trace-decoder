REM FS_DEV_TDC_DEST points to the devroot/trace-decoder package being used by Freedom Studio under the debugger.
REM FS_DEV_TDC_JAR_DEST points to the root of the trace plugin in the workspace.
IF "%FS_DEV_TDC_DEST%"=="" exit FS_DEV_TDC_DEST is NOT defined
IF "%FS_DEV_TDC_JAR_DEST%"=="" exit FS_DEV_TDC_JAR_DEST is NOT defined
make clean Debug install
mkdir %FS_DEV_TDC_DEST%
xcopy /S /Y install\* %FS_DEV_TDC_DEST%
copy /Y install\lib\TraceDecoder.jar %FS_DEV_TDC_JAR_DEST%
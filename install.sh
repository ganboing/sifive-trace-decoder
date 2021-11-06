#!/bin/bash
set -u # or set -o nounset
: "$FS_DEV_TDC_DEST"
: "$FS_DEV_TDC_JAR_DEST"
#REM FS_DEV_TDC_DEST points to the devroot/trace-decoder package being used by Freedom Studio under the debugger.
#REM FS_DEV_TDC_JAR_DEST points to the root of the trace plugin in the workspace.
make clean Debug install
mkdir -p $FS_DEV_TDC_DEST >/dev/null 2>&1
cp -r install/* $FS_DEV_TDC_DEST
cp install/lib/TraceDecoder.jar $FS_DEV_TDC_JAR_DEST
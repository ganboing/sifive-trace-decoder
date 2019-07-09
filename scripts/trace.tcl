#
# Scripts for trace using OpenOCD
#
# riscv set_prefer_sba on

set te_control $baseaddress
set te_impl [expr $baseaddress + 0x04]
set te_sinkbase [expr $baseaddress + 0x10]
set te_sinkbasehigh [expr $baseaddress + 0x14]
set te_sinklimit [expr $baseaddress + 0x18]
set te_sinkwp [expr $baseaddress + 0x1c]
set te_sinkrp [expr $baseaddress + 0x20]
set te_sinkdata [expr $baseaddress + 0x24]
set te_fifo [expr $baseaddress + 0x30]
set te_btmcount [expr $baseaddress + 0x34]
set te_wordcount [expr $baseaddress + 0x38]
set ts_control [expr $baseaddress + 0x40]
set ts_lower [expr $baseaddress + 0x44]
set te_upper [expr $baseaddress + 0x48]
set xti_control [expr $baseaddress + 0x50]
set xto_control_ [expr $baseaddress + 0x54]
set wp_control [expr $baseaddress + 0x58]
set itc_traceenable [expr $baseaddress + 0x60]
set itc_trigenable [expr $baseaddress + 0x64]

proc wordhex {addr} {
    mem2array x 32 $addr 1
    return [format "0x%08x" [lindex $x 1]]
}

proc word {addr} {
    mem2array x 32 $addr 1
    return [lindex $x 1]
}

proc pt {{range "end"}} {
  global te_sinkrp
  global te_sinkdata
  if {$range == "start"} {
    set tracewp [word $te_sinkwp]
    if {$tracewp == 0} {
      echo "no trace collected"
      return
    }
    if {($tracewp & 1) == 0} {      ;# buffer has not wrapped
	set tracebegin 0
	set traceend [expr $tracewp & 0xfffffffe]
        if {$traceend > 0x20} {set traceend 0x20}
    } else {
        set tracebegin [expr $tracewp & 0xfffffffe]
        set traceend [expr $tracebegin + 0x20]
    }
  } else {
    set tracewp [word $te_sinkwp]
    if {$tracewp == 0} {
      echo "no trace collected"
      return
    }
    if {($tracewp & 1) == 0} {      ;# buffer has not wrapped
	set traceend [expr $tracewp & 0xfffffffe]
	set tracebegin [expr $traceend - 0x20]
	if {$tracebegin < 0} {set tracebegin 0}
    } else {
	set traceend [expr $tracewp & 0xfffffffe]
	set tracebegin [expr $traceend - 0x20]
    }
  }
  mww $te_sinkrp $tracebegin
  for {set i $tracebegin} {$i < $traceend} {incr i 4} {
    echo "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
  }
}

proc dt {} {
  global tracesize
  global te_sinkrp
  global te_sinkwp
  global te_sinkdata
  set tracewp [word $te_sinkwp]
  if {($tracewp & 1) == 0 } {	;# buffer has not wrapped
    set tracebegin 0
    set traceend $tracewp
    echo "Trace from $tracebegin to $traceend, nowrap, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i 0} {$i < $traceend} {incr i 4} {
      echo "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
  } else {
    echo "Trace wrapped"
    set tracebegin [expr $tracewp & 0xfffffffe]
    set traceend $tracesize
    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp $tracebegin
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      echo "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
    set tracebegin 0
    set traceend [expr $tracewp & 0xfffffffe]
    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      echo "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
  }
}

proc wt {{file "trace.txt"}} {
  global tracesize
  global te_sinkrp
  global te_sinkwp
  global te_sinkdata
  set fp [open "$file" w]
  set tracewp [word $te_sinkwp]
  if {($tracewp & 1) == 0 } {	;# buffer has not wrapped
    set tracebegin 0
    set traceend $tracewp
    puts $fp "Trace from $tracebegin to $traceend, nowrap, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i 0} {$i < $traceend} {incr i 4} {
      puts $fp "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
  } else {
    puts $fp "Trace wrapped"
    set tracebegin [expr $tracewp & 0xfffffffe]
    set traceend $tracesize
    puts $fp "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp $tracebegin
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      puts $fp "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
    set tracebegin 0
    set traceend [expr $tracewp & 0xfffffffe]
    puts $fp "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      puts $fp "[format {%4d: 0x%08x} $i [word $te_sinkdata]]"
    }
  }
  close $fp
}

proc wtb {{file "trace.rtd"}} {
  riscv set_prefer_sba on
  global tracesize
  global te_sinkrp
  global te_sinkwp
  global te_sinkdata
  set fp [open "$file" wb]
  set tracewp [word $te_sinkwp]
  if {($tracewp & 1) == 0 } { ;# buffer has not wrapped
    set tracebegin 0
    set traceend $tracewp
    echo "Trace from $tracebegin to $traceend, nowrap, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i 0} {$i < $traceend} {incr i 4} {
      pack w [word $te_sinkdata] -intle 32
      puts -nonewline $fp $w
    }
  } else {
    echo "Trace wrapped"
    set tracebegin [expr $tracewp & 0xfffffffe]
    set traceend $tracesize
    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp $tracebegin
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      pack w [word $te_sinkdata] -intle 32
      puts -nonewline $fp $w
    }
    set tracebegin 0
    set traceend [expr $tracewp & 0xfffffffe]
    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"
    mww $te_sinkrp 0
    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      pack w [word $te_sinkdata] -intle 32
      puts -nonewline $fp $w
    }
  }
  close $fp
  riscv set_prefer_sba off
}

# ite = [i]s [t]race [e]nabled
proc ite {} {
    global te_control
    set tracectl [word $te_control]
    if {($tracectl & 0x6) == 0} {
        return "0"
    } else {
        return "1"
    }
}

proc cleartrace {} {
    global tracesize
    global te_sinkrp
    global te_sinkwp
    global te_sinkdata
    mww $te_sinkrp 0
    for {set i 0} {$i<$tracesize} {incr i 4} {
      mww $te_sinkdata 0
    }
    mww $te_sinkwp 0
}  

proc resettrace {} {
    global te_control
    mww $te_control 0            ;# reset the TE
    mww $te_control 0x01830001   ;# activate the TE
}

proc starttrace {} {
    global te_control
    global te_sinkwp
    mww $te_control 0x01830001   ;# disable trace
    mww $te_sinkwp 0             ;# clear WP
    mww $te_control 0x01830037   ;# enable trace
}

proc stoptrace {} {
    global te_control
    global te_sinkwp
    global tracesize
    mww $te_control 0x01830003   ;# stop trace
    mww $te_control 0x01830001   ;# disable and flush trace
    set tracewp [word $te_sinkwp]
}

proc wordscollected {} {
    global tracesize
    global te_sinkwp
    set tracewp [word $te_sinkwp]
    if {$tracewp & 1} {
	      return "[expr $tracesize / 4] trace words collected"
    } else {
        return "[expr $tracewp / 4] trace words collected"
    }
}

proc analytics {} {
  global te_fifo
  global te_btmcount
  global te_wordcount
  echo "trace BTM count:  [word $te_btmcount]"
  echo "trace word count: [word $te_wordcount]"
  set analytics0 [word $te_fifo]
  echo "max fifo usage:   [expr $analytics0 & 0x7fffffff]"
  echo "overflow occur:   [expr ($analytics0 >> 30) & 1]"
}


set btmRP 0
set btmWord 0

proc nextbyte {} {
  global te_sinkdata
  global btmRP
  global btmWord
  if {($btmRP & 3) == 0} {
    set btmWord [word $te_sinkdata]
  }
  set retval [expr (($btmWord >> (($btmRP & 3)*8)) & 0xFF)]
  incr btmRP 
  return $retval
}


proc btype {val} {
    if {($valF & 3) == 1} { return "Exception" }
    if {($valF & 3) == 0} { return "Branch" }
    return "Unknown BTYPE Value"
}

proc sync {val} {
    if {($val & 0xf) == 0} { return "External Trigger" }
    if {($val & 0xf) == 1} { return "Exit Reset" }
    if {($val & 0xf) == 2} { return "Message Counter" }
    if {($val & 0xf) == 3} { return "Exit Debug" }
    if {($val & 0xf) == 4} { return "Instruction Counter" }
    if {($val & 0xf) == 5} { return "Trace Enable" }
    if {($val & 0xf) == 6} { return "Watchpoint" }
    if {($val & 0xf) == 7} { return "Restart After Overflow" }
    if {($val & 0xf) == 15} { return "PC Sample" }
    return "Unknown SYNC Value"
}

proc evcode {val} {
    if {($val & 0xf) == 0} { return "Enter Debug" }
    if {($val & 0xf) == 4} { return "Trace Disable" }
    if {($val & 0xf) == 8} { return "Enter Reset" }
    return "Unknown EVCODE Value"
}

proc btm {{range "end"}} {
  global btmRP
  global te_sinkrp
  global te_sinkwp
  if {$range == "start"} {
    set tracewp [word $te_sinkwp]
    if {$tracewp == 0} {
      echo "no trace collected"
      return
    }
    if {($tracewp & 1) == 0} {      ;# buffer has not wrapped
	set tracebegin 0
	set traceend [expr $tracewp & 0xfffffffe]
        if {$traceend > 0x20} {set traceend 0x20}
    } else {
        set tracebegin [expr $tracewp & 0xfffffffe]
        set traceend [expr $tracebegin + 0x20]
    }
  } else {
    set tracewp [word $te_sinkwp]
    if {$tracewp == 0} {
      echo "no trace collected"
      return
    }
    if {($tracewp & 1) == 0} {      ;# buffer has not wrapped
	set traceend [expr $tracewp & 0xfffffffe]
	set tracebegin [expr $traceend - 0x40]
	if {$tracebegin < 0} {set tracebegin 0}
    } else {
	set traceend [expr $tracewp & 0xfffffffe]
	set tracebegin [expr $traceend - 0x40]
    }
  }

  mww $te_sinkrp $tracebegin
  set btmRP $tracebegin
  if {$tracebegin > 0} findbtmend
  while {$btmRP < $traceend} {
      printonebtm
  }
}


proc findbtmend {} {
    for {set i 0} {$i<10} {incr i} {    ;# scan ahead for next EOM
	set b [nextbyte]
	if {($b & 3) == 3} break
    }
}

proc printonebtm {} {
    global btmRP
    set icntF 0
    set addrF 0
    set timestamp 0
    set b [nextbyte]

    set tcode [expr $b >> 2]
    if {$b == 0xff} return     ;# Idle Slice - no output

    for {set i 0} {$i<10} {incr i} {     ;# max out at 10 slices 
	set b [nextbyte]
        set icntF [expr $icntF | (($b >> 2) & 0x3f) << (6*$i)]
	if {$b & 3} break;
    }
    set hasAddr [expr ($tcode==4) || ($tcode==11) || ($tcode==12) || ($tcode==9)]
    if {$hasAddr} {
	if {(($b & 3) != 1)} {
	    echo "FORMAT ERROR: expected ADDR but found EOM"
	} else {
	    for {set i 0} {$i<10} {incr i} {
		set b [nextbyte]
		set addrF [expr $addrF | (($b >> 2) & 0x3f) << (6*$i)]
		if {$b & 3} break;
	    }
	}
    }
    if {($b & 3) != 3} {
	set b [nextbyte]
        set timestamp [expr $timestamp | (($b >> 2) & 0x3f) << (6*$i)]
	if {$b & 3} break;
    }

    if {$timestamp} { echo "TIME $timestamp" }

    if {$tcode == 3}  { echo [format "%4d: Direct, Icnt $icntF" $btmRP]}
    if {$tcode == 4}  { echo [format "%4d: Indirect, [btype $icntF], Icnt [expr $icntF >> 2], Uaddr 0x%08x" $btmRP $addrF] }
    if {$tcode == 11} { echo [format "%4d: DirectSync, [sync $icntF], Icnt [expr $icntF >> 4], Faddr 0x%08x" $btmRP $addrF] }
    if {$tcode == 12} { echo [format "%4d: IndirectSync, [sync $icntF], [btype [expr $icntF >> 4]], Icnt [expr $icntF >> 6], Faddr 0x%08x" $btmRP $addrF] }
    if {$tcode == 9}  { echo [format "%4d: PTSync, [sync $icntF], Icnt [expr $icntF >> 4], Faddr 0x%08x" $btmRP $addrF] }
    if {$tcode == 8}  { echo [format "%4d: Error, Queue Overrun" $btmRP]}
    if {$tcode == 33} { echo [format "%4d: PTCorr, [evcode $icntF], Icnt [expr $icntF >> 6]" $btmRP]}
    if {$tcode == 23} { echo [format "%4d: ITC Write, Addr [expr $icntF], Data [expr $addrF]" $btmRP]}
    if {$tcode == 2}  { echo [format "%4d: Ownership, Process [expr $icntF]" $btmRP] }
}

mww $te_control 0x01830001
mww $te_sinkrp 0xffffffff
mww $te_sinkwp 0x00000000
set tracesize [expr [word $te_sinkrp] + 4]


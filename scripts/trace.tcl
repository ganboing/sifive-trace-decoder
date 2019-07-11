#
# Scripts for trace using OpenOCD
#

# riscv set_prefer_sba on

set baseaddress 0x20007000
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

proc ts {{opt ""}} {
  global ts_flag
  global te_control
  if {$opt == ""} {
    if {$ts_flag == 0} {
      echo "timestamps are off"
    } else {
      echo "timestamps are on"
    }
  } elseif {$opt == "help"} {
    echo "ts: set or display timestamp mode"
    echo {Usage: ts [on | off | help]}
    echo "  on:   Enable timestamps in trace messages"
    echo "  off:  Disable timstamps in trace messages"
    echo "  help: Display this message"
    echo ""
    echo "ts with no arguments will display the current status of timestamps (on or off)"
    echo ""
  } elseif {[expr [word $te_control] & 0x02] != 0} {
    echo "Error: Cannot set timestamp mode while trace is enabled"
  } elseif {$opt == "on"} {
    set ts_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set ts_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: ts [on | off | help]}
  }
}

proc stoponwrap {{opt ""}} {
  global sow_flag
  global te_control
  if {$opt == ""} {
    if {$sow_flag == 0} {
      echo "stop on wrap is off (disabled)"
    } else {
      echo "stop on wrap is on (enabled)"
    }
  } elseif {$opt == "help"} {
    echo "stoponwrap: set or display trace buffer wrap mode"
    echo {Usage: stoponwrap [on | off | help]}
    echo "  on:   Enable stop trace collection when buffer is full (default)"
    echo "  off:  Continue tracing when the buffer fills, causing it to wrap"
    echo "  help: Display this message"
    echo ""
    echo "stoponwrap with no arguments will display the current status of trace buffer wrap (on or off)"
    echo ""
  } elseif {[expr [word $te_control] & 0x02] != 0} {
    echo "Error: Cannot set wrap mode while trace is enabled"
  } elseif {$opt == "on"} {
    set sow_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set sow_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: stoponwrap [on | off | help]}
  }
}

proc tracemode {{opt ""}} {
  global tm_flag
  global te_control
  if {$opt == ""} {
    switch $tm_flag {
      0 {
        echo "tracemode: Do not generate BTM or Sync messages"
      }
      1 {
        echo "tracemode: Generate Sync messages only (no BTM messages)"
      }
      3 {
        echo "tracemode: Generate both BTM and Symc messages"
      }
      default {
        echo "Error: tracemode: invalid trace type: $tm_flag"
      }
    }
  } elseif {$opt == "help"} {
    echo "tracemode: set or display trace type (sync, sync+btm)"
    echo {Usage: tracemode [sync | all | none | help]}
    echo "  sync: Generate only sync trace messages"
    echo "  all:  Generate both sync and btm trace messages"
    echo "  none: Do not generate sync or btm trace messages"
    echo "  help: Display this message"
    echo ""
    echo "tracemode with no arguments will display the current setting for the type"
    echo "of messages to generate"
    echo ""
  } elseif {[expr [word $te_control] & 0x02] != 0} {
    echo "Error: Cannot set trace mode while trace is enabled"
  } elseif {$opt == "sync"} {
    set tm_flag 1
    echo -n ""
  } elseif {$opt == "all"} {
    set tm_flag 3
    echo -n ""
  } elseif {$opt == "none"} {
    set tm_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: tracemode [sync | all | none | help]}
  }
}

proc itc {{opt ""} {mask ""}} {
  global itc_mode
  global itc_mask
  global te_control
  if {$opt == ""} {
    switch $itc_mode {
      0 {
        echo "itc: All ITC messages are disabled"
      }
      1 {
        echo "itc: All ITC messages are enbaled"
        echo [format "ITC mask: 0x%08x" $itc_mask]
      }
      2 {
        echo "itc: Generate ownership messages for stimulus 15 and 31"
        echo [format "ITC mask: 0x%08x" $itc_mask]
      }
      3 {
        echo "itc: Generate ownership messages for stimulus 15 and 31, and ITC messages"
        echo "for all other stimulus"
        echo [format "ITC mask: 0x%08x" $itc_mask]
      }
      default {
        echo "Error: itc: invalid itc mode: $itc_mode"
      }
    }
  } elseif {$opt == "help"} {
    echo "itc: set or display itc settings"
    echo {Usage: itc [off | ownership | all | all+ownership | mask nn | help]}
    echo "  off:           Disable ITC message generation"
    echo "  ownership:     Enable ITC messages for stiumlus 15 and 31 only. Set the stimulus"
    echo "                 mask to 0x80008000"
    echo "  all:           Enable ITC messages for all stimulus 0 - 31. Set the stimulus"
    echo "                 mask to 0xffffffff"
    echo "  all+ownership: Generate ownership messages for stimulus 15 and 32, and"
    echo "                 ITC messages for all other stimulus. Set the stimulus mask to"
    echo "                 0xffffffff"
    echo "  mask nn:       Set the stimulus mask to nn, where nn is a 32 bit number. Note"
    echo "                 nn should be prefixed with 0x if it is a hex number, or just 0 if"
    echo "                 it is an octal number; otherwise it will be interpreted as a decimal"
    echo "                 number. Does not effect the ITC mode (off, ownership, all, all+ownership)"
    echo "  help:          Display this message"
    echo ""
    echo "itc with no arguments will display the current itc settings"
    echo ""
  } elseif {$opt == "off"} {
    set itc_mode 0
    set itc_mask 0
    echo -n ""
  } elseif {$opt == "all"} {
    set itc_mode 1
    set itc_mask 0xffffffff
    echo -n ""
  } elseif {$opt == "ownership"} {
    set itc_mode 2
    set itc_mask 0x80008000
    echo -n ""
  } elseif {$opt == "all+ownership"} {
    set itc_mode 3
    set itc_mask 0xffffffff
    echo -n ""
  } elseif {$opt == "mask"} {
    if {$mask == ""} {
      echo [format "ITC mask: 0x%08x" $itc_mask]
    } else {
      set itc_mask [expr $mask]
      echo -n ""
    }
  } else {
    echo {Error: Usage: itc [off | ownership | all | all+ownership | mask nn | help]}
  }
}

proc maxicnt {{opt ""}} {
  global max_icnt_val
  global te_control
  if {$opt == ""} {
      echo "max icnt: [expr {$max_icnt_val + 5}] ([expr {2 ** ($max_icnt_val + 5)}])"
  } elseif {$opt == "help"} {
    echo "maxicnt: set or dipspaly the maximum i-cnt field"
    echo {Usage: maxicnt [nn | help]}
    echo "  nn: Set max i-cnt value to 2^(nn+5). nn must be between 5 and 16 for"
    echo "      a range between 32 and 65536"
    echo "  help: Display this message"
    echo ""
    echo "maxicnt with no arguments will display the current maximum i-cnt value"
    echo ""
  } elseif {[expr [word $te_control] & 0x02] != 0} {
    echo "Error: Cannot set the max i-cnt value while trace is enabled"
  } elseif {$opt >= 5 && $opt <= 16} {
    set max_icnt_val [expr {$opt - 5}]
    echo -n ""
  } else {
    echo {Error: Usage: maxicnt [5 - 16]}
  }
}

proc maxbtm {{opt ""}} {
  global max_btm_val
  global te_control
  if {$opt == ""} {
    echo "max btm: [expr {$max_btm_val + 5}] ([expr {2 ** ($max_btm_val + 5)}])"
  } elseif {$opt == "help"} {
    echo "maxbtm: set or display the maximum number of BTMs between Sync messages"
    echo {Usage: maxbtm [nn | help]}
    echo "  nn:   Set the maximum number of BTMs between Syncs to nn. nn must be between"
    echo "        5 and 16 for a range between 32 and 65536"
    echo "  help: Display this message"
    echo ""
    echo "maxbtm with no arguments will display the current maximum number of BTMs"
    echo "between sync messages"
    echo ""
  } elseif {[expr [word $te_control] & 0x02] != 0} {
    echo "Error: Cannot set the maxbtm value while trace is enabled"
  } elseif {$opt >= 5 && $opt <= 16} {
    set max_btm_val [expr {$opt - 5}]
    echo -n ""
  } else {
    echo {Error: Usage: maxbtm [5 - 16 | help]}
  }
}

proc tracesettings {} {
  itc
  maxicnt
  maxbtm
  tracemode
  stoponwrap
  ts
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

proc readts {} {
	global ts_control
        echo "[format {0x%08x} [word $ts_control]]"
}

proc resetts {} {
    global te_control
    global ts_control
    if {( [expr [word $te_control] & 0x02] != 0)} {
	    echo "cannot reset timestamp while trace is enabled"
    } else {
	    mww $ts_control 0x04
	    mww $ts_control 0x00
	    echo "timestamp reset"
    }
}

proc starttrace {} {
    global ts_control
    global te_control
    global te_sinkwp
    global tm_flag
    global ts_flag
    global sow_flag
    global itc_mode
    global itc_mask
    global max_icnt_val
    global max_btm_val
    global itc_traceenable

    mww $te_control 0x00000001   ;# disable trace
    mww $te_sinkwp 0             ;# clear WP

    set te_control_val 0
    set ts_control_val 0
    set itc_val 0

    if {($ts_flag != 0)} {
	    set ts_control_val 0x000f8023
    }

    set te_control_val [expr $te_control_val | ($tm_flag << 4)]

    set te_control_val [expr $te_control_val | ($itc_mode << 7)]

    if {($sow_flag != 0)} {
	    set te_control_val [expr $te_control_val | (1 << 14)]
    }

    set te_control_val [expr $te_control_val | ($max_btm_val << 16)]

    set te_control_val [expr $te_control_val | ($max_icnt_val << 20)]

    set te_control_val [expr $te_control_val | (1 << 24) | 0x07]

    mww $ts_control $ts_control_val
    mww $itc_traceenable $itc_mask
    mww $te_control $te_control_val

#    echo "tm_flag: $tm_flag"
#    echo "itc_mode: $itc_mode"
#    echo "sow_flag: $sow_flag"
#    echo "max_btm_val: $max_btm_val"
#    echo "max_icnt_val: $max_icnt_val"
#    echo [format "ts_control: 0x%08x" $ts_control_val]
#    echo [format "itc_traceenable: 0x%08x" $itc_mask]
#    echo [format "te_control: 0x%08x" $te_control_val]
}

proc stoptrace {} {
    global te_control
    global te_sinkwp
    global tracesize
    mww $te_control 0x01830003   ;# stop trace
    mww $te_control 0x01830001   ;# disable and flush trace
    set tracewp [word $te_sinkwp]
    if {$tracewp & 1} {
	echo "[expr $tracesize / 4] trace words collected"
    } else {
        echo "[expr $tracewp / 4] trace words collected"
    }
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
echo $tracesize

set tm_flag 3
set ts_flag 0
set sow_flag 1
set itc_mode 0
set itc_mask 0
set max_icnt_val 8
set max_btm_val 3

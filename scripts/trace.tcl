#
# Scripts for trace using OpenOCD
#

# riscv set_prefer_sba on

# set traceBaseAddresses {0x20007000 0x20008000}
# set traceFunnelAddress 0x20009000

set te_control_offset      0x00
set te_impl_offset         0x04
set te_sinkbase_offset     0x10
set te_sinkbasehigh_offset 0x14
set te_sinklimit_offset    0x18
set te_sinkwp_offset       0x1c
set te_sinkrp_offset       0x20
set te_sinkdata_offset     0x24
set te_fifo_offset         0x30
set te_btmcount_offset     0x34
set te_wordcount_offset    0x38
set ts_control_offset      0x40
set ts_lower_offset        0x44
set te_upper_offset        0x48
set xti_control_offset     0x50
set xto_control_offset     0x54
set wp_control_offset      0x58
set itc_traceenable_offset 0x60
set itc_trigenable_offset  0x64

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
  global tracebuffersize
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
    set traceend $tracebuffersize
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
  global tracebuffersize
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
    set traceend $tracebuffersize
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

# ite = [i]s [t]race [e]nabled

proc ite {} {
    global te_control_offset
    global traceBaseAddresses
    global traceFunnelAddress

    set rc 0

    foreach baseAddress $traceBaseAddresses {
	set tracectl [word [expr $baseAddress + $te_control_offset]]
	if {($tracectl & 0x6) != 0} {
	    return 1
        }
    }

    if {$traceFunnelAddress != 0} {
	set tracectl [word [expr $traceFunnelAddress + $te_control_offset]]
	if {($tracectl & 0x6) != 0} {
	    return 1
        }
    }

    return 0
}

proc setAllTeControls {offset val} {
  global traceBaseAddresses

  foreach controlReg $traceBaseAddresses {
    mww [expr $controlReg + $offset] $val
  }
}

proc setAllTfControls {offset val} {
  global traceFunnelAddress

  if {$traceFunnelAddress != 0} {
    mww [expr $traceFunnelAddress + $offset] $val
  }
}

proc ts {{opt ""}} {
  global tsenable_flag
  global ts_control_offset

  if {$opt == ""} {
    if {$tsenable_flag == 0} {
      return "off"
    } else {
      return "on"
    }
  } elseif {$opt == "help"} {
    echo "ts: set or display timestamp mode"
    echo {Usage: ts [on | off | reset | help]}
    echo "  on:    Enable timestamps in trace messages"
    echo "  off:   Disable timstamps in trace messages"
    echo "  reset: Reset the internal timestamp to 0"
    echo "  help:  Display this message"
    echo ""
    echo "ts with no arguments will display the current status of timestamps (on or off)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set timestamp mode or reset timestamp while trace is enabled"
  } elseif {$opt == "on"} {
    set tsenable_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set tsenable_flag 0
    echo -n ""
  } elseif {$opt == "reset"} {
    setAllTeControls $ts_control_offset 0x04
    setAllTeControls $ts_control_offset 0x00
    echo "timestamp reset"
  } else {
    echo {Error: Usage: ts [on | off | reset | help]}
  }
}

proc tsdebug {{opt ""}} {
  global tsdebug_flag
  if {$opt == ""} {
    if {$tsdebug_flag == 0} {
      return "off"
    } else {
      return "on"
    }
  } elseif {$opt == "help"} {
    echo "tsdebug: set or display if timestamp internal clock runs while in debug"
    echo {Usage: tsdebug [on | off | help]}
    echo "  on:   Timestamp clock continues to run while in debug"
    echo "  off:  Timnestamp clock halts while in debug"
    echo "  help: Display this message"
    echo ""
    echo "tsdebug with no arguments will display the current status of timstamp debug"
    echo "(on or off)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set timestamp debug mode while trace is enabled"
  } elseif {$opt == "on"} {
    set tsdebug_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set tsdebug_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: tsdebug [on | off | help]}
  }
}

proc srcbits {} {
  global te_impl_offset
  global traceBaseAddresses

  set numbits [expr [word [expr [lindex $traceBaseAddresses 0] + $te_impl_offset]] >> 24 & 7]
  return $numbits
}

proc tsclock {{opt ""}} {
  global tstype_flag
  if {$opt == ""} {
    switch $tstype_flag {
      0 {
	return "none"
      }
      1 {
	return "external"
      }
      2 {
	return "internal"
      }
      default {
	return "invalid"
      }
    }
  } elseif {$opt == "help"} {
    echo "tsclock: set or display the source of the timestamp clock (internal or external)"
    echo {Usage: tsclock [none | internal | external | help]}
    echo "  none:     No source for the timestampe clock"
    echo "  internal: Set the source of the timestamp clock to internal"
    echo "  external: Set the srouce of the timestamp clock to external"
    echo "  help:     Display this message"
    echo ""
    echo "tsclock with no arguments will display the current source of hte timestamp clock"
    echo "(internal, external, or none)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set timestamp clock source while trace is enabled"
  } elseif {$opt == "none" } {
    set tstype_flag 0
    echo -n ""
  } elseif {$opt == "internal"} {
    set tstype_flag 2
    echo -n ""
  } elseif {$opt == "external"} {
    set tstype_flag 1
    echo -n ""
  } else {
    echo {Error: Usage: tsclock [none | internal | external | help]}
  }
}

proc tsprescale {{opt ""}} {
  global tsprescale_flag
  if {$opt == ""} {
    switch $tsprescale_flag {
      0 {
	return "1"
      }
      1 {
	return "4"
      }
      2 {
	return "16"
      }
      3 {
	return "64"
      }
      default {
	return "invalid"
      }
    }
  } elseif {$opt == "help"} {
    echo "tsprescale: set or display the timesampe clock prescalser (1, 4, 16, or 64)"
    echo {Usage: tsprescale [1 | 4 | 16 | 64 | help]}
    echo "   1:    Set the prescaler to 1"
    echo "   4:    Set the prescaler to 4"
    echo "  16:    Set the prescaler to 16"
    echo "  64:    Set the prescaler to 64"
    echo "  help:  Display this message"
    echo ""
    echo "tspresacle with no arguments will display the current timestamp clock prescaler value (1, 4, 16, or 64)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set timestamp clock prescaler while trace is enabled"
  } elseif {$opt == 1} {
    set tsprescale_flag 0
    echo -n ""
  } elseif {$opt == 4} {
    set tsprescale_flag 1
    echo -n ""
  } elseif {$opt == 16} {
    set tsprescale_flag 2
    echo -n ""
  } elseif {$opt == 64} {
    set tsprescale_flag 3
    echo -n ""
  } else {
    echo {Error: Usage: tsprescale [1 | 4 | 16 | 64 | help]}
  }
}

proc tsbranch {{opt ""}} {
  global tsbranch_flag
  if {$opt == ""} {
    switch $tsbranch_flag {
      0 {
	return "off"
      }
      1 {
	return "indirect"
      }
      3 {
	return "all"
      }
      default {
	echo "Invalid setting for tsbranch"
      }
    }
  } elseif {$opt == "help"} {
    echo "tsbranch: set or display if timestamps are generated for branch messages"
    echo {Usage: tsbranch [off | indirect | all | help]}
    echo "  off:      Timestamps are not generated for branch messages"
    echo "  indirect: Generate timestamps for all indirect branch and exception messages"
    echo "  all:      Generate timestamps for all branch, exception, PTCM, and Error messages"
    echo "  help:     Display this message"
    echo ""
    echo "tsbranch with no arguments will display the current setting for tsbranch (off, indirect, all)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set branch timestamp mode while trace is enabled"
  } elseif {$opt == "off"} {
    set tsbranch_flag 0
    echo -n ""
  } elseif {$opt == "indirect"} {
    set tsbranch_flag 1
    echo -n ""
  } elseif {$opt == "all"} {
    set tsbranch_flag 3
    echo -n ""
  } else {
    echo {Error: Usage: tsbranch [off | indirect | all | help]}
  }
}

proc tsitc {{opt ""}} {
  global tsitc_flag
  if {$opt == ""} {
    if {$tsitc_flag == 0} {
      return "off"
    } else {
      return "on"
    }
  } elseif {$opt == "help"} {
    echo "tsitc: set or display if timestamp messages are generated for itc messages"
    echo {Usage: tsitc [on | off | help]}
    echo "  on:   Timestamp are generated for itc messages"
    echo "  off:  Timestamp are not generated for itc messages"
    echo "  help: Display this message"
    echo ""
    echo "tsitc with no arguments will display whether or not timestamps are generated for itc messages (on or off)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set itc timestamp mode while trace is enabled"
  } elseif {$opt == "on"} {
    set tsitc_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set tsitc_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: tsitc [on | off | help]}
  }
}

proc tsowner {{opt ""}} {
  global tsowner_flag
  if {$opt == ""} {
    if {$tsowner_flag == 0} {
      return "off"
    } else {
      return "on"
    }
  } elseif {$opt == "help"} {
    echo "tsowner: set or display if timestamp messages are generated for ownership"
    echo "messages"
    echo {Usage: tsowner [on | off | help]}
    echo "  on:   Timestamp are generated for ownership messages"
    echo "  off:  Timestamp are not generated for ownership messages"
    echo "  help: Display this message"
    echo ""
    echo "tsowner with no arguments will display whether or not timestamps are generated"
    echo "for ownership messages (on or off)"
    echo ""
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set ownership timestamp mode while trace is enabled"
  } elseif {$opt == "on"} {
    set tsowner_flag 1
    echo -n ""
  } elseif {$opt == "off"} {
    set tsowner_flag 0
    echo -n ""
  } else {
    echo {Error: Usage: tsowner [on | off | help]}
  }
}

proc stoponwrap {{opt ""}} {
  global sow_flag
  if {$opt == ""} {
    if {$sow_flag == 0} {
      return "off"
    } else {
      return "on"
    }
  } elseif {$opt == "help"} {
    echo "stoponwrap: set or display trace buffer wrap mode"
    echo {Usage: stoponwrap [on | off | help]}
    echo "  on:   Enable stop trace collection when buffer is full (default)"
    echo "  off:  Continue tracing when the buffer fills, causing it to wrap"
    echo "  help: Display this message"
    echo ""
    echo "stoponwrap with no arguments will display the current status of trace buffer"
    echo "wrap (on or off)"
    echo ""
  } elseif {[eval ite] != 0} {
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
  if {$opt == ""} {
    switch $tm_flag {
      0 {
	return "none"
      }
      1 {
	return "sync"
      }
      3 {
	return "all"
      }
      default {
	return "invalid"
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
    echo "of messages to generate (none, sync, or all)"
    echo ""
  } elseif {[eval ite] != 0} {
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
  global itc_trigmask
  global te_control foo
  if {$opt == ""} {
    switch $itc_mode {
      0 {
	return "off"
      }
      1 {
	return "all"
      }
      2 {
	return "ownership"
      }
      3 {
	return "all+ownership"
      }
      default {
        return "invalid"
      }
    }
  } elseif {$opt == "help"} {
    echo "itc: set or display itc settings"
    echo {Usage: itc [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
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
    echo "                 number. Does not effect the ITC mode (off, ownership, all, all+ownership)."
    echo "                 itc mask without nn displays the current value of themask"
    echo "  trigmask nn:   Set the trigger enable mask to nn, where nn is a 32 bit number. Note"
    echo "                 nn should be prefixed with 0x if it is a hex number, or just 0 if"
    echo "                 it is an octal number; othwise it will be interpreted as a decimal"
    echo "                 number. Does not effect the ITC mode (off, ownership, all, all+ownership)."
    echo "                 itc trigmask without nn displays the current value of the trigger enable mask"
    echo "  help:          Display this message"
    echo ""
    echo "itc with no arguments will display the current itc settings"
    echo ""
  } elseif {$opt == "mask" && $mask == "" } {
      return [format "0x%08x" $itc_mask]
  } elseif {$opt == "trigmask" && $mask == ""} {
      return [format "0x%08x" $itc_trigmask]
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set trace mode while trace is enabled"
  } elseif {$opt == "off"} {
    set itc_mode 0
    echo -n ""
  } elseif {$opt == "all"} {
    set itc_mode 1
    echo -n ""
  } elseif {$opt == "ownership"} {
    set itc_mode 2
    echo -n ""
  } elseif {$opt == "all+ownership"} {
    set itc_mode 3
    echo -n ""
  } elseif {$opt == "mask"} {
    if {$mask == ""} {
      echo [format "ITC mask: 0x%08x" $itc_mask]
    } else {
      set itc_mask [expr $mask]
      echo -n ""
    }
  } elseif {$opt == "trigmask"} {
    if {$mask == ""} {
       # This case was covered above!
    } else {
      set itc_trigmask [expr $mask]
      echo -n ""
    }
  } else {
    echo {Error: Usage: itc [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
  }
}

proc maxicnt {{opt ""}} {
  global max_icnt_val
  if {$opt == ""} {
      return "[expr {$max_icnt_val + 5}]"
  } elseif {$opt == "help"} {
    echo "maxicnt: set or dipspaly the maximum i-cnt field"
    echo {Usage: maxicnt [nn | help]}
    echo "  nn: Set max i-cnt value to 2^(nn+5). nn must be between 5 and 16 for"
    echo "      a range between 32 and 65536"
    echo "  help: Display this message"
    echo ""
    echo "maxicnt with no arguments will display the current maximum i-cnt value"
    echo ""
  } elseif {[eval ite] != 0} {
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
  if {$opt == ""} {
    return "[expr {$max_btm_val + 5}]"
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
  } elseif {[eval ite] != 0} {
    echo "Error: Cannot set the maxbtm value while trace is enabled"
  } elseif {$opt >= 5 && $opt <= 16} {
    set max_btm_val [expr {$opt - 5}]
    echo -n ""
  } else {
    echo {Error: Usage: maxbtm [5 - 16 | help]}
  }
}

proc setreadptr {ptr} {
    global traceFunnelAddress
    global traceBaseAddresses
    global te_sinkrp_offset

    if {$traceFunnelAddress != 0} {
	mww [expr $traceFunnelAddress + $te_sinkrp_offset] $ptr
    } else {
        mww [expr [lindex $traceBaseAddresses 0] + $te_sinkrp_offset] $ptr
    }
}

proc readtracedata {} {
    global traceFunnelAddress
    global traceBaseAddresses
    global te_sinkdata_offset

    if {$traceFunnelAddress != 0} {
	return [word [expr $traceFunnelAddress + $te_sinkdata_offset]]
    } else {
        return [word [expr [lindex $traceBaseAddresses 0] + $te_sinkrp_offset]]
    }
}

proc wtb {{file "trace.rtd"}} {
  riscv set_prefer_sba on
  global tracebuffersize
  global te_sinkrp_offset
  global te_sinkwp_offset
  global te_sinkdata_offset

  set fp [open "$file" wb]

  set tracewp [eval gettracewp]

  if {($tracewp & 1) == 0 } { ;# buffer has not wrapped
    set tracebegin 0
    set traceend $tracewp
    echo "Trace from $tracebegin to $traceend, nowrap, [expr $traceend - $tracebegin] bytes"

    setreadptr 0
    for {set i 0} {$i < $traceend} {incr i 4} {
      pack w [eval readtracedata] -intle 32
      puts -nonewline $fp $w
    }
  } else {
    echo "Trace wrapped"
    set tracebegin [expr $tracewp & 0xfffffffe]
    set traceend $tracebuffersize
    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"

    setreadptr $tracebegin

    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      pack w [eval readtracedata] -intle 32
      puts -nonewline $fp $w
    }

    set tracebegin 0
    set traceend [expr $tracewp & 0xfffffffe]

    echo "Trace from $tracebegin to $traceend, [expr $traceend - $tracebegin] bytes"

    setreadptr 0

    for {set i $tracebegin} {$i < $traceend} {incr i 4} {
      pack w [eval readtracedata] -intle 32
      puts -nonewline $fp $w
    }
  }
  close $fp
  riscv set_prefer_sba off
}

proc cleartrace {} {
    global tracebuffersize
    global traceFunnelAddress
    global te_sinkrp_offset
    global te_sinkwp_offset
    global te_sinkdata_offset

    if {$traceFunnelAddress != 0} {
        setAllTfControls $te_sinkrp_offset 0
        for {set i 0} {$i<$tracebuffersize} {incr i 4} {
	    setAllTfControls $te_sinkdata_offset 0
        }
	setAllTfControls $te_sinkwp_offset 0
    } else {
        setAllTeControls $te_sinkrp_offset 0
        for {set i 0} {$i<$tracebuffersize} {incr i 4} {
	    setAllTeControls $te_sinkdata_offset 0
        }
	setAllTeControls $te_sinkwp_offset 0
    }
}  

proc resettrace {} {
    global te_control_offset

    setAllTeControls $te_control_offset 0	;# reset the TE
    setAllTfControls $te_sinkrp_offset  0	;# reset the TE

    setAllTeControls $te_control_offset 0x01830001	;# reset the TE
    setAllTfControls $te_sinkrp_offset  0x00000001	;# reset the TE
}

proc readts {} {
    global traceBaseAddresses
    global ts_control_offset

    echo "[format {0x%08x} [word [expr [lindex $traceBaseAddresses 0] + $ts_control_offset]]]"
}

proc gettracewp {} {
    global traceBaseAddresses
    global traceFunnelAddress
    global te_sinkwp_offset

    if {$traceFunnelAddress != 0} {
        set tracewp [word [expr $traceFunnelAddress + $te_sinkwp_offset]]
    } else {
        set tracewp [word [expr [lindex $traceBaseAddresses 0] + $te_sinkwp_offset]]
    }

    return $tracewp
}

proc trace {{opt ""}} {
    global ts_control_offset
    global te_control_offset
    global te_sinkwp_offset
    global tm_flag
    global tsenable_flag
    global tsdebug_flag
    global tstype_flag
    global tsprescale_flag
    global tsbranch_flag
    global tsitc_flag
    global tsowner_flag
    global sow_flag
    global itc_mode
    global itc_mask
    global max_icnt_val
    global max_btm_val
    global itc_traceenable_offset
    global tracebuffersize

    if {$opt == ""} {
        set tracectl ite
        if {[eval ite] == 0} {
            return "off"
        } else {
            return "on"
        }
    } elseif {$opt == "help"} {
        echo "trace: set or display the maximum number of BTMs between Sync messages"
        echo {Usage: trace [on | off | help]}
        echo "  on:       Enable tracing"
        echo "  off:      Disable tracing"
	echo "  settings: Display current trace related settings"
        echo "  help:     Display this message"
        echo ""
        echo "trace with no arguments will display if tracing is currently enabled (on or off)"
        echo ""
    } elseif {$opt == "on"} {
        setAllTeControls $te_control_offset 0x01000001   ;# disable trace
        setAllTeControls $te_sinkwp_offset 0             ;# clear WP

        setAllTfControls $te_control_offset 0x00000001   ;# disable trace
        setAllTfControls $te_sinkwp_offset 0             ;# clear WP

        set te_control_val 0
        set ts_control_val 0
        set itc_val 0

        if {($tsenable_flag != 0)} {
            set ts_control_val 0x00008001

            set ts_control_val [expr $ts_control_val | ($tsdebug_flag << 3)]

            set ts_control_val [expr $ts_control_val | ($tstype_flag << 4)]

            if {($tstype_flag == 2)} {
                set ts_control_val [expr $ts_control_val | 0x00000006]
            }

            set ts_control_val [expr $ts_control_val | ($tsprescale_flag << 8)]

            set ts_control_val [expr $ts_control_val | ($tsbranch_flag << 16)]

            set ts_control_val [expr $ts_control_val | ($tsitc_flag << 18)]

            set ts_control_val [expr $ts_control_val | ($tsowner_flag << 19)]
        }

        set te_control_val [expr $te_control_val | ($tm_flag << 4)]

        set te_control_val [expr $te_control_val | ($itc_mode << 7)]

        set te_control_val [expr $te_control_val | ($sow_flag << 14)]

        set te_control_val [expr $te_control_val | ($max_btm_val << 16)]

        set te_control_val [expr $te_control_val | ($max_icnt_val << 20)]

        set te_control_val [expr $te_control_val | (1 << 24) | 0x07]

	setAllTeControls $ts_control_offset $ts_control_val
	setAllTeControls $itc_traceenable_offset $itc_mask
	setAllTeControls $te_control_offset $te_control_val
	setAllTfControls $te_control_offset [expr $te_control_val & 0x00000007]

#       echo "te_control: [format "0x%08x" $te_control_val]"
#       echo "itc_mask: [format "0x%08x" $itc_mask]"
#       echo "itc_mode: [format "0x%08x" [expr $itc_mode << 7]]"

    } elseif {$opt == "off"} {
        setAllTeControls $te_control_offset 0x01000003   ;# stop trace
        setAllTfControls $te_control_offset 0x00000003   ;# stop trace

        setAllTeControls $te_control_offset 0x01000001   ;# disable and flush trace
        setAllTfControls $te_control_offset 0x00000001   ;# disable and flush trace

	set tracewp [eval gettracewp]

        if {$tracewp & 1} {
            echo "[expr $tracebuffersize / 4] trace words collected"
        } else {
            echo "[expr $tracewp / 4] trace words collected"
        }
    } elseif {$opt == "settings"} {
        if {[expr [word $te_control] & 0x02] != 0} {
	    echo "Tracing on"
	} else {
	    echo "Tracing off"
	}
        echo "tm_flag: $tm_flag"
        echo "itc_mode: $itc_mode"
        echo "sow_flag: $sow_flag"
        echo "max_btm_val: $max_btm_val"
        echo "max_icnt_val: $max_icnt_val"
        echo [format "itc_traceenable: 0x%08x" $itc_mask]
    } else {
        echo {Error: Usage: trace [on | off | settings | help]}
    }
}

proc wordscollected {} {
    global tracebuffersize
    global te_sinkwp

    set tracewp [eval gettracewp]

    if {$tracewp & 1} {
	      return "[expr $tracebuffersize / 4] trace words collected"
    } else {
        return "[expr $tracewp / 4] trace words collected"
    }
}

proc is_itc_implmented {} {
    # Caller is responsible for enabling trace before calling this
    # proc, otherwise behavior is undefined

    global itc_traceenable_offset
    global traceBaseAddresses

    # We'll write a non-zero value to itc_traceenable, verify a
    # non-zero readback, and restore the original value
 
    set originalval [word [expr [lindex $traceBaseAddresses 0] + $itc_traceenable_offset]]
    mww [expr [lindex $traceBaseAddresses 0] + $itc_traceenable_offset] 0xFFFFFFFF
    set readback [word [expr [lindex $traceBaseAddresses 0] + $itc_traceenable_offset]]
    set result [expr $readback != 0]
    mww [expr [lindex $traceBaseAddresses 0] + $itc_traceenable_offset] $originalval

    return $result
}

proc get_num_external_trigger_outputs {} {
    global xto_control_offset
    global traceBaseAddresses

    # We'll write non-zero nibbles to xto_control, count
    # non-zero nibbles on readback,
    # restore the original xto_control value.  0x1 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger output exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set originalval [word [expr [lindex $traceBaseAddresses 0] + $xto_control_offset]]
    mww [expr [lindex $traceBaseAddresses 0] + $xto_control_offset] 0x11111111
    set readback [word [expr [lindex $traceBaseAddresses 0] + $xto_control_offset]]
    mww [expr [lindex $traceBaseAddresses 0] + $xto_control_offset] $originalval

    set result 0
    for {set i 0} {$i < 8} {incr i} {
       if {($readback & 0xF) == 1} {
	   incr result
       }
       set readback [expr $readback >> 4]
    }
    return $result
}

proc get_num_external_trigger_inputs {} {
    global xti_control_offset
    global traceBaseAddresses

    # We'll write non-zero nibbles to xti_control, count
    # non-zero nibbles on readback,
    # restore the original xti_control value.  2 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger input exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set originalval [word [expr [lindex $traceBaseAddresses 0] + $xti_control_offset]]
    mww [expr [lindex $traceBaseAddresses 0] + $xti_control_offset] 0x22222222
    set readback [word [expr [lindex $traceBaseAddresses 0] + $xti_control_offset]]
    mww [expr [lindex $traceBaseAddresses 0] + $xti_control_offset] $originalval

    set result 0
    for {set i 0} {$i < 8} {incr i} {
       if {($readback & 0xF) == 0x2} {
	   incr result
       }
       set readback [expr $readback >> 4]
    }
    return $result
}

# Surprisingly, Jim Tcl lacks a primitive that returns the value of a
# register.  It only exposes a "cooked" line of output suitable for
# display.  But we can use that to extrace the actual register value
# and return it

proc regval {name} {
    set displayval [ocd_reg $name]
    set splitval [split $displayval ':']
    set val [lindex $splitval 1]
    return [string trim $val]
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

proc wp_control_set {bit} {
    global wp_control_offset
    global traceBaseAddresses

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] | (1 << $bit))]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_clear {bit} {
    global wp_control_offset
    global traceBaseAddresses

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] & ~(1 << $bit)]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_get_bit {bit} {
    global wp_control_offset
    global traceBaseAddresses

    set baseAddress [lindex $traceBaseAddresses 0]
    return [expr ([word [expr $baseAddress + $wp_control_offset]] >> $bit) & 0x01]
}

proc xti_action_read {idx} {
    global xti_control_offset
    global traceBaseAddresses

    set baseAddress [lindex $traceBaseAddresses 0]
    return [expr ([word [expr $baseAddress + $xti_control_offset]] >> ($idx*4)) & 0xF]
}

# xti_action_write only writes first core control regs. Needs work

proc xti_action_write {idx} {
    global xto_control_offset
    global traceBaseAddresses

    set baseAddress [lindex $traceBaseAddresses 0]
    set zeroed [expr ([word [expr $baseAddress + $xti_control_offset]] & ~(0xF << ($idx*4)))]
    set ored [expr ($zeroed | (($val & 0xF) << ($idx*4)))]
    mww [expr $baseAddress + $xti_control_offset] $ored
}

proc init {} {
    global te_control_offset
    global te_sinkrp_offset
    global te_sinkwp_offset
    global traceBaseAddresses
    global traceFunnelAddress
    global tracebuffersize

    setAllTeControls $te_control_offset 0x01830001
    setAllTfControls $te_control_offset 0x00000001

    setAllTeControls $te_sinkrp_offset 0xffffffff
    setAllTfControls $te_sinkrp_offset 0xffffffff

    setAllTeControls $te_sinkwp_offset 0x00000000
    setAllTfControls $te_sinkwp_offset 0x00000000

    if {$traceFunnelAddress != 0x00000000} {
      set tracebuffersize [expr [word [expr $traceFunnelAddress + $te_sinkrp_offset]] + 4]
    } else {
      set tracebuffersize [expr [word [expr [lindex $traceBaseAddresses 0] + $te_sinkrp_offset]] + 4]
    }

    echo -n ""
}

init

echo $tracebuffersize

set tm_flag 3
set tsenable_flag 0
set tsdebug_flag 0
set tstype_flag 2
set tsprescale_flag 0
set tsbranch_flag 3
set tsitc_flag 1
set tsowner_flag 1
set sow_flag 1
set itc_mode 0
set itc_mask 0
set itc_trigmask 0
set max_icnt_val 8
set max_btm_val 3

echo -n ""

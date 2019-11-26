#
# Scripts for trace using OpenOCD
#

# riscv set_prefer_sba on

# set traceBaseAddresses {0x20007000 0x20008000}
# set traceFunnelAddress 0x20009000
# set traceBaseAddresses 0x20007000
# set traceFunnelAddress 0x00000000

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

set num_cores  0
set has_funnel 0

# local helper functions not intented to be called directly

proc wordhex {addr} {
    mem2array x 32 $addr 1
    return [format "0x%08x" [lindex $x 1]]
}

proc word {addr} {
    mem2array x 32 $addr 1
    return [lindex $x 1]
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

# Returns list of all cores and funnel if present

proc getAllCoreFunnelList {} {
  global traceBaseAddresses
  global traceFunnelAddress

  set cores {}
  set index 0

  foreach controlReg $traceBaseAddresses {
    lappend cores $index
    set index [expr $index + 1]
  }

  if {$traceFunnelAddress != "0x00000000" && $traceFunnelAddress != ""} {
    lappend cores "funnel"
  }

  return $cores
}

# Returns list of all cores (but not funnel)

proc getAllCoreList {} {
  global traceBaseAddresses
  global traceFunnelAddress

  set cores {}
  set index 0

  foreach controlReg $traceBaseAddresses {
    lappend cores $index
    set index [expr $index + 1]
  }

  return $cores
}

# returns a list struct from parsing $cores with each element a core id

proc parseCoreFunnelList {cores} {
  global num_cores

  # parse core and build list of cores

  if {$cores == "all" || $cores == ""} {
    return [getAllCoreFunnelList]
  }

  set t [split $cores ","]

  foreach core $t {
    if {$core == "funnel"} {
      # only accept "funnel" if one is present
	    #
      if {has_funnel == 0} {
        return "error"
      }
    } elseif {$core < 0 || $core > $num_cores} {
      return "error"
    }
  }

  # t is now a list of cores

  return $t
}

proc parseCoreList {cores} {
  global num_cores

  # parse core and build list of cores

  if {$cores == "all" || $cores == ""} {
    return [getAllCoreList]
  }

  set t [split $cores ","]

  foreach core $t {
    if {($core < 0 || $core >= $num_cores)} {
	    return "error"
    }
  }

  # t is now a list of cores

  return $t
}

proc cores {} {
  return [parseCoreFunnelList "all"]
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

proc getTraceEnable {core} {
  global traceBaseAddrArray
  global te_control_offset

  set tracectl [word [expr $traceBaseAddrArray($core) + $te_control_offset]]

  if {($tracectl & 0x6) != 0} {
    return "on"
  }

  return "off"
}

proc enableTrace {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t | 0x00000007]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc disableTrace {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0x00000006]
  set t [expr $t | 0x00000001]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc resetTrace {core} {
  global traceBaseAddrArray
  global te_control_offset

  mww [expr $traceBaseAddrArray($core) + $te_control_offset] 0
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] 1
}

proc isTsEnabled {core} {
  global traceBaseAddrArray
  global ts_control_offset

  if {$core != "funnel"} {
    set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
    if {[expr $tsctl & 0x00008001] != 0} {
      return "on"
    }

    return "off"
  }
}

proc enableTs {core} {
  global traceBaseAddrArray
  global ts_control_offset

  if {$core != "funnel"} {
    set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
    set tsctl [expr $tsctl | 0x00008001]
    mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
  }
}

proc disableTs {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set tsctl [expr $tsctl & ~0x00008001]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
}

proc resetTs {core} {
  global traceBaseAddrArray
  global ts_control_offset

  if {$core != "funnel"} {
    set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
    set tsctl [expr $tsctl | 0x00008004]
    mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
    set t [expr $tsctl | ~0x00008004]
    mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
  }
}

proc getTsDebug {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  if {[expr $tsctl & 0x00000008] != 0} {
    return "on"
  }

  return "off"
}

proc enableTsDebug {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set tsctl [expr $tsctl | 0x0000008]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
}

proc disableTsDebug {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set tsctl [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set tsctl [expr $tsctl & ~0x0000008]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $tsctl
}

proc getTsClockSrc {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr ($t >> 4) & 0x7]
  switch $t {
  0       { return "none"     }
  1       { return "external" }
  2       { return "bus"      }
  3       { return "core"     }
  4       { return "slave"    }
  default { return "reserved" }
  }
}

# Note: ts clock src is read only and cannot be set

#proc setTsClockSrc {core clock} {
#  global traceBaseAddrArray
#  global ts_control_offset
#
#  switch $clock {
#  "none"     { set src 0 }
#  "external" { set src 1 }
#  "bus"      { set src 2 }
#  "core"     { set src 3 }
#  "slave"    { set src 4 }
#  default    { set src 0 }
#  }
#
#  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
#  set t [expr $t & ~0x0070]
#  set t [expr $t | ($src << 4)]
#  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $t
#}

proc getTsPrescale {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr ($t >> 8) & 0x3]
  switch $t {
  0 { return 1  }
  1 { return 4  }
  2 { return 16 }
  3 { return 64 }
  }
}

proc setTsPrescale {core prescl} {
  global traceBaseAddrArray
  global ts_control_offset

  switch $prescl {
  1       { set ps 0 }
  4       { set ps 1 }
  16      { set ps 2 }
  64      { set ps 3 }
  default { set ps 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr $t & ~0x0300]
  set t [expr $t | ($ps << 8)]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $t
}

proc getTsBranch {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr ($t >> 16) & 0x3]
  switch $t {
  0 { return "off"  }
  1 { return "indirect+exception"  }
  2 { return "reserved" }
  3 { return "all" }
  }
}

proc setTsBranch {core branch} {
  global traceBaseAddrArray
  global ts_control_offset

  switch $branch {
  "off"                { set br 0 }
  "indirect+exception" { set br 1 }
  "indirect"           { set br 1 }
  "all"                { set br 3 }
  default              { set br 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr $t & ~0x30000]
  set t [expr $t | ($br << 16)]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $t
}

proc setTsITC {core itc} {
  global traceBaseAddrArray
  global ts_control_offset

  switch $itc {
  "on"    { set f 1 }
  "off"   { set f 0 }
  default { set f 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr $t & ~0x40000]
  set t [expr $t | ($f << 18)]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $t
}

proc getTsITC {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr ($t >> 18) & 0x1]

  switch $t {
  0 { return "off"  }
  1 { return "on"  }
  }
}

proc setTsOwner {core owner} {
  global traceBaseAddrArray
  global ts_control_offset

  switch $owner {
  "on"    { set f 1 }
  "off"   { set f 0 }
  default { set f 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr $t & ~0x80000]
  set t [expr $t | ($f << 19)]
  mww [expr $traceBaseAddrArray($core) + $ts_control_offset] $t
}

proc getTsOwner {core} {
  global traceBaseAddrArray
  global ts_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $ts_control_offset]]
  set t [expr ($t >> 19) & 0x1]

  switch $t {
  0 { return "off"  }
  1 { return "on"  }
  }
}

proc setTeStopOnWrap {core wrap} {
  global traceBaseAddrArray
  global te_control_offset

  switch $wrap {
  "on"    { set sow 1 }
  "off"   { set sow 0 }
  default { set sow 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0x4000]
  set t [expr $t | ($sow << 14)]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc getTeStopOnWrap {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr ($t >> 14) & 0x1]

  switch $t {
  0 { return "off"  }
  1 { return "on"  }
  }
}

proc setTraceMode {core mode} {
  global traceBaseAddrArray
  global te_control_offset

  switch $mode {
  "none"  { set tm 0 }
  "sync"  { set tm 1 }
  "all"   { set tm 3 }
  default { set tm 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0x0070]
  set t [expr $t | ($tm << 4)]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc getTraceMode {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr ($t >> 4) & 0x7]

  switch $t {
  0       { return "none" }
  1       { return "sync" }
  3       { return "all"  }
  default { return "reserved" }
  }
}

proc setITC {core mode} {
  global traceBaseAddrArray
  global te_control_offset

  switch $mode {
  "off"           { set itc 0 }
  "all"           { set itc 1 }
  "ownership"     { set itc 2 }
  "all+ownership" { set itc 3 }
  default         { set itc 0 }
  }

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0x0180]
  set t [expr $t | ($itc << 7)]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc getITC {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr ($t >> 7) & 0x3]

  switch $t {
  0       { return "none" }
  1       { return "all"  }
  2       { return "ownership" }
  3       { return "all+ownership" }
  default { return "reserved" }
  }
}

proc setITCMask {core mask} {
  global traceBaseAddrArray
  global itc_traceenable_offset

  mww [expr $traceBaseAddrArray($core) + $itc_traceenable_offset] [expr $mask]
}

proc getITCMask {core} {
  global traceBaseAddrArray
  global itc_traceenable_offset

  set t [wordhex [expr $traceBaseAddrArray($core) + $itc_traceenable_offset]]

  return $t
}

proc setITCTriggerMask {core mask} {
  global traceBaseAddrArray
  global itc_trigenable_offset

  mww [expr $traceBaseAddrArray($core) + $itc_trigenable_offset] [expr $mask]
}

proc getITCTriggerMask {core} {
  global traceBaseAddrArray
  global itc_trigenable_offset

  set t [wordhex [expr $traceBaseAddrArray($core) + $itc_trigenable_offset]]

  return $t
}

proc setMaxIcnt {core maxicnt} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0xf00000]
  set t [expr $t | ($maxicnt << 20)]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc getMaxIcnt {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & 0xf00000]
  set t [expr $t >> 20]

  return $t
}

proc setMaxBTM {core maxicnt} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & ~0x0f0000]
  set t [expr $t | ($maxicnt << 16)]
  mww [expr $traceBaseAddrArray($core) + $te_control_offset] $t
}

proc getMaxBTM {core} {
  global traceBaseAddrArray
  global te_control_offset

  set t [word [expr $traceBaseAddrArray($core) + $te_control_offset]]
  set t [expr $t & 0x0f0000]
  set t [expr $t >> 16]

  return $t
}

# helper functions used during debugging ot script

proc srcbits {} {
  global te_impl_offset
  global traceBaseAddresses

  set numbits [expr [word [expr [lindex $traceBaseAddresses 0] + $te_impl_offset]] >> 24 & 7]
  return $numbits
}

# global functions intended to be called from command line or from freedom studio

proc ts {{cores "all"} {opt ""}} {
  global ts_control_offset

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: ts [corelist] [on | off | reset | help]]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tse "core $core: "

      lappend tse [isTsEnabled $core]

      if {$rv != ""} {
        append rv "; "
      }

      append rv $tse
    }

    return $rv
  } elseif {$opt == "help"} {
    echo "ts: set or display timestamp mode"
    echo {Usage: ts [corelist] [on | off | reset | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is"
    echo "            equivalent to all"
    echo "  on:       Enable timestamps in trace messages"
    echo "  off:      Disable timstamps in trace messages"
    echo "  reset:    Reset the internal timestamp to 0"
    echo "  help:     Display this message"
    echo ""
    echo "ts with no arguments will display the current status of timestamps (on or off)"
    echo ""
  } elseif {$opt == "on"} {
    # iterate through coreList and enable timestamps
    foreach core $coreList {
      enableTs $core
    }
    echo -n ""
  } elseif {$opt == "off"} {
    foreach core $coreList {
      disableTs $core
    }
    echo -n ""
  } elseif {$opt == "reset"} {
    foreach core $coreList {
      resetTs $core
    }
    echo "timestamp reset"
  } else {
    echo {Error: Usage: ts [corelist] [on | off | reset | help]]}
  }
}

proc tsdebug {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
       echo {Error: Usage: tsdebug [corelist] [on | off | reset | help]}
       return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsDebug $core]

      if {$rv != ""} {
        append rv "; "
      }

      append rv $tsd
    }

    return $rv
  }

  if {$opt == "help"} {
    echo "tsdebug: set or display if timestamp internal clock runs while in debug"
    echo {Usage: tsdebug [corelist] [on | off | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is"
    echo "            equivalent to all"
    echo "  on:       Timestamp clock continues to run while in debug"
    echo "  off:      Timnestamp clock halts while in debug"
    echo "  help:     Display this message"
    echo ""
    echo "tsdebug with no arguments will display the current status of timstamp debug"
    echo "(on or off)"
    echo ""
  } elseif {$opt == "on"} {
    # iterate through coreList and enable timestamps
    foreach core $coreList {
      enableTsDebug $core
    }
    echo -n ""
  } elseif {$opt == "off"} {
    foreach core $coreList {
      disableTsDebug $core
    }
    echo -n ""
  } else {
    echo {Error: Usage: tsdebug [corelist] [on | off | help]}
  }
}

proc tsclock {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
       echo {Error: Usage: tsclock [corelist] [help]}
       return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsClockSrc $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tsclock: display the source of the timestamp clock (none, external, bus, core, or slave)"
    echo {Usage: tsclock [corelist] [none | external | bus | core | slave | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all."
    echo "  none:     No source for the timestampe clock"
    echo "  internal: Set the source of the timestamp clock to internal"
    echo "  external: Set the srouce of the timestamp clock to external"
    echo "  help:     Display this message"
    echo ""
  } else {
    echo {Error: Usage: tsclock [corelist] [help]}
  }
}

proc tsprescale {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: tsprescale [corelist] [1 | 4 | 16 | 64 | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsPrescale $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tsprescale: set or display the timesampe clock prescalser (1, 4, 16, or 64)"
    echo {Usage: tsprescale [corelist] [1 | 4 | 16 | 64 | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "   1:       Set the prescaler to 1"
    echo "   4:       Set the prescaler to 4"
    echo "  16:       Set the prescaler to 16"
    echo "  64:       Set the prescaler to 64"
    echo "  help:     Display this message"
    echo ""
    echo "tspresacle with no arguments will display the current timestamp clock prescaler value (1, 4, 16, or 64)"
    echo ""
  } elseif {($opt == 1) || ($opt == 4) || ($opt == 16) || ($opt == 64)} {
    foreach core $coreList {
      setTsPrescale $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: tsprescale [corelist] [1 | 4 | 16 | 64 | help]}
  }
}

proc tsbranch {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: tsbranch [coreslist] [off | indirect | all | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsBranch $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tsbranch: set or display if timestamps are generated for branch messages"
    echo {Usage: tsbranch [corelist] [off | indirect | all | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  off:      Timestamps are not generated for branch messages"
    echo "  indirect: Generate timestamps for all indirect branch and exception messages"
    echo "  all:      Generate timestamps for all branch, exception, PTCM, and Error messages"
    echo "  help:     Display this message"
    echo ""
    echo "tsbranch with no arguments will display the current setting for tsbranch (off, indirect, all)"
    echo ""
  } elseif {($opt == "off") || ($opt == "indirect") || ($opt == "indirect+exception") || ($opt == "all")} {
    foreach core $coreList {
      setTsBranch $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: tsbranch [corelist] [off | indirect | all | help]}
  }
}

proc tsitc {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: tsitc [corelist] [on | off | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsITC $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tsitc: set or display if timestamp messages are generated for itc messages"
    echo {Usage: tsitc [corelist] [on | off | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  on:       Timestamp are generated for itc messages"
    echo "  off:      Timestamp are not generated for itc messages"
    echo "  help:     Display this message"
    echo ""
    echo "tsitc with no arguments will display whether or not timestamps are generated for itc messages (on or off)"
    echo ""
  } elseif {($opt == "on") || ($opt == "off")} {
    foreach core $coreList {
      setTsITC $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: tsitc [corelist] [on | off | help]}
  }
}

proc tsowner {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: tsowner [corelist] [on | off | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTsOwner $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tsowner: set or display if timestamp messages are generated for ownership messages"
    echo {Usage: tsowner [on | off | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  on:   Timestamp are generated for ownership messages"
    echo "  off:  Timestamp are not generated for ownership messages"
    echo "  help: Display this message"
    echo ""
    echo "tsowner with no arguments will display whether or not timestamps are generated"
    echo "for ownership messages (on or off)"
    echo ""
  } elseif {($opt == "on") || ($opt == "off")} {
    foreach core $coreList {
      setTsOwner $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: tsowner [corelist] [on | off | help]}
  }
}

proc stoponwrap {{cores "all"} {opt ""}} {
  set coreList [parseCoreFunnelList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreFunnelList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: stoponwrap [corelist] [on | off | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTeStopOnWrap $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "stoponwrap: set or display trace buffer wrap mode"
    echo {Usage: stoponwrap [corelist] [on | off | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  on:       Enable stop trace collection when buffer is full (default)"
    echo "  off:      Continue tracing when the buffer fills, causing it to wrap"
    echo "  help:     Display this message"
    echo ""
    echo "stoponwrap with no arguments will display the current status of trace buffer"
    echo "wrap (on or off)"
    echo ""
  } elseif {($opt == "on") || ($opt == "off")} {
    foreach core $coreList {
      setTeStopOnWrap $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: stoponwrap [corelist] [on | off | help]}
  }
}

proc tracemode {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: tracemode [corelist] [none | sync | all | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getTraceMode $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "tracemode: set or display trace type (sync, sync+btm)"
    echo {Usage: tracemode [corelist] [sync | all | none | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  sync:     Generate only sync trace messages"
    echo "  all:      Generate both sync and btm trace messages"
    echo "  none:     Do not generate sync or btm trace messages"
    echo "  help:     Display this message"
    echo ""
    echo "tracemode with no arguments will display the current setting for the type"
    echo "of messages to generate (none, sync, or all)"
    echo ""
  } elseif {($opt == "sync") || ($opt == "all") || ($opt == "none")} {
    foreach core $coreList {
      setTraceMode $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: tracemode [corelist] [sync | all | none | help]}
  }
}

proc itc {{cores "all"} {opt ""} {mask ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$mask == ""} {
      set mask $opt
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: itc [corelist] [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [getITC $core]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "itc: set or display itc settings"
    echo {Usage: itc [corelist] [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
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
    echo "                 itc mask without nn displays the current value of the mask"
    echo "  trigmask nn:   Set the trigger enable mask to nn, where nn is a 32 bit number. Note"
    echo "                 nn should be prefixed with 0x if it is a hex number, or just 0 if"
    echo "                 it is an octal number; othwise it will be interpreted as a decimal"
    echo "                 number. Does not effect the ITC mode (off, ownership, all, all+ownership)."
    echo "                 itc trigmask without nn displays the current value of the trigger enable mask"
    echo "  help:          Display this message"
    echo ""
    echo "itc with no arguments will display the current itc settings"
    echo ""
  } elseif {$opt == "mask"} {
    if {$mask == "" } {
      foreach core $coreList {
        set rv ""

        foreach core $coreList {
          set tsd "core $core: "

          lappend tsd [getITCMask $core]

          if {$rv != ""} {
            append rv "; "
          }

          append rv $tsd
        }

        return $rv
      }
    } else {
      foreach core $coreList {
        setITCMask $core $mask
      }
    }
  } elseif {$opt == "trigmask"} {
    if {$mask == ""} {
      foreach core $coreList {
        set rv ""

        foreach core $coreList {
          set tsd "core $core: "

          lappend tsd [getITCTriggerMask $core]

          if {$rv != ""} {
            append rv "; "
          }

          append rv $tsd
        }

        return $rv
      }
    } else {
      foreach core $coreList {
        setITCTriggerMask $core $mask
      }
    }
  } elseif {($opt == "off") || ($opt == "all") || ($opt == "ownership") || ($opt == "all+ownership")} {
    foreach core $coreList {
      setITC $core $opt
    }
    echo -n ""
  } else {
    echo {Error: Usage: itc [corelist] [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
  }
}

proc maxicnt {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: maxicnt [corelist] [5 - 10 | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [expr [getMaxIcnt $core] + 5]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "maxicnt: set or dipspaly the maximum i-cnt field"
    echo {Usage: maxicnt [corelist] [nn | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  nn:       Set max i-cnt value to 2^(nn+5). nn must be between 5 and 10 for"
    echo "            a range between 32 and 1024"
    echo "  help:     Display this message"
    echo ""
    echo "maxicnt with no arguments will display the current maximum i-cnt value"
    echo ""
  } elseif {$opt >= 5 && $opt <= 10} {
    foreach core $coreList {
      setMaxIcnt $core [expr $opt - 5]
    }
    echo -n ""
  } else {
    echo {Error: Usage: maxicnt [corelist] [5 - 10 | help]}
  }
}

proc maxbtm {{cores "all"} {opt ""}} {
  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: maxbtm [corelist] [5 - 16 | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    set rv ""

    foreach core $coreList {
      set tsd "core $core: "

      lappend tsd [expr [getMaxBTM $core] + 5]

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }
    return $rv
  }

  if {$opt == "help"} {
    echo "maxbtm: set or display the maximum number of BTMs between Sync messages"
    echo {Usage: maxbtm [corelist] [nn | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  nn:       Set the maximum number of BTMs between Syncs to nn. nn must be between"
    echo "            5 and 16 for a range between 32 and 65536"
    echo "  help:     Display this message"
    echo ""
    echo "maxbtm with no arguments will display the current maximum number of BTMs"
    echo "between sync messages"
    echo ""
  } elseif {$opt >= 5 && $opt <= 16} {
    foreach core $coreList {
      setMaxBTM $core [expr $opt - 5]
    }
    echo -n ""
  } else {
    echo {Error: Usage: maxbtm [corelist] [5 - 16 | help]}
  }
}

proc setreadptr {ptr} {
    global traceFunnelAddress
    global traceBaseAddresses
    global te_sinkrp_offset
    global has_funnel

    if {$has_funnel != 0} {
	mww [expr $traceFunnelAddress + $te_sinkrp_offset] $ptr
    } else {
        mww [expr [lindex $traceBaseAddresses 0] + $te_sinkrp_offset] $ptr
    }
}

proc readtracedata {} {
    global traceFunnelAddress
    global traceBaseAddresses
    global te_sinkdata_offset
    global te_sinkrp_offset
    global has_funnel

    if {$has_funnel != 0} {
	return [word [expr $traceFunnelAddress + $te_sinkdata_offset]]
    } else {
        return [word [expr [lindex $traceBaseAddresses 0] + $te_sinkdata_offset]]
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

proc clearTraceBuffer {core} {
    global traceBaseAddrArray
    global te_sinkrp_offset
    global te_sinkwp_offset

    mww [expr $traceBaseAddrArray($core) + $te_sinkrp_offset] 0
    mww [expr $traceBaseAddrArray($core) + $te_sinkwp_offset] 0
}

proc cleartrace {{cores "all"}} {
  set coreList [parseCoreFunnelList $cores]

  if {$coreList == "error"} {
      echo {Error: Usage: cleartrace [corelist]}
      return "error"
  }

  foreach core $coreList {
      clearTraceBuffer $core
  }
}

proc readts {} {
    global traceBaseAddrArray
    global ts_control_offset

    echo "[wordhex [expr $traceBaseAddrArray(0) + $ts_control_offset]]"
}

proc gettracewp {} {
    global traceBaseAddresses
    global traceFunnelAddress
    global te_sinkwp_offset
    global has_funnel

    if {$has_funnel != 0} {
        set tracewp [word [expr $traceFunnelAddress + $te_sinkwp_offset]]
    } else {
        set tracewp [word [expr [lindex $traceBaseAddresses 0] + $te_sinkwp_offset]]
    }

    return $tracewp
}

proc trace {{cores "all"} {opt ""}} {
  set coreList [parseCoreFunnelList $cores]

  if {$coreList == "error"} {
    if {$opt == ""} {
      set opt $cores
      set cores "all"

      set coreList [parseCoreFunnelList $cores]
    }

    if {$coreList == "error"} {
      echo {Error: Usage: trace [corelist] [on | off | reset | settings | help]}
      return "error"
    }
  }

  if {$opt == ""} {
    # display current status of ts enable
    set rv ""

    foreach core $coreList {
      if {$core == "funnel"} {
        set te "$core: "
      } else {
        set te "core $core: "
      }

      lappend te [getTraceEnable $core]

      if {$rv != ""} {
  	append rv "; "
      }

      append rv $te
    }

    return $rv
  }

  if {$opt == "help"} {
    echo "trace: set or display the maximum number of BTMs between Sync messages"
    echo {Usage: trace [corelist] [on | off | reset | settings | help]}
    echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
    echo "  on:       Enable tracing"
    echo "  off:      Disable tracing"
    echo "  reset:    Reset trace encoder"
    echo "  settings: Display current trace related settings"
    echo "  help:     Display this message"
    echo ""
    echo "trace with no arguments will display if tracing is currently enabled for all cores (on or off)"
    echo ""
  } elseif {$opt == "on"} {
    foreach core $coreList {
      enableTrace $core
    }
  } elseif {$opt == "off"} {
    foreach core $coreList {
      disableTrace $core
    }
  } elseif {$opt == "reset"} {
    foreach core $coreList {
      resetTrace $core
    }
  } elseif {$opt == "settings"} {
    # build a cores option without funnel

    set cores2 ""

    foreach core $coreList {
      if {$core != "funnel"} {
        if {$cores2 != ""} {
          append cores2 ","
        }
        append cores2 $core
      }
    }

    cores
    srcbits

    trace $cores
    stoponwrap $cores

    if {$cores2 != ""} {
      echo "ts: [ts $cores2]"
      echo "tsdebug: [tsdebug $cores2]"
      echo "tsclock: [tsclock $cores2]"
      echo "tsprescale: [tsprescale $cores2]"
      echo "tsbranch: [tsbranch $cores2]"
      echo "tsitc: [tsitc $cores2]"
      echo "tsowner: [tsowner $cores2]"
      echo "tracemode: [tracemode $cores2]"
      echo "itc: [itc $cores2]"
      echo "maxicnt: [maxicnt $cores2]"
      echo "maxbtm: [maxbtm $cores2]"
    }
  } else {
    echo {Error: Usage: trace [corelist] [on | off | reset | settings | help]}
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

proc is_itc_implmented {core} {
    # Caller is responsible for enabling trace before calling this
    # proc, otherwise behavior is undefined

    global itc_traceenable_offset
    global traceBaseAddrArray

    # We'll write a non-zero value to itc_traceenable, verify a
    # non-zero readback, and restore the original value
 
    set originalval [word [expr $traceBaseAddrArray($core) + $itc_traceenable_offset]]
    mww [expr $traceBaseAddrArray($core) + $itc_traceenable_offset] 0xFFFFFFFF
    set readback [word [expr $traceBaseAddrArray($core) + $itc_traceenable_offset]]
    set result [expr $readback != 0]
    mww [expr $traceBaseAddrArray($core) + $itc_traceenable_offset] $originalval

    return $result
}

proc get_num_external_trigger_outputs {core} {
    global xto_control_offset
    global traceBaseAddrArray

    # We'll write non-zero nibbles to xto_control, count
    # non-zero nibbles on readback,
    # restore the original xto_control value.  0x1 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger output exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set originalval [word [expr $traceBaseAddrArray($core) + $xto_control_offset]]
    mww [expr $traceBaseAddrArray($core) + $xto_control_offset] 0x11111111
    set readback [word [expr $traceBaseAddrArray($core) + $xto_control_offset]]
    mww [expr $traceBaseAddrArray($core) + $xto_control_offset] $originalval

    set result 0
    for {set i 0} {$i < 8} {incr i} {
       if {($readback & 0xF) == 1} {
	   incr result
       }
       set readback [expr $readback >> 4]
    }
    return $result
}

proc get_num_external_trigger_inputs {core} {
    global xti_control_offset
    global traceBaseAddrArray

    # We'll write non-zero nibbles to xti_control, count
    # non-zero nibbles on readback,
    # restore the original xti_control value.  2 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger input exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set originalval [word [expr $traceBaseAddrArray($core) + $xti_control_offset]]
    mww [expr $traceBaseAddrArray($core) + $xti_control_offset] 0x22222222
    set readback [word [expr $traceBaseAddrArray($core) + $xti_control_offset]]
    mww [expr $traceBaseAddrArray($core) + $xti_control_offset] $originalval

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

proc wp_control_set {core bit} {
    global wp_control_offset
    global traceBaseAddresses

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] | (1 << $bit)]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_clear {core bit} {
    global wp_control_offset
    global traceBaseAddresses

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] & ~(1 << $bit)]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_get_bit {core bit} {
    global wp_control_offset
    global traceBaseAddresses

    set baseAddress [lindex $traceBaseAddresses 0]
    return [expr ([word [expr $baseAddress + $wp_control_offset]] >> $bit) & 0x01]
}

proc wp_control {cores {bit ""} {val ""}} {
  if {$bit == ""} {
    set bit $cores
    set cores "all"
  }

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    echo {Error: Usage: wp_control corelist (wpnum [edge | range]) | help}
    return "error"
  }

  if {$bit == "help"} {
    echo "wp_control: set or display edge or range mode for slected watchpoint"
    echo {Usage: wp_control corelist (wpnum [edge | range]) | help}
    echo "  corelist: Comma separated list of core numbers, or 'all'."
    echo "  wpnum:    Watchpoint number (0-31)"
    echo "  edge:     Select edge mode"
    echo "  range:    Select range mode"
    echo "  help:     Display this message"
    echo ""
  } elseif {$val == ""} {
    set rv ""

    foreach core $coreList {
      set b [wp_control_get_bit $core $bit]
      if {$b != 0} {
        set wp "core $core: range"
      } else {
        set wp "core $core: edge"
      }

      if {$rv != ""} {
	append rv "; "
      }

      append rv $wp
    }

    return $rv
  } elseif {$val == "edge"} {
    foreach core $coreList {
      wp_control_clear $core $bit
    }
  } elseif {$val == "range"} {
    foreach core $coreList {
      wp_control_set $core $bit
    }
  } else {
    echo {Error: Usage: wp_control corelist (wpnum [edge | range]) | help}
  }
}

proc xti_action_read {core idx} {
    global xti_control_offset
    global traceBaseAddrArray

    return [expr ([word [expr $traceBaseAddrArray($core) + $xti_control_offset]] >> ($idx*4)) & 0xF]
}

proc xti_action_write {core idx val} {
  global xti_control_offset
  global traceBaseAddrArray

  set zeroed [expr ([word [expr $traceBaseAddrArray($core) + $xti_control_offset]] & ~(0xF << ($idx*4)))]
  set ored [expr ($zeroed | (($val & 0xF) << ($idx*4)))]
  mww [expr $traceBaseAddrArray($core) + $xti_control_offset] $ored
}

proc xto_event_read { core idx } {
    global xto_control_offset
    global traceBaseAddrArray
    return [expr ([word [expr $traceBaseAddrArray($core) + $xto_control_offset]] >> ($idx*4)) & 0xF]
}

proc xto_event_write { core idx val } {
    global xto_control_offset
    global traceBaseAddrArray
    set zeroed [expr ([word [expr $traceBaseAddrArray($core) + $xto_control_offset]] & ~(0xF << ($idx*4)))]
    set ored [expr ($zeroed | (($val & 0xF) << ($idx*4)))]
    mww [expr $traceBaseAddrArray($core) + $xto_control_offset] $ored
}


proc xti_action {cores {idx ""} {val ""}} {
  if {$idx == ""} {
    set idx $cores
    set cores "all"
  }

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
    echo {Error: Usage: xti_action corelist (xtireg [none | start | stop | record] | help)}
    return "error"
  }

  if {$idx == "help"} {
    echo "xti_action: set or display external trigger input action type (none, start, stop record)"
    echo {Usage: xti_action corelist (xtireg [none | start | stop | record] | help)}
    echo "  corelist: Comma separated list of core numbers, or 'all'."
    echo "  xtireg:   XTI action number (0 - 7)"
    echo "  none:     no action"
    echo "  start:    start tracing"
    echo "  stop:     stop tracing"
    echo "  record:   emot program trace sync message"
    echo "  help:     Display this message"
    echo ""
  } elseif {$val == ""} {
    # display current state of xti reg
    set rv ""

    foreach core $coreList {
      switch [xti_action_read $core $idx] {
      0       { set action "none"   }
      2       { set action "start"  }
      3       { set action "stop"   }
      4       { set action "record" }
      default { set action "reserved" }
      }

      set tsd "core $core: $action"

      if {$rv != ""} {
	append rv "; "
      }

      append rv $tsd
    }

    return $rv
  } else {
    # set current state of xti reg
    set rv ""

    foreach core $coreList {
      switch $val {
      "none"   { set action 0 }
      "start"  { set action 2 }
      "stop"   { set action 3 }
      "record" { set action 4 }
      default  { set action 0 }
      }

      xti_action_write $core $idx $action
    }
    echo -n ""
  }
}

proc init {} {
    global te_control_offset
    global te_sinkrp_offset
    global te_sinkwp_offset
    global traceBaseAddresses
    global traceFunnelAddress
    global tracebuffersize
    global traceBaseAddrArray
    global num_cores
    global has_funnel

    # put all cores and funnel in a known state

    setAllTeControls $te_control_offset 0x01830001
    setAllTfControls $te_control_offset 0x00000001

    setAllTeControls $te_sinkrp_offset 0xffffffff
    setAllTfControls $te_sinkrp_offset 0xffffffff

    setAllTeControls $te_sinkwp_offset 0x00000000
    setAllTfControls $te_sinkwp_offset 0x00000000

    set core 0

    foreach addr $traceBaseAddresses {
      set traceBaseAddrArray($core) $addr
      incr core
    }

    set num_cores $core

    if {($traceFunnelAddress != 0x00000000) && ($traceFunnelAddress != "")} {
      set has_funnel 1
      set traceBaseAddrArray(funnel) $traceFunnelAddress
      set tracebuffersize [expr [word [expr $traceFunnelAddress + $te_sinkrp_offset]] + 4]
    } else {
      set has_funnel 0
      set tracebuffersize [expr [word [expr [lindex $traceBaseAddresses 0] + $te_sinkrp_offset]] + 4]
    }

    echo -n ""
}

# following routines are used durring script debug, or just to find out the state of things

proc qts {{cores "all"}} {
  global ts_control_offset
  global traceBaseAddrArray

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
     echo "Error: Usage: qts [<cores>]"
     return "error"
  }

  # display current status of ts enable
  set rv ""

  foreach core $coreList {
    if {$core != "funnel"} {
      set tse "core $core: "

      lappend tse [wordhex [expr $traceBaseAddrArray($core) + $ts_control_offset]]

      if {$rv != ""} {
        append rv "; $tse"
      } else {
        set rv $tse
      }
    }
  }

  return $rv
}

proc qte {{cores "all"}} {
  global te_control_offset
  global traceBaseAddrArray

  set coreList [parseCoreFunnelList $cores]

  if {$coreList == "error"} {
     echo "Error: Usage: qte [<cores>]"
     return "error"
  }

  # display current status of ts enable
  set rv ""

  foreach core $coreList {
    set tse "core $core: "

    lappend tse [wordhex [expr $traceBaseAddrArray($core) + $te_control_offset]]

    if {$rv != ""} {
      append rv "; $tse"
    } else {
      set rv $tse
    }
  }

  return $rv
}

proc qitctraceenable {{cores "all"}} {
  global traceBaseAddrArray
  global itc_traceenable_offset

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
     echo "Error: Usage: qitctraceenable [corelist]"
     return "error"
  }

  # display current status of itctraceenable
  set rv ""

  foreach core $coreList {
    set tse "core $core: "

    lappend tse [wordhex [expr $traceBaseAddrArray($core) + $itc_traceenable_offset]]

    if {$rv != ""} {
      append rv "; $tse"
    } else {
      set rv $tse
    }
  }

  return $rv
}

proc qitctriggerenable {{cores "all"}} {
  global traceBaseAddrArray
  global itc_triggerenable_offset

  set coreList [parseCoreList $cores]

  if {$coreList == "error"} {
     echo "Error: Usage: qitctriggerenable [corelist]"
     return "error"
  }

  # display current status of itctriggerenable
  set rv ""

  foreach core $coreList {
    set tse "core $core: "

    lappend tse [wordhex [expr $traceBaseAddrArray($core) + $te_triggerenable_offset]]

    if {$rv != ""} {
      append rv "; $tse"
    } else {
      set rv $tse
    }
  }

  return $rv
}

init

echo $tracebuffersize

echo -n ""

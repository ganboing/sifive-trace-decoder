set traceModes { 0 1 2 3 4 5 6 7 }
set traceFormats { 0 1 2 3 4 5 6 7  }

proc traceModesStr {tm} {
    switch $tm {
       0       { return "none" }
       1       { return "sample/event" }
       3       { return "btm+sync"  }
       6       { return "htmc+sync" }
       7       { return "htm+sync"  }
       default { return "reserved" }
    }
}

proc traceModesParse {mode} {
    switch $mode {
       "none"       { return 0 }
       "sample"     { return 1 }
       "event"      { return 1 }
       "btm+sync"   { return 3 }
       "btm"        { return 3 }
       "htmc+sync"  { return 6 }
       "htmc"       { return 6 }
       "htm+sync"   { return 7 }
       "htm"        { return 7 }
       default      { return 0 }
    }
}

proc txIsEnabled {core} {
    global traceBaseAddrArray
    global te_control_offset

    set tracectl [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]

    if {($tracectl & 0x2) != 0} {
        return "ENABLED"
    }

    return "DISABLED"
}

proc txIsRunning {core} {
    global traceBaseAddrArray
    global te_control_offset

    set tracectl [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]

    if {($tracectl & 0x4) != 0} {
        return "RUNNING"
    }

    return "STOPPED"
}

proc txWaitFlush {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    while {($t & 0x8) == 0} {
        set t [word $reg]
    }
}

proc txDisable {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    # Disable tfEnable | tfTracing
    set t [expr {$t & ~0x6}]
    mww $reg $t
}

proc txEnable {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    # Enable tfEnable
    set t [expr {$t | 0x2}]
    mww $reg $t
}

proc txStop {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    # Disable tfTracing
    set t [expr {$t & ~0x4}]
    mww $reg $t
}

proc txStart {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    # Enable tfEnable | tfTracing
    set t [expr {$t | 0x6}]
    mww $reg $t
}

proc teGetSinkBase {core} {
    global traceBaseAddrArray
    global te_sinkbase_offset
    global te_sinkbasehigh_offset

    set tracebase [word [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}]]
    set tracebasehi [word [expr {$traceBaseAddrArray($core) + $te_sinkbasehigh_offset}]]

    set baseAddr [expr {$tracebase + ($tracebasehi << 32)}]
    
    return $baseAddr
}

proc teSetMaxIcnt {core maxicnt} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0xf00000}]
    set t [expr {$t | ($maxicnt << 20)}]
    mww $reg $t
}

proc teGetMaxIcnt {core} {
    global traceBaseAddrArray
    global te_control_offset

    set t [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
    set t [expr {$t & 0xf00000}]
    set t [expr {$t >> 20}]

    return $t
}

proc teFindMaxICntFrom { core maxidx }  {
#   echo "findMaxIcntFrom($core $maxidx)"

    # We used to always start from 15, and assume the first sticking value
    # was the end of a contiguous range of valid indexes, but that is no longer
    # always true with latest encoders that can support event trace, so this is
    # a new proc that allows a maxidx to be passed in.
    # E.g. code can call with maxidx of 15 to find out if event trace is supported,
    # then can call again with maxidx of 14 to find out the largest value in the
    # valid contiguous range of values

    # Save current value so we can restore it.  Otherwise this
    # proc is destructive to the value.
    set original [getEncoderMaxIcnt $core]
    
    # Start on $maxidx and work down until one sticks.
    for {set x $maxidx} { $x > 0 } {set x [expr {$x - 1}]} {
        setEncoderMaxIcnt $core $x
        set y [getEncoderMaxIcnt $core]
        if {$x == $y} {
            # restore original value before returning result
            setEncoderMaxIcnt $core $original
            return $x;
        }
    }
}

proc tefindMaxICnt { core }  {
    #   echo "findMaxIcnt($core)"

    # Backward compatible shim that assumes 15 as the maxidx
    return [teFindMaxICntFrom $core 15]
}

proc teSetMaxBTM {core maxbtm} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word ]
    set t [expr {$t & ~0x0f0000}]
    set t [expr {$t | ($maxicnt << 16)}]
    mww $reg $t
}

proc teGetMaxBTM {core} {
    global traceBaseAddrArray
    global te_control_offset

    set t [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
    set t [expr {$t & 0x0f0000}]
    set t [expr {$t >> 16}]

    return $t
}

proc teProbeModes {core} {
    global traceBaseAddrArray
    global traceModes
    global te_control_offset
    
    set supported 0

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set saved $t
    foreach tm $traceModes {
        set t [expr {$t & ~0x0070}]
        set t [expr {$t | ($tm << 4)}]
        mww $reg $t
        set tm [word $reg]
        set tm [expr {($tm >> 4) & 0x7}]
        set supported [expr {$supported | (1 << $tm)}]
    }
    mww $reg $saved
    
    set modes ""
    foreach tm $traceModes {
        if {($supported & (1 << $tm)) != 0} {
            append modes " " [traceModesStr $tm]
        }
    }
    echo "probeEncoderModes\[$core\]:$modes"
    return $supported
}

proc teProbeFormats { core } {
    global traceBaseAddrArray
    global traceFormats
    global te_control_offset

    set supported 0
    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set saved $t
    foreach fo $traceFormats {
        set t [expr {$t & ~0x7000000}]
        set t [expr {$t | ($fo << 24)}]
        mww $reg $t
        set fo [word $reg]
        set fo [expr {($fo >> 24) & 0x7}]
        set supported [expr {$supported | (1 << $fo)}]
    }
    mww $reg $saved

    set formats ""
    foreach fo $traceFormats {
        if {($supported & (1 << $fo)) != 0} {
            append formats " $fo"
        }
    }
    echo "teProbeFormats\[$core\]:$formats"
}

proc teGetMode {core} {
    global traceBaseAddrArray
    global te_control_offset
    global ev_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {($t >> 4) & 0x7}]

    return [traceModesStr $t]
}

proc teSetMode {core mode} {
    global traceBaseAddrArray
    global te_control_offset
    global has_event
    global have_htm

    set tm [traceModesParse $mode]
    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x70}]
    set t [expr {$t | ($tm << 4)}]
    mww $reg $t
}

proc teSetStopOnWrap {core wrap} {
    global traceBaseAddrArray
    global te_control_offset

    switch $wrap {
        "on"    { set sow 1 }
        "off"   { set sow 0 }
        default { set sow 0 }
    }

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x4000}]
    set t [expr {$t | ($sow << 14)}]
    mww $reg $t
}

proc teGetStopOnWrap {core} {
    global traceBaseAddrArray
    global te_control_offset

    set t [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
    set t [expr {($t >> 14) & 0x1}]
    switch $t {
        0 { return "off"  }
        1 { return "on"  }
    }
}

proc teGetNumExtTrigOut {core} {
    global xto_control_offset
    global traceBaseAddrArray

    # We'll write non-zero nibbles to xto_control, count
    # non-zero nibbles on readback,
    # restore the original xto_control value.  0x1 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger output exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set reg [expr {$traceBaseAddrArray($core) + $xto_control_offset}]
    set originalval [word $reg]
    mww $reg 0x11111111
    set readback [word $reg]
    echo [format "getEncoderNumExtTrigOut: core\[$core\] xto_ctrl=%x" $readback]
    mww $reg $originalval

    set result 0
    for {set i 0} {$i < 8} {incr i} {
        if {($readback & 0xF) == 1} {
            incr result
        }
        set readback [expr {$readback >> 4}]
    }
    return $result
}

proc teGetNumExtTrigIn {core} {
    global xti_control_offset
    global traceBaseAddrArray

    # We'll write non-zero nibbles to xti_control, count
    # non-zero nibbles on readback,
    # restore the original xti_control value.  2 seems a good
    # nibble to write, that bit is always legal if an external
    # trigger input exists in the nibble slot, regardless of whether other
    # optional trace features are present

    set reg [expr {$traceBaseAddrArray($core) + $xti_control_offset}]
    set originalval [word $reg]
    mww $reg 0x22222222
    set readback [word $reg]
    echo [format "get_num_external_trigger_inputs: core\[$core\] xti_ctrl=%x" $readback]
    mww $reg $originalval

    set result 0
    for {set i 0} {$i < 8} {incr i} {
        if {($readback & 0xF) == 0x2} {
            incr result
        }
        set readback [expr {$readback >> 4}]
    }
    return $result
}


proc teSetStallEnable {core enable} {
    global traceBaseAddrArray
    global te_control_offset

    switch $enable {
        "on"    { set en 1 }
        "off"   { set en 0 }
        default { set en 0 }
    }

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~(1 << 13)}]
    set t [expr {$t | ($en << 13)}]
    mww $reg $t
}

proc teGetStallEnable {core} {
    global traceBaseAddrArray
    global te_control_offset

#    echo "getTeStallEnable($core)"

    set t [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
    set t [expr {($t >> 13) & 0x1}]

    switch $t {
        0 { return "off"  }
        1 { return "on"  }
    }
}

# TODO

proc clearTraceBuffer {core} {
    global traceBaseAddrArray
    global te_sinkrp_offset
    global te_sinkwp_offset
    global te_sinkbase_offset

#    echo "clearTraceBuffer($core)"

    set s [getSink $core]
    switch [string toupper $s] {
        "SRAM" { 
            mww [expr $traceBaseAddrArray($core) + $te_sinkrp_offset] 0
            mww [expr $traceBaseAddrArray($core) + $te_sinkwp_offset] 0
        }
        "SBA" { 
            set t [word [expr $traceBaseAddrArray($core) + $te_sinkbase_offset]]
            mww [expr $traceBaseAddrArray($core) + $te_sinkwp_offset] $t
            mww [expr $traceBaseAddrArray($core) + $te_sinkrp_offset] $t
        }
    }
}

proc cleartrace {{cores "all"}} {
    set coreList [parseCoreFunnelList $cores]

#    echo "cleartrace($cores)"

    if {$coreList == "error"} {
        echo {Error: Usage: cleartrace [corelist]}
        return "error"
    }

    foreach core $coreList {
        clearTraceBuffer $core
    }
}

proc teGetTeVersion {core} {
    global traceBaseAddrArray
    global te_impl_offset

    set t [word [expr {$traceBaseAddrArray($core) + $te_impl_offset}]]
    set version [expr {$t & 0x7}]
    return $version
}



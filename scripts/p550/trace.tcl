#
# Scripts for trace using OpenOCD
#

source constants.tcl
source helpers.tcl
source encoder.tcl
source encoder_cli.tcl
source itc.tcl
source event.tcl
source event_cli.tcl
source timestamp.tcl
source timestamp_cli.tcl
source funnel.tcl
source combined.tcl
source combined_cli.tcl
source dump.tcl

set num_cores  0
set num_funnels 0

set traceBaseAddresses { 0x100000 0x101000 0x102000 0x103000 }
set traceFunnelAddresses { 0x18000 }
set caBaseAddresses { }

proc wordscollected {core} {
    global te_sinkwp
    global traceBaseAddrArray
    global te_sinkbase_offset

#    echo "wordscollected($core)"

    set tracewp [gettracewp $core]

    switch [string toupper [getSink $core]] {
        "SRAM" { if {$tracewp & 1} {
            set size [getTraceBufferSize $core]
            return "[expr $size / 4] trace words collected"
            }

            return "[expr $tracewp / 4] trace words collected"
        }
        "SBA"  { if {$tracewp & 1} {
            set size [getTraceBufferSize $core]
            return "[expr $size / 4] trace words collected"
            }

            set tracebegin [word [expr $traceBaseAddrArray($core) + $te_sinkbase_offset]]
            set traceend $tracewp

            return "[expr ($traceend - $tracebegin) / 4] trace words collected"
        }
    }

    return "unknown trace words collected"
}

# Surprisingly, Jim Tcl lacks a primitive that returns the value of a
# register.  It only exposes a "cooked" line of output suitable for
# display.  But we can use that to extrace the actual register value
# and return it

proc regval {name} {
    set displayval [reg $name]
    set splitval [split $displayval ':']
    set val [lindex $splitval 1]
    return [string trim $val]
}

proc wp_control_set {core bit} {
    global wp_control_offset
    global traceBaseAddresses

#    echo "wp_control_set($core $bit)"

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] | (1 << $bit)]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_clear {core bit} {
    global wp_control_offset
    global traceBaseAddresses

#    echo "wp_control_clear($core $bit)"

    foreach baseAddress $traceBaseAddresses {
        set newval [expr [word [expr $baseAddress + $wp_control_offset]] & ~(1 << $bit)]
        mww [expr $baseAddress + $wp_control_offset] $newval
    }
}

proc wp_control_get_bit {core bit} {
    global wp_control_offset
    global traceBaseAddresses

#    echo "wp_control_get_bit($core $bit)"

    set baseAddress [lindex $traceBaseAddresses 0]
    return [expr ([word [expr $baseAddress + $wp_control_offset]] >> $bit) & 0x01]
}

proc wp_control {cores {bit ""} {val ""}} {
#    echo "wp_control($cores $bit $val)"

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

proc inittracing {} {
    global traceBaseAddresses
    global traceBaseAddrArray
    global traceFunnelAddresses
    global traceFunnelAddrArray
    global caBaseAddresses
    global CABaseAddrArray
    global te_control_offset
    global num_cores
    global num_funnels

    if {[info exists traceFunnelAddresses] == 0} {
      set traceFunnelAddresses { 0 }
    }

    # put all cores and funnels in a known state
    changeAllTeControls $te_control_offset 6 1
    changeAllTfControls $te_control_offset 6 1

    set core 0

    foreach addr $traceBaseAddresses {
        set traceBaseAddrArray($core) $addr
        #setSink $core "SRAM"
        incr core
    }

    set num_cores $core
    set core 0

    foreach addr $caBaseAddresses {
        set CABaseAddrArray($core) $addr
        incr core
    }

    # find the master funnel. If there is only one funnel, it will be it. Otherwise
    # look at all funnel destinations supported for each funnel

    # the idea is we will set all funnels except master to feed into the master

    set num_funnels 0

    if {($traceFunnelAddresses != 0) && ($traceFunnelAddresses != "")} {
        foreach addr $traceFunnelAddresses {
            if {[tfHasFunnelSink $addr] != 0} {
                set traceFunnelAddrArray($num_funnels) $addr
                incr num_funnels
            } else {
                set traceBaseAddrArray(funnel) $addr
            }
        }
        incr num_funnels
    }

    echo -n ""
}

inittracing

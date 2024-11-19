
proc xti_action_read {core idx} {
    global xti_control_offset
    global traceBaseAddrArray

#    echo "xti_action_read($core $idx)"

    return [expr ([word [expr $traceBaseAddrArray($core) + $xti_control_offset]] >> ($idx*4)) & 0xF]
}

proc xti_action_write {core idx val} {
    global xti_control_offset
    global traceBaseAddrArray

#    echo "xti_action_write($core [format 0x%08lx $idx] [format 0x%08lx $val])"

    set zeroed [expr ([word [expr $traceBaseAddrArray($core) + $xti_control_offset]] & ~(0xF << ($idx*4)))]
    set ored [expr ($zeroed | (($val & 0xF) << ($idx*4)))]
    mww [expr $traceBaseAddrArray($core) + $xti_control_offset] $ored
}

proc xto_event_read {core idx} {
    global xto_control_offset
    global traceBaseAddrArray

#    echo "xto_event_read($core $idx)"

    return [expr ([word [expr $traceBaseAddrArray($core) + $xto_control_offset]] >> ($idx*4)) & 0xF]
}

proc xto_event_write {core idx val} {
    global xto_control_offset
    global traceBaseAddrArray

#    echo "xto_even_write($core [format 0x%08lx $idx] [format 0x%08lx $val])"

    set zeroed [expr ([word [expr $traceBaseAddrArray($core) + $xto_control_offset]] & ~(0xF << ($idx*4)))]
    set ored [expr ($zeroed | (($val & 0xF) << ($idx*4)))]
    mww [expr $traceBaseAddrArray($core) + $xto_control_offset] $ored
}

proc xti_action {cores {idx ""} {val ""}} {
#    echo "xti_action($cores $idx $val)"

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
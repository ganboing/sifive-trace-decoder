set eventIdxs { 0 1 2 3 4 5 }

proc eventStr {event} {
    switch $event {
       0       { return "trigger"    }
       1       { return "watchpoint" }
       2       { return "call"       }
       3       { return "interrupt"  }
       4       { return "exception"  }
       5       { return "context"    }
       default { return "unknown($event)" }
    }
}

proc eventParse {event} {
    switch $event {
       "trigger"    { return 0 }
       "watchpoint" { return 1 }
       "call"       { return 2 }
       "interrupt"  { return 3 }
       "exception"  { return 4 }
       "context"    { return 5 }
       default      { return -1 }
    }
}

proc evGetControl {core} {
    global traceBaseAddrArray
    global ev_control_offset
    global eventIdxs

    set events [word [expr {$traceBaseAddrArray($core) + $ev_control_offset}]]
    set eventl ""

    foreach ev $eventIdxs {
        if {($events & (1 << $ev)) != 0} {
            append eventl " " [eventStr $ev]
        }
    }
    echo "evGetControl\[$core\]:$eventl"
    return $eventl
}

proc evSetControl {core opts} {
    global traceBaseAddrArray
    global ev_control_offset

    set reg 0

    foreach opt $opts {
        switch $opt {
        "none"       { set reg 0 }
        "all"        { set reg 0x3f }
        "trigger"    { set reg [expr {$reg | (1 << 0)}] }
        "watchpoint" { set reg [expr {$reg | (1 << 1)}] }
        "call"       { set reg [expr {$reg | (1 << 2)}] }
        "interrupt"  { set reg [expr {$reg | (1 << 3)}] }
        "exception"  { set reg [expr {$reg | (1 << 4)}] }
        "context"    { set reg [expr {$reg | (1 << 5)}] }
        default      { return 1 }
        }
    }

    mww [expr {$traceBaseAddrArray($core) + $ev_control_offset}] $reg

    return 0
}

# Individual access to each event control bit works better in the case of
# certain UI environment(s) that may be accessing this script.  These routines are
# roughly equivalent to getEventControl and setEventControl, except they access each
# event bit individually, and also don't affect the maxIcnt setting.  Some environments
# will prefer to use getEventControl and setEventControl instead.
proc evSetBit { core bit enable } {
    global traceBaseAddrArray
    global ev_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $ev_control_offset}]
    set events [word $reg]
    if {$enable} {
        set events [expr {$events | (1 << $bit)}]
    } else {
        set events [expr {$events & ~(1 << $bit)}]
    }

    mww $reg $events
    return 0
}

proc evGetBit { core bit } {
    global traceBaseAddrArray
    global ev_control_offset

    set events [word [expr $traceBaseAddrArray($core) + $ev_control_offset]]
    return [expr {($events & (1 << $bit)) != 0}]
}

proc evSetCtlTrigger { core enable } {
    return [evSetBit $core 0 $enable]
}

proc evGetCtlTrigger { core } {
    return [evGetBit $core 0]
}

proc evSetCtlWatchpoint { core enable } {
    return [evSetBit $core 1 $enable]
}

proc evGetCtlWatchpoint { core } {
    return [evGetBit $core 1]
}

proc evSetCtlCall { core enable } {
    return [evSetBit $core 2 $enable]
}

proc evGetCtlCall { core } {
    return [evGetBit $core 2]
}

proc evSetCtlInterrupt { core enable } {
    return [evSetBit $core 3 $enable]
}

proc evGetCtlInterrupt { core } {
    return [evGetBit $core 3]
}

proc evSetCtlException { core enable } {
    return [evSetBit $core 4 $enable]
}

proc evGetCtlException { core } {
    return [evGetBit $core 4]
}

proc evSetCtlContext { core enable } {
    return [evSetBit $core 5 $enable]
}

proc evGetCtlContext { core } {
    return [evGetBit $core 5]
}

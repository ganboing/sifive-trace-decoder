proc tsGetSrc {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set t [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    set t [expr {($t >> 4) & 0x7}]
    switch $t {
        0       { return "none"     }
        1       { return "external" }
        2       { return "internal" }
        3       { return "core"     }
        4       { return "slave"    }
        default { return "reserved" }
    }
}

proc tsSetSrc {comp src} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set s 0
    switch $src {
        "none"       { set s 0 }
        "external"   { set s 1 }
        "internal"   { set s 2 }
        "core"       { set s 3 }
        "slave"      { set s 4 }
        default      { return 1 }
    }
    set tsctl [word $reg]
    set tsctl [expr {$tsctl & ~0x70}]
    set tsctl [expr {$tsctl | ($s << 4)}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsSetSrc\[$comp\]: source=$src tsctl=%x" $tsctl]
}

proc tsStatus {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set src [expr {($tsctl >> 4) & 0x7}]
    if {($tsctl & 0x1) == 0} {
        return "disabled"
    }
    if {$src == 0} {
        return "disconnected"
    }
    if {$src != 2} {
        return "running"
    }
    if {($tsctl & 0x2) != 0} {
        return "intrunning"
    }
    return "stopped"
}

proc tsEnable {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl | 0x1}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsEnable\[$comp\]: tsctl=%x" $tsctl]
}

proc tsDisable {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl & ~0x8003}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsDisable\[$comp\]: tsctl=%x" $tsctl]
}

proc tsStart {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl | 0x8003}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsStart\[$comp\]: tsctl=%x" $tsctl]
}

proc tsStop {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl & ~0x8002}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsStop\[$comp\]: tsctl=%x" $tsctl]
}

proc tsReset {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl | 0x4}]
    mww $reg $tsctl
    set tsctl [word $reg]
    echo [format "tsReset\[$comp\]: tsctl=%x" $tsctl]
}

proc tsGetTime {comp} {
    global traceBaseAddrArray
    global ts_lower_offset
    global ts_upper_offset

    set timelo [word [expr {$traceBaseAddrArray($comp) + $ts_lower_offset}]]
    set timehi [word [expr {$traceBaseAddrArray($comp) + $ts_upper_offset}]]
    set t [expr {($timehi << 32) | $timelo}]
    echo [format "tsGetTime\[$comp\]: %016lx" $t]

    return $t
}

proc getTsDebugNonStop {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set tsctl [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    if {[expr {$tsctl & 0x8}] != 0} {
        return "on"
    }

    return "off"
}

proc tsEnableDebugNonStop {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl | 0x8}]
    mww $reg $tsctl

    return 0
}

proc tsDisableDebugNonStop {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set tsctl [word $reg]
    set tsctl [expr {$tsctl & ~0x8}]
    mww $reg $tsctl

    return 0
}

proc tsGetPrescale {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set t [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    set t [expr {($t >> 8) & 0x3}]
    switch $t {
        0 { return 1  }
        1 { return 4  }
        2 { return 16 }
        3 { return 64 }
    }
}

proc tsSetPrescale {comp prescl} {
    global traceBaseAddrArray
    global ts_control_offset

    switch $prescl {
        1       { set ps 0 }
        4       { set ps 1 }
        16      { set ps 2 }
        64      { set ps 3 }
        default { set ps 0 }
    }

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x0300}]
    set t [expr {$t | ($ps << 8)}]
    mww $reg $t
}

proc tsGetBranch {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set t [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    set t [expr {($t >> 16) & 0x3}]
    switch $t {
        0 { return "off"  }
        1 { return "indirect+exception"  }
        2 { return "reserved" }
        3 { return "all" }
    }
}

proc tsSetBranch {comp branch} {
    global traceBaseAddrArray
    global ts_control_offset

    switch $branch {
        "off"                { set br 0 }
        "indirect"           { set br 1 }
        "reserved"           { set br 2 }
        "all"                { set br 3 }
        default              { set br 0 }
    }

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x30000}]
    set t [expr {$t | ($br << 16)}]
    mww $reg $t
}

proc tsSetITC {comp itc} {
    global traceBaseAddrArray
    global ts_control_offset

    switch $itc {
        "on"    { set f 1 }
        "off"   { set f 0 }
        default { set f 0 }
    }

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x40000}]
    set t [expr {$t | ($f << 18)}]
    mww $reg $t
}

proc tsGetITC {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set t [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    set t [expr {($t >> 18) & 0x1}]

    switch $t {
        0 { return "off"  }
        1 { return "on"  }
    }
}

proc tsSetOwner {comp owner} {
    global traceBaseAddrArray
    global ts_control_offset

    switch $owner {
        "on"    { set f 1 }
        "off"   { set f 0 }
        default { set f 0 }
    }

    set reg [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x80000}]
    set t [expr {$t | ($f << 19)}]
    mww $reg $t
}

proc tsGetOwner {comp} {
    global traceBaseAddrArray
    global ts_control_offset

    set t [word [expr {$traceBaseAddrArray($comp) + $ts_control_offset}]]
    set t [expr {($t >> 19) & 0x1}]

    switch $t {
        0 { return "off"  }
        1 { return "on"  }
    }
}

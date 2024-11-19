# local helper functions not intended to be called directly

proc wordhex {addr} {
    return [format "0x%08x" [read_memory $addr 32 1]]
}

proc word {addr} {
    return [read_memory $addr 32 1]
}

proc probeReg {reg {mask 0xffffffff}} {
    set t [word $reg]
    mww $reg $mask
    set w [word $reg]
    mww $reg $t
    return $w
}

proc changeAllTeControls {offset to0 to1} {
    global traceBaseAddresses

    foreach controlReg $traceBaseAddresses {
        set reg [expr {$controlReg + $offset}]
        set val [word $reg]
    echo [format "TeControlReg %x -> %x" $reg $val]
        set val [expr {$val & ~$to0}]
    set val [expr {$val | $to1}]
        echo [format "TeControlReg %x <- %x" $reg $val]
    mww $reg $val
    }
}

proc changeAllTfControls {offset to0 to1} {
    global traceFunnelAddresses

    foreach controlReg $traceFunnelAddresses {
        set reg [expr {$controlReg + $offset}]
        set val [word $reg]
        echo [format "TfControlReg %x -> %x" $reg $val]
        set val [expr {$val & ~$to0}]
        set val [expr {$val | $to1}]
        echo [format "TfControlReg %x <- %x" $reg $val]
        mww $reg $val
    }
}

proc setAllTeControls {offset val} {
    global traceBaseAddresses

#    echo "setAllTeControls([format 0x%08lx $offset] [format 0x%08lx $val])"

    foreach controlReg $traceBaseAddresses {
        mww [expr $controlReg + $offset] $val
    }
}

proc setAllTfControls {offset val} {
    global traceFunnelAddresses

#    echo "setAllTfControls([format 0x%08lx $offset] [format 0x%08lx $val])"

    if {($traceFunnelAddresses != 0) && ($traceFunnelAddresses != "")} {
        foreach controlReg $traceFunnelAddresses {
            mww [expr $controlReg + $offset] $val
        }
    }
}

# Returns list of all cores and funnel if present

proc getAllCoreFunnelList {} {
    global traceBaseAddresses
    global traceFunnelAddresses

    set cores {}
    set index 0

    foreach controlReg $traceBaseAddresses {
        lappend cores $index
        set index [expr {$index + 1}]
    }

    if {($traceFunnelAddresses != 0) && ($traceFunnelAddresses != "")} {
        lappend cores funnel
    }

    return $cores
}

# Returns list of all cores (but not funnel)

proc getAllCoreList {} {
    global traceBaseAddresses

    set cores {}
    set index 0

    foreach controlReg $traceBaseAddresses {
        lappend cores $index
        incr index
    }

    return $cores
}

# returns a list struct from parsing $cores with each element a core id

proc parseCoreFunnelList {cores} {
    global num_cores
    global num_funnels

    # parse core and build list of cores

    if {$cores == "all" || $cores == ""} {
        return [getAllCoreFunnelList]
    }

    set t [split $cores ","]

    foreach core $t {
        if {$core == "funnel"} {
            # only accept funnel if one is present

            if {$num_funnels == 0} {
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

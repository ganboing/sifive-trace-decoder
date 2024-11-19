
proc is_itc_implmented {core} {
    # Caller is responsible for enabling trace before calling this
    # proc, otherwise behavior is undefined

    global itc_traceenable_offset
    global traceBaseAddrArray

    # We'll write a non-zero value to itc_traceenable, verify a
    # non-zero readback, and restore the original value

    set reg [expr {$traceBaseAddrArray($core) + $itc_traceenable_offset}]
    set originalval [word $reg]
    mww $reg 0xFFFFFFFF
    set readback [word $reg]
    echo [format "is_itc_implmented: core\[$core\] itc_traceen=%x" $readback]
    set result [expr {$readback != 0}]
    mww $reg $originalval

    return $result
}


proc itcSet {core mode} {
    global traceBaseAddrArray
    global te_control_offset

    switch $mode {
        "off"           { set itc 0 }
        "all"           { set itc 1 }
        "ownership"     { set itc 2 }
        "all+ownership" { set itc 3 }
        default         { set itc 0 }
    }

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr {$t & ~0x0180}]
    set t [expr {$t | ($itc << 7)}]
    mww $reg $t
}

proc itcGet {core} {
    global traceBaseAddrArray
    global te_control_offset

    set t [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
    set t [expr {($t >> 7) & 0x3}]

    switch $t {
        0       { return "none" }
        1       { return "all"  }
        2       { return "ownership" }
        3       { return "all+ownership" }
        default { return "reserved" }
    }
}

proc itcSetMask {core mask} {
    global traceBaseAddrArray
    global itc_traceenable_offset

    mww [expr {$traceBaseAddrArray($core) + $itc_traceenable_offset}] [expr $mask]
}

proc itcGetMask {core} {
    global traceBaseAddrArray
    global itc_traceenable_offset

    set mask [word [expr {$traceBaseAddrArray($core) + $itc_traceenable_offset}]]

    return [format "%x" $mask]
}

proc setITCTriggerMask {core mask} {
    global traceBaseAddrArray
    global itc_trigenable_offset

    mww [expr {$traceBaseAddrArray($core) + $itc_trigenable_offset}] [expr {$mask}]
}

proc getITCTriggerMask {core} {
    global traceBaseAddrArray
    global itc_trigenable_offset

    set mask [word [expr {$traceBaseAddrArray($core) + $itc_trigenable_offset}]]

    return [format "%x" $mask]
}

proc itc {{cores "all"} {opt ""} {mask ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: itc [corelist] [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
        return "error"
    }

    if {$opt == ""} {
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [itcGet $core]

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

                    lappend tsd [itcGetMask $core]

                    if {$rv != ""} {
                    append rv "; "
                    }

                    append rv $tsd
                }

                return $rv
            }
        } else {
            foreach core $coreList {
                itcSetMask $core $mask
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
            itcSet $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: itc [corelist] [off | ownership | all | all+ownership | mask [ nn ] | trigmask [ nn ] | help]}
    }
}

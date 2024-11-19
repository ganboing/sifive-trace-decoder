proc teversion {{cores "all"} {opt ""}} {
    global te_impl_offset
    global traceBaseAddresses

    set coreList [parseCoreList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: teversion [corelist] [help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tev "core $core: "

            lappend tev [teGetTeVersion $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $tev
        }

        return $rv
    } elseif {$opt == "help"} {
        echo "teversion: display trace encoder version"
        echo {Usage: teversion [corelist] [help]}
        echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is"
        echo "            equivalent to all"
        echo "  help:     Display this message"
        echo ""
        echo "teversion with no arguments will display the trace encoder version for all cores"
        echo ""
    } else {
        echo {Usage: teversion [corelist] [help]}
    }
}

proc stoponwrap {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: stoponwrap [corelist] [on | off | help]}
        return "error"
    }

    if {$opt == ""} {
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [teGetStopOnWrap $core]

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
            teSetStopOnWrap $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: stoponwrap [corelist] [on | off | help]}
    }
}

proc maxicnt {{cores "all"} {opt ""}} {
    set coreList [parseCoreList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: maxicnt [corelist] [5 - 10 | help]}
        return "error"
    }

    if {$opt == ""} {
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [expr {[teGetMaxIcnt $core] + 4}]

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
        echo "  nn:       Set max i-cnt value to 2^(nn+4). nn must be between 0 and 10 for"
        echo "            a range between 16 and 16384"
        echo "  help:     Display this message"
        echo ""
        echo "maxicnt with no arguments will display the current maximum i-cnt value"
        echo ""
    } elseif {$opt >= 4 && $opt < 15} {
        foreach core $coreList {
            teSetMaxIcnt $core [expr {$opt - 4}]
        }
        echo -n ""
    } else {
        echo {Error: Usage: maxicnt [corelist] [5 - 10 | help]}
    }
}

proc maxbtm {{cores "all"} {opt ""}} {
    set coreList [parseCoreList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: maxbtm [corelist] [5 - 16 | help]}
        return "error"
    }

    if {$opt == ""} {
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [expr {[teGetMaxBTM $core] + 5}]

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
            teSetMaxBTM $core [expr {$opt - 5}]
        }
        echo -n ""
    } else {
        echo {Error: Usage: maxbtm [corelist] [5 - 16 | help]}
    }
}

proc tracemode {{cores "all"} {opt ""}} {
    set coreList [parseCoreList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tracemode [corelist] [btm | htm | htmc | none | sample | event | help]}
        return "error"
    }

    if {$opt == ""} {
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [ teGetMode $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $tsd
        }

        return $rv
    }

    if {$opt == "help"} {
        echo "tracemode: set or display trace type (sync, sync+btm)"
        echo {Usage: tracemode [corelist] [none | all | btm | htm | htmc | sample | event | help]}
        echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
        echo "  btm:      Generate both sync and btm trace messages"
        echo "  htm:      Generate sync and htm trace messages (with return stack optimization or repeat branch optimization)"
        echo "  htmc      Generate sync and conservitive htm trace messages (without return stack optimization or repeat branch optimization)"
        echo "  sample    Generate PC sample trace using In Circuit Trace mode"
        echo "  event     Generate event trace. Use eventmode to select events"
        echo "  none:     Do not generate sync or btm trace messages"
        echo "  help:     Display this message"
        echo ""
        echo "tracemode with no arguments will display the current setting for the type"
        echo "of messages to generate (none, sync, or all)"
        echo ""
    } elseif {($opt == "sample") || ($opt == "event") || ($opt == "all") || ($opt == "none") || ($opt == "btm") || ($opt == "htm") || ($opt == "htmc")} {
        foreach core $coreList {
            teSetMode $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: tracemode [corelist] [all | btm | htm | htmc | none | sample | event | help]}
    }
}
proc ts {{cores "all"} {opt ""}} {
    global ts_control_offset

    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: ts [corelist] [enable | disable | start | stop | reset | help]]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tse "core $core: "

            lappend tse [tsStatus $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $tse
        }

        return $rv
    } elseif {$opt == "help"} {
        echo "ts: set or display timestamp mode"
        echo {Usage: ts [corelist] [enable | disable | start | stop | reset | help]}
        echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is"
        echo "            equivalent to all"
        echo "  enable:   Enable  timestamps in trace messages"
        echo "  disable:  Disable timestamps in trace messages"
        echo "  start:    Start  timstamping in trace messages"
        echo "  stop:     Stop   timstamping in trace messages"
        echo "  reset:    Reset the internal timestamp to 0"
        echo "  help:     Display this message"
        echo ""
        echo "ts with no arguments will display the current status of timestamps (on or off)"
        echo ""
    } elseif {$opt == "enable"} {
        foreach core $coreList {
            tsEnable $core
        }
        echo -n ""
    } elseif {$opt == "disable"} {
        foreach core $coreList {
            tsDisable $core
        }
        echo -n ""
    } elseif {$opt == "start"} {
        foreach core $coreList {
            tsStart $core
        }
        echo -n ""
    } elseif {$opt == "stop"} {
        foreach core $coreList {
            tsStop $core
        }
        echo -n ""
    } elseif {$opt == "reset"} {
        foreach core $coreList {
            tsReset $core
        }
        echo "timestamp reset"
    } else {
        echo {Error: Usage: ts [corelist] [enable | disable | start | stop | reset | help]]}
    }
}

proc tsnonstop {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tsnonstop [corelist] [on | off | help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [getTsDebugNonStop $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $tsd
        }

        return $rv
    }

    if {$opt == "help"} {
        echo "tsnonstop: set or display if timestamp internal clock runs while in debug"
        echo {Usage: tsnonstop [corelist] [on | off | help]}
        echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is"
        echo "            equivalent to all"
        echo "  on:       Timestamp clock continues to run while in debug"
        echo "  off:      Timnestamp clock halts while in debug"
        echo "  help:     Display this message"
        echo ""
        echo "tsnonstop with no arguments will display the current status of timstamp debug"
        echo "(on or off)"
        echo ""
    } elseif {$opt == "on"} {
        # iterate through coreList and enable timestamps
        foreach core $coreList {
            tsEnableDebugNonStop $core
        }
        echo -n ""
    } elseif {$opt == "off"} {
        foreach core $coreList {
            tsDisableDebugNonStop $core
        }
        echo -n ""
    } else {
        echo {Error: Usage: tsdebug [corelist] [on | off | help]}
    }
}

proc tsclock {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tsclock [corelist] [help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [tsGetSrc $core]

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
        foreach core $coreList {
            tsSetSrc $core $opt
        }
    }
}

proc tsprescale {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]
    
    if {$coreList == "error"} {
        echo {Error: Usage: tsprescale [corelist] [1 | 4 | 16 | 64 | help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [tsGetPrescale $core]

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
            tsSetPrescale $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: tsprescale [corelist] [1 | 4 | 16 | 64 | help]}
    }
}

proc tsbranch {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tsbranch [coreslist] [off | indirect | all | help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [tsGetBranch $core]

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
    } elseif {($opt == "off") || ($opt == "indirect") || ($opt == "all")} {
        foreach core $coreList {
            tsSetBranch $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: tsbranch [corelist] [off | indirect | all | help]}
    }
}

proc tsitc {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tsitc [corelist] [on | off | help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [tsGetITC $core]

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
            tsSetITC $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: tsitc [corelist] [on | off | help]}
    }
}

proc tsowner {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: tsowner [corelist] [on | off | help]}
        return "error"
    }

    if {$opt == ""} {
        # display current status of ts enable
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [tsGetOwner $core]

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
            tsSetOwner $core $opt
        }
        echo -n ""
    } else {
        echo {Error: Usage: tsowner [corelist] [on | off | help]}
    }
}

proc tsallstart {} {
    ts all enable
    tsbranch all all
    tsitc all on
    tsowner all on
    ts all start
}
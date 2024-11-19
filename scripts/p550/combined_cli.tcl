proc sinkstatus {{cores "all"} {action ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: sinkstatus [corelist] [reset | help]}
        return "error"
    }

    if {$action == ""} {
        # display current status of teSinkError
        set rv ""

        foreach core $coreList {
            if {$core == "funnel"} {
                set te "$core: "
            } else {
                set te "core $core: "
            }

            lappend te [txGetSinkError $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $te
        }

        return $rv
    }

    if {$action == "help"} {
        echo "sinkstatus: display or clear status of teSinkError bit in the teControl register"
        echo {Usage: sinkstatus [corelist] [clear | help]}
        echo "  clear:    Clear the teSinkError bit"
        echo "  help:     Display this message"
        echo ""
        echo "sinkstatus with no arguments will display the status of the teSinkError bit for"
        echo "all cores and the funnel (if present)"
        echo ""
    } elseif {$action == "clear"} {
        foreach core $coreList {
            txClearSinkError $core
        }
    }
}

proc tracedst {{cores ""} {dst ""} {addr ""} {size ""}} {
    global traceBaseAddrArray
    global te_control_offset
    global te_impl_offset
    global te_sinkbase_offset
    global te_sinkbasehigh_offset
    global te_sinklimit_offset
    global num_funnels
    global te_sinkwp_offset
    global te_sinkrp_offset
    global trace_buffer_width

    switch [string toupper $cores] {
        ""       {
            set cores "all" }
        "SBA"    {
            set size $addr
            set addr $dst
            set dst "sba"
            set cores "all"
        }
        "SRAM"   {
            set dst "sram"
            set cores "all"
        }
        "ATB"    {
            set dst "atb"
            set cores "all"
        }
        "PIB"    {
            set dst "pib"
            set cores "all"
        }
        "HELP"   {
            set dst "help"
            set cores "all"
        }
        "FUNNEL" {
            switch [string toupper $dst] {
                ""     {}
                "SBA"  {}
                "SRAM" {}
                "ATB"  {}
                "PIB"  {}
                "HELP" {}
                "FUNNEL" {}
                default {
                    set size $addr
                    set addr $dst
                    set dst "funnel"
                    set cores "all"
                }
            }
        }
        default  {
            echo "Unkown sink $cores"
            return 1
        }
    }

    set coreFunnelList [parseCoreFunnelList $cores]
    set coreList [parseCoreList $cores]

    if {$coreFunnelList == "error"} {
        echo {Usage: tracedst [corelist] [sram | atb | pib | funnel | sba [base size] | help]}
        return "error"
    }

    if {$dst == ""} {
        set teSink {}
        foreach core $coreFunnelList {
            set sink [txGetSinkType $core]

            if {$teSink != ""} {
                append teSink "; "
            }

            append teSink " core: $core $sink"

            switch [string toupper $sink] {
                "SRAM"  {
                    # get size of SRAM
                    append teSink [format " sizebits=%d" [txProbeSRAMSinkBits $core]]
                }
                "SBA"   {
                    append teSink [format " basebits=%d sizebits=%d base=%x limit=%x" \
                         [txProbeMemBaseMaxBits $core] [txProbeMemLimitMaxBits $core] \
                         [txGetBufferBase $core] [txGetBufferSize $core]]
                }
            }
        }
        return $teSink
    } elseif {[string compare -nocase $dst "help"] == 0} {
        echo "tracedst: set or display trace sink for cores and funnel"
        echo {Usage: tracedst [corelist] [sram | atb | pib | funnel | sba [base size] | help]}
        echo "  corelist: Comma separated list of core numbers, funnel, or 'all'. Not specifying is equivalent to all"
        echo "  sram:     Set the trace sink to on-chip sram"
        echo "  atb:      Set the trace sink to the ATB"
        echo "  pib:      Set the trace sink to the PIB"
        echo "  funnel:   set the trtace sink to the funnel"
        echo "  sba:      Set the trace sink to the system memory at the specified base and limit. If no specified"
        echo "            they are left as previously programmed"
        echo "  base:     The address to begin the sba trace buffer in system memory at"
        echo "  size:     Size of the sba buffer in bytes. Must be a multiple of 4"
        echo "  help:     Display this message"
        echo ""
        echo "tracedst with no arguments will display the trace sink for all cores and the funnel (if present)"
        echo ""
        echo "If no cores are specified and there is no trace funnel, all cores will be programed with the"
        echo "sink specified. If no cores are specified and there is a trace funnel, all cores will be"
        echo "programmed to sink to the funnel and the funnel will be programmed to use the sink specified"
        echo ""
    } elseif {[string compare -nocase $dst funnel] == 0} {
            if {$num_funnels == 0} {
                return "Error: funnel not present"
            }

            foreach $core $coreList {
                set rc [txSetSink $core FUNNEL]
                if {$rc != 0} {
                    return "Failed to set sink of core $core to funnel"
                }
            }
    } else {
        if {$cores == "all"} {
            if {$num_funnels > 0} {
                foreach core $coreList {
                    set rc [txSetSink $core FUNNEL]
                    if {$rc != 0} {
                        return "Failed to set sink of core $core to funnel"
                    }
                }
                set rc [txSetSink funnel [string toupper $dst] $addr $size]
                if {$rc != 0} {
                    return "Failed to set sink of funnel to $dst ($size@$size)"
                }
            } else {
                foreach core $coreList {
                    set rc [txSetSink $core [string toupper $dst] $addr $size]
                    if {$rc != 0} {
                        return "Failed to set sink of core $core to $dst ($size@$addr)"
                    }
                }
            }
        } else {
            foreach core $coreFunnelList {
                set rc [txSetSink $core [string toupper $dst] $addr $size]
                if {$rc != 0} {
                    return "Failed to set sink of core $core to $dst ($size@$addr)"
                }
            }
        }
    }

    return ""
}

proc printtracebaseaddresses {} {
    global traceBaseAddresses
    global traceFunnelAddresses
    global num_funnels

    set core 0

    foreach baseAddress $traceBaseAddresses {
        echo "core $core: trace block at $baseAddress"
        set core [expr {$core + 1}]
    }

    if {$num_funnels > 0} {
        foreach funnel $traceFunnelAddresses {
            echo "Funnel block at $funnel"
        }
    }

    echo -n ""
}

proc trace {{cores "all"} {opt ""}} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: trace [corelist] [on | off | reset | settings | help]}
        return "error"
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

            append te "[txIsEnabled $core] [txIsRunning $core]"

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
        echo "  enable:   Enable tracing"
        echo "  disable:  Disable tracing"
        echo "  start:    Start tracing"
        echo "  stop:     Stop tracing"
        echo "  reset:    Reset trace encoder"
        echo "  settings: Display current trace related settings"
        echo "  help:     Display this message"
        echo ""
        echo "trace with no arguments will display if tracing is currently enabled for all cores (on or off)"
        echo ""
    } elseif {$opt == "enable"} {
        foreach core $coreList {
            txEnable $core
        }
    } elseif {$opt == "disable"} {
        foreach core $coreList {
            txDisable $core
        }
    } elseif {$opt == "start"} {
        foreach core $coreList {
            txStart $core
        }
    } elseif {$opt == "stop"} {
        foreach core $coreList {
            txStop $core
        }
    } elseif {$opt == "reset"} {
        foreach core $coreList {
            txDisable $core
            txResetBufferPtr $core
        }
    } elseif {$opt == "settings"} {
        echo "ts: [ts $cores]"
        echo "tsitc: [tsitc $cores]"
        echo "tsclock: [tsclock $cores]"
        echo "tsowner: [tsowner $cores]"
        echo "tsbranch: [tsbranch $cores]"
        echo "tsnonstop: [tsnonstop $cores]"
        echo "tracemode: [tracemode $cores]"
        echo "tsprescale: [tsprescale $cores]"
        echo "stoponwrap: [stoponwrap $cores]"
        echo "itc: [itc $cores]"
        echo "maxbtm: [maxbtm $cores]"
        echo "maxicnt: [maxicnt $cores]"
    } else {
        echo {Error: Usage: trace [corelist] [on | off | reset | settings | help]}
    }
}

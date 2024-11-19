proc eventmode {{cores "all"} args} {
    set coreList [parseCoreFunnelList $cores]

    if {$coreList == "error"} {
        echo {Error: Usage: eventmode [corelist] [none | all | sample | trigger | watchpoint | call | interrupt | exception | context | help]}
        return "error"
    }

    if {$args == ""} {
        # display current status of evControl
        set rv ""

        foreach core $coreList {
            set tsd "core $core: "

            lappend tsd [evGetControl $core]

            if {$rv != ""} {
                append rv "; "
            }

            append rv $tsd
        }

        return $rv
    }

    if {$args == "help"} {
        echo "eventmode: set or display event enabled mode"
        echo {Usage: eventmode [corelist] [none | all | sample | trigger | watchpoint | call | interrupt | exception | context]}
        echo "  corelist: Comma separated list of core numbers, or 'all'. Not specifying is equivalent to all"
        echo "  sample:     Enable pc sampling"
        echo "  trigger:    Enable trigger event messagaging"
        echo "  watchpoint: Enable watchpoint event messaging"
        echo "  call:       Enable call event messaging"
        echo "  interrupt:  Enable interrupt event messaging"
        echo "  exception:  Enable exception event messaging"
        echo "  context:    Enable context switch event messaging"
        echo "  all:        Enable all event messaging"
        echo "  none:       Do not generate any event messaging"
        echo "  help:     Display this message"
        echo ""
        echo "eventmode with no arguments will display the current setting for event messaging"
        echo "To enable multiple events, separate the event types with a space"
        echo ""
    } else {
        foreach core $coreList {
            set rc [evSetControl $core $args]
            if {$rc != 0} {
                echo {Error: Usage: eventmode [corelist] [none | all | sample | trigger | watchpoint | call | interrupt | exception | context | help]}
                return ""
            }
        }
    }
    echo -n ""
}
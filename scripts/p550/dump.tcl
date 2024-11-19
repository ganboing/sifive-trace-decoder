# dump registers (at least some of them)

proc dts {{cores "all"}} {
    global traceBaseAddrArray
    global traceFunnelAddrArray
    global te_control_offset
    global te_impl_offset
    global ts_control_offset
    global ts_lower_offset
    global ts_upper_offset

    global num_funnels
    global has_event

    if {$num_funnels > 0} {
        set coreList [parseCoreFunnelList $cores]
    } else {
        set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
        echo "Error: Usage: dts [corelist]"
        return "error"
    }

    set doFunnel 0

    foreach core $coreList {
        if {$core == "funnel"} {
            set doFunnel 1
        } else {
            set tsc [word [expr {$traceBaseAddrArray($core) + $ts_control_offset}]]
            set tsc [word [expr {$traceBaseAddrArray($core) + $ts_control_offset}]]
            set tsl [word [expr {$traceBaseAddrArray($core) + $ts_lower_offset}]]
            set tsu [word [expr {$traceBaseAddrArray($core) + $ts_upper_offset}]]
            set tsw [expr {($tsc >> 24)}]
            echo [format "dts: core\[$core\]: ts_control=0x%08x ts_width=%d ts=0x%08x%08x" $tsc $tsw $tsu $tsl]
        }
    }

    if {$doFunnel != 0} {
        if {$num_funnels > 0} {
            for {set i 0} {$i < ($num_funnels - 1)} {incr i} {
                set fa [format "0x%08x" $traceFunnelAddrArray($i)]
                set tsc [word [expr {$traceFunnelAddrArray($i) + $ts_control_offset}]]
                set tsl [word [expr {$traceFunnelAddrArray($i) + $ts_lower_offset}]]
                set tsu [word [expr {$traceFunnelAddrArray($i) + $ts_upper_offset}]]
                set tsw [expr {($tsc >> 24)}]
                echo [format "dts: funnel @$fa: ts_control=0x%08x ts_width=%d ts=0x%08x%08x" $tsc $tsw $tsu $tsl]
            }
        }

        set fa [format "0x%08x" $traceBaseAddrArray(funnel)]
        set tsc [word [expr {$traceBaseAddrArray(funnel) + $ts_control_offset}]]
        set tsl [word [expr {$traceBaseAddrArray(funnel) + $ts_lower_offset}]]
        set tsu [word [expr {$traceBaseAddrArray(funnel) + $ts_upper_offset}]]
        set tsw [expr {($tsc >> 24)}]
        echo [format "dts: master funnel @$fa: ts_control=0x%08x ts_width=%d ts=0x%08x%08x" $tsc $tsw $tsu $tsl]
    }
}

proc dtr {{cores "all"}} {
    global traceBaseAddrArray
    global te_control_offset
    global ev_control_offset
    global te_impl_offset
    global te_sinkbase_offset
    global te_sinkbasehigh_offset
    global te_sinklimit_offset
    global te_sinkwp_offset
    global te_sinkrp_offset
    global te_sinkdata_offset

    global xti_control_offset
    global xto_control_offset
    global wp_control_offset
    global itc_traceenable_offset
    global itc_trigenable_offset
    global ts_control_offset

    global num_funnels
    global has_event

    if {$num_funnels > 0} {
        set coreList [parseCoreFunnelList $cores]
    } else {
        set coreList [parseCoreList $cores]
    }

    if {$coreList == "error"} {
        echo "Error: Usage: dtr [corelist]"
        return "error"
    }

    foreach core $coreList {
        set tectrl [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]
        set teimpl [word [expr {$traceBaseAddrArray($core) + $te_impl_offset}]]
        set tesinkblo [word [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}]]
        set tesinkbhi [word [expr {$traceBaseAddrArray($core) + $te_sinkbasehigh_offset}]]
        set tesinklim [word [expr {$traceBaseAddrArray($core) + $te_sinklimit_offset}]]
        set tesinkwp [word [expr {$traceBaseAddrArray($core) + $te_sinkwp_offset}]]
        set tesinkrp [word [expr {$traceBaseAddrArray($core) + $te_sinkrp_offset}]]
        set xtictrl [word [expr {$traceBaseAddrArray($core) + $xti_control_offset}]]
        set xtoctrl [word [expr {$traceBaseAddrArray($core) + $xto_control_offset}]]
        set wpctrl [word [expr {$traceBaseAddrArray($core) + $wp_control_offset}]]
        set teitctraceen [word [expr {$traceBaseAddrArray($core) + $itc_traceenable_offset}]]
        set teitctrigen [word [expr {$traceBaseAddrArray($core) + $itc_trigenable_offset}]]
        if {$core == "funnel"} {
            echo [format "dtr: core\[$core\] ctrl=%08x impl=%08x base=%x%08x limit=%x wp=%x rp=%x" \
             $tectrl $teimpl $tesinkbhi $tesinkblo $tesinklim $tesinkwp $tesinkrp]
        } else {
            echo [format "dtr: core\[$core\] ctrl=%08x impl=%08x base=%x%08x limit=%x wp=%x rp=%x xti=%x xto=%x wpctrl=%x itctrig=%x itctrace=%x" \
             $tectrl $teimpl $tesinkbhi $tesinkblo $tesinklim $tesinkwp $tesinkrp $xtictrl $xtoctrl $wpctrl $teitctrigen $teitctraceen]
        }
    }
}

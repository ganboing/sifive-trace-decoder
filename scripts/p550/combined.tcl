set sinkTypes { 4 5 6 7 8 }

proc sinkTypesStr { st } {
    switch $st {
       4       { return "SRAM"   }
       5       { return "ATB"    }
       6       { return "PIB"    }
       7       { return "SBA"    }
       8       { return "FUNNEL" }
       default { return "reserved" }
    }
}

proc sinkTypesParse { type } {
    switch $type {
       "SRAM"   { return 4 }
       "ATB"    { return 5 }
       "PIB"    { return 6 }
       "SBA"    { return 7 }
       "FUNNEL" { return 8 }
       default  { return 0 }
    }
}

# ite = [i]s [t]race [e]nabled
proc getTraceEnabled {} {
    global te_control_offset
    global traceBaseAddresses
    global traceBaseAddrArray
    global num_funnels

    if {$num_funnels > 0} {
        set tracectl [word [expr {$traceBaseAddrArray(funnel) + $te_control_offset}]]
        if {($tracectl & 0x2) == 0} {
            echo "getTraceEnabled: funnel is not enabled"
            return 1
        }
    }

    foreach baseAddress $traceBaseAddresses {
        set tracectl [word [expr {$baseAddress + $te_control_offset}]]
        if {($tracectl & 0x6) != 0} {
            echo "getTraceEnabled: core@$baseAddress is not enabled"
            return 1
        }
    }

    return 0
}

proc probeSinkBuffer {} {
    global te_sinkbase_offset
    global te_sinkbasehigh_offset
    global te_sinklimit_offset
    global traceBaseAddrArray
    global num_funnels

    if {$num_funnels > 0} {
        set addrlo [probeReg [expr {$traceBaseAddrArray(funnel) + $te_sinkbase_offset}]]
        set addrhi [probeReg [expr {$traceBaseAddrArray(funnel) + $te_sinkbasehigh_offset}]]
        set limit [probeReg [expr {$traceBaseAddrArray(funnel) + $te_sinklimit_offset}]]
        echo [format "probeSinkBuffer: funnel sinkbase=%08x%08x limit=%08x" \
              $addrhi $addrlo $limit]
    }

    set addrlo [probeReg [expr {$traceBaseAddrArray(0) + $te_sinkbase_offset}]]
    set addrhi [probeReg [expr {$traceBaseAddrArray(0) + $te_sinkbasehigh_offset}]]
    set limit [probeReg [expr {$traceBaseAddrArray(0) + $te_sinklimit_offset}]]
    echo [format "probeSinkBuffer: encoder sinkbase=%08x%08x limit=%08x" \
          $addrhi $addrlo $limit]
}

proc _txProbeMemLimitAlignBits { addr } {
    global te_sinklimit_offset

    set limit [probeReg [expr {$addr + $te_sinklimit_offset}]]
    # Assume 11...10...0
    set i 0
    for {} {($limit & (1 << $i)) == 0} {incr i} {}
    echo [format "_txProbeMemLimitAlignBits(%x): align=%d" $addr $i]
    return $i
}

proc txProbeMemLimitAlignBits { core } {
    global traceBaseAddrArray

    return [_txProbeMemLimitAlignBits $traceBaseAddrArray($core)]
}

proc _txProbeMemLimitMaxBits { addr } {
    global te_sinklimit_offset

    set limit [probeReg [expr {$addr + $te_sinklimit_offset}]]
    # Assume 11...10...0
    set i [_txProbeMemLimitAlignBits $addr]
    for {} {($limit & (1 << $i)) != 0} {incr i} {}
    echo [format "_txProbeMemLimitMaxBits(%x): maxbit=%d" $addr $i]
    return $i
}

proc txProbeMemLimitMaxBits { core } {
    global traceBaseAddrArray

    return [_txProbeMemLimitMaxBits $traceBaseAddrArray($core)]
}

proc _txProbeMemBaseAlignBits { addr } {
    global te_sinkbase_offset

    set baselo [probeReg [expr {$addr + $te_sinkbase_offset}]]
    # Assume 11...10...0
    for {set i 0} {($baselo & (1 << $i)) == 0} {incr i} {}
    echo [format "_txProbeMemBaseAlignBits(%x): align=%d" $addr $i]
    return $i
}

proc txProbeMemBaseAlignBits { core } {
    global traceBaseAddrArray

    return [_txProbeMemBaseAlignBits $traceBaseAddrArray($core)]
}

proc _txProbeMemBaseMaxBits { addr } {
    global te_sinkbase_offset
    global te_sinkbasehigh_offset

    set baselo [probeReg [expr {$addr + $te_sinkbase_offset}]]
    # Assume 11...10...0
    set basehi [probeReg [expr {$addr + $te_sinkbasehigh_offset}]]
    set base64 [expr {($basehi << 32) | $baselo}]
    set i [_txProbeMemBaseAlignBits $addr]
    for {} {($base64 & (1 << $i)) != 0} {incr i} {}
    echo [format "_txProbeMemBaseMaxBits(%x): maxbit=%d" $addr $i]
    return $i
}

proc txProbeMemBaseMaxBits { core } {
    global traceBaseAddrArray

    return [_txProbeMemBaseMaxBits $traceBaseAddrArray($core)]
}

proc _txProbeSRAMSinkBits { addr } {
    global te_sinkwp_offset

    set wp [probeReg [expr {$addr + $te_sinkwp_offset}]]
    set i 0
    for {} {($wp & (1 << $i)) == 0} {incr i} {}
    set align $i
    for {} {($wp & (1 << $i)) != 0} {incr i} {}
    set maxbit $i
    echo [format "_txProbeSRAMSinkBits(%x) align=%d maxbit=%d" $addr $align $maxbit]
    return $maxbit
}

proc txProbeSRAMSinkBits { core } {
    global traceBaseAddrArray

    return [_txProbeSRAMSinkBits $traceBaseAddrArray($core)]
}

proc _txGetSinkType { addr } {
    global te_control_offset

    set t [word [expr {$addr + $te_control_offset}]]
    set t [expr {($t >> 28) & 0x0f}]
    return [sinkTypesStr $t]
}

proc txGetSinkType {core} {
    global traceBaseAddrArray

    return [_txGetSinkType $traceBaseAddrArray($core)]
}

proc _txSetSinkType { addr sink } {
    global te_impl_offset
    global te_control_offset

    # make sure sink is supported by funnel
    set sinkbit [sinkTypesParse $sink]
    if {$sinkbit == 0} {
        echo [format  "txSetSink(%x): unknown sink $sink" $addr]
        return 1
    }

    set impl [word [expr {$addr + $te_impl_offset}]]
    echo [format "txSetSink(%x): sink=%d impl=%x" $addr $sinkbit $impl]

    if {($impl & (1 << $sinkbit)) == 0} {
        echo [format  "txSetSink(%x): sink $sink is not supported" $addr]
        return 1
    }

    echo [format "txSetSink(%x): enabling sink $sink" $addr]
    set control [word [expr {$addr + $te_control_offset}]]
    set control [expr {$control & ~(0x0f << 28)}]
    set control [expr {$control | ($sinkbit << 28)}]
    mww [expr {$addr + $te_control_offset}] $control

    return 0
}

proc txSetSinkType {core sink} {
    global traceBaseAddrArray

    return [_txSetSinkType $traceBaseAddrArray($core) $sink]
}

proc txGetSinkError {core} {
    global traceBaseAddrArray
    global te_control_offset

    set tracectl [word [expr {$traceBaseAddrArray($core) + $te_control_offset}]]

    if {(($tracectl >> 27) & 1) != 0} {
        return "ERROR"
    }

    return "OK"
}

proc txClearSinkError {core} {
    global traceBaseAddrArray
    global te_control_offset

    set reg [expr {$traceBaseAddrArray($core) + $te_control_offset}]
    set t [word $reg]
    set t [expr $t | (1 << 27)]
    mww $reg $t
}

proc txGetBufferSize { core } {
    global traceBaseAddrArray
    global te_sinklimit_offset
    global te_sinkbase_offset

    set limit [word [expr {$traceBaseAddrArray($core) + $te_sinklimit_offset}]]
    set baselo [word [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}]]
    return [expr {$limit - $baselo}]
}

proc txGetBufferBase { core } {
    global traceBaseAddrArray
    global te_sinkbase_offset
    global te_sinkbasehigh_offset

    set baselo [word [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}]]
    set basehi [word [expr {$traceBaseAddrArray($core) + $te_sinkbasehigh_offset}]]
    return [expr {$baselo | ($basehi << 32)}]
}

proc txGetBufferCurrentPtr {core} {
    global traceBaseAddrArray
    global te_sinkwp_offset
    global te_sinkbasehigh_offset

    set tracewp [word [expr {$traceBaseAddrArray($core) + $te_sinkwp_offset}]]
    set tracewp [expr {$tracewp & ~3}]
    set tracebasehi [word [expr {$traceBaseAddrArray($core) + $te_sinkbasehigh_offset}]]
    return [expr {$tracewp | ($tracebasehi << 32)}]
}

proc txGetBufferWrapped {core} {
    global traceBaseAddrArray
    global te_sinkwp_offset

    set tracewp [word [expr {$traceBaseAddrArray($core) + $te_sinkwp_offset}]]
    return [expr {$tracewp & 1}]
}

proc txResetBufferPtr {core} {
    global traceBaseAddrArray
    global te_sinkrp_offset
    global te_sinkwp_offset
    global te_sinkbase_offset

    set s [txGetSinkType $core]
    switch [string toupper $s] {
        "SRAM" {
            mww [expr {$traceBaseAddrArray($core) + $te_sinkrp_offset}] 0
            mww [expr {$traceBaseAddrArray($core) + $te_sinkwp_offset}] 0
        }
        "SBA" {
            set t [word [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}]]
            mww [expr {$traceBaseAddrArray($core) + $te_sinkwp_offset}] $t
            mww [expr {$traceBaseAddrArray($core) + $te_sinkrp_offset}] $t
        }
    }
}

proc txSetReadPtr {core ptr} {
    global traceBaseAddrArray
    global te_sinkrp_offset

    mww [expr {$traceBaseAddrArray($core) + $te_sinkrp_offset}] $ptr
}

proc txSetSink {core type {base ""} {size ""}} {
    global traceBaseAddrArray
    global te_sinkbase_offset
    global te_sinkbasehigh_offset
    global te_sinklimit_offset

    set rc [txSetSinkType $core $type]
    if {$rc != 0} {
        echo "txSetSink\[$core\]: failed to set sink to $type"
    }

    if {[string compare -nocase $type "sba"] != 0} {
        return 0
    }

    echo "txSetSink\[$core\]: setting SBA sink to $base+$size"

    set maxbits [txProbeMemBaseMaxBits $core]
    set align [txProbeMemBaseAlignBits $core]
    set mbase [expr {$base}]
    set mbasep [expr {$mbase & ((1 << $maxbits) - 1)}]
    if {$mbase != $mbasep} {
        echo "txSetSink\[$core\]: SBA sink base too high, maxbits=$maxbits, mbasep=[format %x $mbasep]"
        return 1
    }
    set mbasep [expr {$mbase & ~((1 << $align) - 1)}]
    if {$mbase != $mbasep} {
        echo "txSetSink\[$core\]: SBA sink base misaligned, alignbits=$align, mbasep=[format %x $mbasep]"
        return 1
    }

    set maxbits [txProbeMemLimitMaxBits $core]
    set align [txProbeMemLimitAlignBits $core]
    set mlimit [expr {$size}]
    set mlimitp [expr {$mlimit & ((1 << $maxbits) - 1)}]
    if {$mlimit != $mlimitp} {
        echo "txSetSink\[$core\]: SBA sink size too large, maxbits=$maxbits, mlimitp=[format %x $mlimitp]"
        return 1
    }
    set mlimitp [expr {$mlimit & ~((1 << $align) - 1)}]
    if {$mlimit != $mlimitp} {
        echo "txSetSink\[$core\]: SBA sink size misaligned, alignbits=$align, mlimitp=[format %x $mlimitp]"
        return 1
    }

    mww [expr {$traceBaseAddrArray($core) + $te_sinkbase_offset}] [expr {$mbase & 0xffffffff}]
    mww [expr {$traceBaseAddrArray($core) + $te_sinkbasehigh_offset}] [expr {$mbase >> 32}]
    mww [expr {$traceBaseAddrArray($core) + $te_sinklimit_offset}] $mlimit

    txResetBufferPtr $core

    return 0
}

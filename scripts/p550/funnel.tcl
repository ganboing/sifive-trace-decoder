# funnel routines for multi-core. Not indented to be called directly!

proc tfHasFunnelSink { addr } {
    global te_impl_offset

    set impl [word [expr {$addr + $te_impl_offset}]]
    if {($impl & (1 << 8))} {
        return 1
    }

    return 0
}

proc tfEnable { addr } {
    global te_control_offset

    set reg [expr {$addr + $te_control_offset}]
    set t [word $reg]
    # Enable tfEnable
    set t [expr $t | 0x2]
    mww $reg $t
}

proc tfDisable { addr } {
    global te_control_offset

    set reg [expr {$addr + $te_control_offset}]
    set t [word $reg]
    # Disable tfEnable | tfTracing
    set t [expr $t & ~0x6]
    mww $reg $t
}

proc tfStop { addr } {
    global te_control_offset

    set reg [expr {$addr + $te_control_offset}]
    set t [word $reg]
    # Disable tfTracing
    set t [expr $t & ~0x4]
    mww $reg $t
}

proc tfStart { addr } {
    global te_control_offset

    set reg [expr {$addr + $te_control_offset}]
    set t [word $reg]
    # Enable tfEnable | tfTracing
    set t [expr $t | 0x6]
    mww $reg $t
}


proc tfWaitFlush { addr } {
    global te_control_offset

    set reg [expr {$addr + $te_control_offset}]
    set t [word $reg]
    while {($t & 0x8) == 0} {
        set t [word $reg]
    }
}

# end multi-core funnel routines

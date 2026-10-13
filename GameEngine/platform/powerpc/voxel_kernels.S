    .text
    .align 2
    .globl gc_engine_dot4
    .type gc_engine_dot4, @function
gc_engine_dot4:
    lfs 0, 0(3)
    lfs 1, 0(4)
    lfs 2, 4(3)
    lfs 3, 4(4)
    lfs 4, 8(3)
    lfs 5, 8(4)
    lfs 6, 12(3)
    lfs 7, 12(4)
    fmuls 8, 0, 1
    fmadds 8, 2, 3, 8
    fmadds 8, 4, 5, 8
    fmadds 1, 6, 7, 8
    blr
    .size gc_engine_dot4, .-gc_engine_dot4

    .align 2
    .globl gc_engine_clear_depth
    .type gc_engine_clear_depth, @function
gc_engine_clear_depth:
    cmpwi 4, 0
    beqlr
    mtctr 4
    fmr 0, 1
1:
    stfs 0, 0(3)
    addi 3, 3, 4
    bdnz 1b
    blr
    .size gc_engine_clear_depth, .-gc_engine_clear_depth

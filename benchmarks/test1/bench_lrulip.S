# This is a testcase in which LRULIP has better performance than LRU

.data
argument:
    # each row has 16 words (16 * 8)
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    .word 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
.text
    # for i = 0; i < 1000; i++
    # lw a[0], a[32], a[64], a[96], a[128]
    la t0, argument     # t0: array a
    li t1, 1000         # t1: iter
    mv t2, zero            # t2: i
loop:
    addi t2, t2, 1
    lw t3, 0(t0)
    lw t3, 32(t0)
    lw t3, 64(t0)
    lw t3, 96(t0)
    lw t3, 128(t0)
    bne t2, t1, loop    # if i != iter then target
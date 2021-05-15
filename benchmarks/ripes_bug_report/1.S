# This is a testcase in which LRU has better performance than LRULIP

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
    # lw a[0], a[32], a[64]
    # for i = 0; i < 1000; i++
    # lw a[128], a[96]
    la t0, argument     # t0: array a
    li t1, 4000         # t1: iter
    mv t2, zero         # t2: i
    mv t4, t0
loop:
    addi t2, t2, 4
    addi t4, t4, 128
    sw t3, 0(t4)
    lw t3, 0(t4)
    bne t2, t1, loop    # if i != iter then target
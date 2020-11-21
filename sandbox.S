// vim: syntax=arm64asm
#include "helper.S"

// QR function:

// v0 += v1
// v3 = (v3 ^ v0) <<< 16
// v2 += v3
// v1 = (v1 ^ v2) <<< 12
// v0 += v1
// v3 = (v3 ^ v0) <<< 8
// v2 += v3
// v1 = (v1 ^ v2) <<< 7

// What a double round looks like:

// QR 0, 4, 8, 12
// QR 1, 5, 9, 13
// QR 2, 6, 10, 14
// QR 3, 7, 11, 15

// QR 0, 5, 10, 15
// QR 1, 6, 11, 12
// QR 2, 7, 8, 13
// QR 3, 4, 9, 14

// Each quarter-round only operates on its own values, this means we can do 4
// quarter-rounds at a time (column-wise).

// The 2nd, 3rd, and 4th columns can be shifted by 32, 64, and 96 bits
// respectively and the same column-wise 4x quarter-round function run.

function chacha20_block_neon, export=1
    ld1 {v0.4S-v3.4S}, [x0]
    ld1 {v4.4S-v7.4S}, [x0]

    // 8x quarter rounds, x10
    mov x1, #10
.double_round:
    add  v0.4S,   v0.4S,   v1.4S
    eor  v16.16B, v3.16B, v0.16B
    ushr v3.4S,  v16.4S, #16
    sli  v3.4S,  v16.4S, #16

    add  v2.4S,   v2.4S,   v3.4S
    eor  v16.16B, v1.16B, v2.16B
    ushr v1.4S,   v16.4S, #20
    sli  v1.4S,   v16.4S, #12

    add  v0.4S,   v0.4S,  v1.4S
    eor  v16.16B, v3.16B, v0.16B
    ushr v3.4S,   v16.4S, #24
    sli  v3.4S,   v16.4S, #8

    add  v2.4S,   v2.4S,  v3.4S
    eor  v16.16B, v1.16B, v2.16B
    ushr v1.4S,   v16.4S, #25
    sli v1.4S,    v16.4S, #7

    // shift
    ext v1.16b, v1.16b, v1.16b, #4
    ext v2.16b, v2.16b, v2.16b, #8
    ext v3.16b, v3.16b, v3.16b, #12

    add  v0.4S,   v0.4S,   v1.4S
    eor  v16.16B, v3.16B, v0.16B
    ushr v3.4S,  v16.4S, #16
    sli  v3.4S,  v16.4S, #16

    add  v2.4S,   v2.4S,   v3.4S
    eor  v16.16B, v1.16B, v2.16B
    ushr v1.4S,   v16.4S, #20
    sli  v1.4S,   v16.4S, #12

    add  v0.4S,   v0.4S,  v1.4S
    eor  v16.16B, v3.16B, v0.16B
    ushr v3.4S,   v16.4S, #24
    sli  v3.4S,   v16.4S, #8

    add  v2.4S,   v2.4S,  v3.4S
    eor  v16.16B, v1.16B, v2.16B
    ushr v1.4S,   v16.4S, #25
    sli v1.4S,    v16.4S, #7

    // shift back
    ext v1.16b, v1.16b, v1.16b, #12
    ext v2.16b, v2.16b, v2.16b, #8
    ext v3.16b, v3.16b, v3.16b, #4

    subs x1, x1, #1
    bne .double_round

    add v0.4S, v0.4S, v4.4S
    add v1.4S, v1.4S, v5.4S
    add v2.4S, v2.4S, v6.4S
    add v3.4S, v3.4S, v7.4S
    st1 {v0.4S-v3.4S}, [x0]

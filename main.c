#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void dump_creg(uint32_t x[16], char *comment)
{
    printf("%s:\n"
           "v0.S {%10u, %10u, %10u, %10u }\n"
           "v1.S {%10u, %10u, %10u, %10u }\n"
           "v2.S {%10u, %10u, %10u, %10u }\n"
           "v3.S {%10u, %10u, %10u, %10u }\n",
           comment,
           x[0], x[1], x[2], x[3],
           x[4], x[5], x[6], x[7],
           x[8], x[9], x[10], x[11],
           x[12], x[13], x[14], x[15]);
}

static inline uint32_t rotl(uint32_t x, uint32_t n)
{
    return ((x << n) | (x >> (-n & 31)));
}

void chacha20_quarter_round(uint32_t x[16], uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    // dump_creg(x, "pre op");
    x[a] += x[b];
    x[d] = rotl(x[d] ^ x[a], 16);
    // dump_creg(x, "first op");
    x[c] += x[d];
    x[b] = rotl(x[b] ^ x[c], 12);
    // dump_creg(x, "second op");
    x[a] += x[b];
    x[d] = rotl(x[d] ^ x[a], 8);
    // dump_creg(x, "third op");
    x[c] += x[d];
    x[b] = rotl(x[b] ^ x[c], 7);
    // dump_creg(x, "fourth op");
}

void chacha20_setup(uint32_t s[16], const uint8_t k[32], const uint8_t n[12], uint32_t c)
{
    s[0] = 0x61707865;
    s[1] = 0x3320646e;
    s[2] = 0x79622d32;
    s[3] = 0x6b206574;
#define load(x, i) x[i + 3] << 24 | x[i + 2] << 16 | x[i + 1] << 8 | x[i]
    for (int i = 0; i < 32; i += 4)
        s[i / 4 + 4] = load(k, i);
    s[12] = c;
    for (int i = 0; i < 12; i += 4)
        s[i / 4 + 13] = load(n, i);
#undef load
}

void chacha20_block_neon(uint32_t s[16]);
void chacha20_block_c(uint32_t s[16])
{
    uint32_t state[16] = {0};

    for (int i = 0; i < 16; i++)
        state[i] = s[i];

    for (int i = 0; i < 10; i++) {
        chacha20_quarter_round(state, 0, 4, 8, 12);
        chacha20_quarter_round(state, 1, 5, 9, 13);
        chacha20_quarter_round(state, 2, 6, 10, 14);
        chacha20_quarter_round(state, 3, 7, 11, 15);
        chacha20_quarter_round(state, 0, 5, 10, 15);
        chacha20_quarter_round(state, 1, 6, 11, 12);
        chacha20_quarter_round(state, 2, 7, 8, 13);
        chacha20_quarter_round(state, 3, 4, 9, 14);
    }
    for (int i = 0; i < 16; i++)
        s[i] += state[i];
}

int chacha20_random(void (*block_func)(uint32_t *), uint8_t k[32], uint8_t n[12], uint8_t *out, size_t bytes)
{
    uint32_t state[16] = {
        // this is wiped by chacha20_setup()
        1634760805,  857760878, 2036477234, 1797285236,
        2058694467, 2612546140, 1163293007,  104192297,
        2378830003, 1681753675, 1089841512, 2040389686,
                 1, 1811173903, 3943148700, 1194351902
    };

    chacha20_setup(state, k, n, 1);

    while (bytes > 0) {
        int len = (bytes > 64) ? 64 : bytes;
        block_func(state);
        if (out)
          memcpy(out, state, len);
        bytes -= len;
    }

    return 0;
}

int main(void)
{
    clock_t time_asm, time_c;
    double msec_asm, msec_c;

    uint8_t k[32] = {0};
    uint8_t n[12] = {0};
    uint8_t block_c[64] = {0};
    uint8_t block_neon[64] = {0};

    srand(time(NULL));
    for (int i = 0; i < 32; i++)
        k[i] = rand();
    for (int i = 0; i < 12; i++)
        n[i] = rand();

    chacha20_random(chacha20_block_c, k, n, block_c, 64);
    chacha20_random(chacha20_block_neon, k, n, block_neon, 64);

    if (memcmp(block_c, block_neon, 64)) {
        fprintf(stderr, "Bad ASM!\n");
        return 1;
    }

    // bench 16384 blocks

    time_c = clock();
    chacha20_random(chacha20_block_c, k, n, NULL, 64 * 16384);
    time_c = clock() - time_c;

    time_asm = clock();
    chacha20_random(chacha20_block_neon, k, n, NULL, 64 * 16384);
    time_asm = clock() - time_asm;

    msec_c = ((double)time_c) / (CLOCKS_PER_SEC / 1000);
    msec_asm = ((double)time_asm) / (CLOCKS_PER_SEC / 1000);

    printf("c:    %f msec (%ld clocks)\n"
           "neon: %f msec (%ld clocks)\n"
           "That's a %.2fx speedup!\n",
           msec_c, time_c,
           msec_asm, time_asm,
           ((double)time_c / (double)time_asm));

    return 0;
}

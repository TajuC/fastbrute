#ifndef FASTBRUTE_BRUTE_H
#define FASTBRUTE_BRUTE_H

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define FB_STOP_LOAD(p)     __atomic_load_n((p), __ATOMIC_RELAXED)
#define FB_STOP_STORE(p, v) __atomic_store_n((p), (v), __ATOMIC_RELAXED)
#else
#include <intrin.h>
#define FB_STOP_LOAD(p)     _InterlockedOr((volatile long *)(p), 0)
#define FB_STOP_STORE(p, v) ((void)_InterlockedExchange((volatile long *)(p), (long)(v)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FB_ALPHABET_SIZE 62
#define FB_SECRET_LEN 5
#define FB_MAX_BLOCKS 2
#define FB_INNER_MAX_VAR 3
#define FB_OUTER_MAX_VAR 9

typedef struct {
    int block;
    int wib;
} fb_word_ref;

typedef struct {
    uint32_t mid[4];
    size_t tail_len;
    int nb_inner;
    int nb_outer;
    uint8_t inner_tmpl[128];
    uint8_t outer_tmpl[128];
    uint8_t expected[16];
    uint32_t expected_a;

    uint32_t wv_inner[FB_MAX_BLOCKS][64];
    uint32_t wv_outer[FB_MAX_BLOCKS][64];

    int inner_nvar;
    fb_word_ref inner_var[FB_INNER_MAX_VAR];
    int inner_map_var[FB_SECRET_LEN];
    int inner_map_shift[FB_SECRET_LEN];

    int outer_r;
    int outer_nvar;
    fb_word_ref outer_var[FB_OUTER_MAX_VAR];
    int outer_var_m[FB_OUTER_MAX_VAR];
} fb_ctx;

extern const uint8_t FB_ALPHABET[FB_ALPHABET_SIZE];
extern const uint32_t FB_K[64];
extern const uint8_t FB_SH[64];
extern const uint8_t FB_G[64];

void fb_setup(fb_ctx *ctx, const uint8_t *challenge, size_t clen, const uint8_t expected[16]);
void fb_hex16(const uint32_t digest[4], uint8_t out[32]);
void fb_hash_one(const fb_ctx *ctx, const uint8_t cand[FB_SECRET_LEN], uint8_t out[16]);

long long fb_scan_scalar(const fb_ctx *ctx, long long lo, long long hi, volatile int *stop);
long long fb_scan_avx2(const fb_ctx *ctx, long long lo, long long hi, volatile int *stop);
long long fb_scan_avx512(const fb_ctx *ctx, long long lo, long long hi, volatile int *stop);

void fb_group8_avx2(const fb_ctx *ctx, long long base, uint8_t out[8][16]);
void fb_group16_avx512(const fb_ctx *ctx, long long base, uint8_t out[16][16]);

int fb_has_avx2(void);
int fb_has_avx512(void);

#ifdef __cplusplus
}
#endif

#endif

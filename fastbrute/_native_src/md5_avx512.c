#include "brute.h"
#include <string.h>
#include <immintrin.h>

static inline __m512i nib_ascii16(__m512i n) {
    __m512i base = _mm512_add_epi8(n, _mm512_set1_epi8(0x30));
    __mmask64 k = _mm512_cmpgt_epi8_mask(n, _mm512_set1_epi8(9));
    return _mm512_mask_add_epi8(base, k, base, _mm512_set1_epi8(0x27));
}

static inline void hex_expand16(const __m512i st[4], __m512i H[8]) {
    const __m512i m4 = _mm512_set1_epi32(0xf);
    for (int k = 0; k < 4; k++) {
        __m512i W = st[k];
        __m512i nlo = _mm512_or_si512(
            _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(W, 4), m4),
                            _mm512_slli_epi32(_mm512_and_si512(W, m4), 8)),
            _mm512_or_si512(_mm512_slli_epi32(_mm512_and_si512(_mm512_srli_epi32(W, 12), m4), 16),
                            _mm512_slli_epi32(_mm512_and_si512(_mm512_srli_epi32(W, 8), m4), 24)));
        __m512i nhi = _mm512_or_si512(
            _mm512_or_si512(_mm512_and_si512(_mm512_srli_epi32(W, 20), m4),
                            _mm512_slli_epi32(_mm512_and_si512(_mm512_srli_epi32(W, 16), m4), 8)),
            _mm512_or_si512(_mm512_slli_epi32(_mm512_and_si512(_mm512_srli_epi32(W, 28), m4), 16),
                            _mm512_slli_epi32(_mm512_and_si512(_mm512_srli_epi32(W, 24), m4), 24)));
        H[2 * k] = nib_ascii16(nlo);
        H[2 * k + 1] = nib_ascii16(nhi);
    }
}

#define STEP(TT, A, B, C, D, WI, GI, S) { \
    __m512i w = _mm512_add_epi32(wvv[WI], vword[GI]); \
    __m512i f = _mm512_ternarylogic_epi32(B, C, D, TT); \
    __m512i t = _mm512_add_epi32(_mm512_add_epi32(A, f), w); \
    A = _mm512_add_epi32(B, _mm512_rol_epi32(t, S)); }

static void md5_block16_folded(__m512i st[4], const __m512i wvv[64], const __m512i vword[16]) {
    __m512i a = st[0], b = st[1], c = st[2], d = st[3];

    STEP(0xCA, a,b,c,d, 0, 0, 7);
    STEP(0xCA, d,a,b,c, 1, 1,12);
    STEP(0xCA, c,d,a,b, 2, 2,17);
    STEP(0xCA, b,c,d,a, 3, 3,22);
    STEP(0xCA, a,b,c,d, 4, 4, 7);
    STEP(0xCA, d,a,b,c, 5, 5,12);
    STEP(0xCA, c,d,a,b, 6, 6,17);
    STEP(0xCA, b,c,d,a, 7, 7,22);
    STEP(0xCA, a,b,c,d, 8, 8, 7);
    STEP(0xCA, d,a,b,c, 9, 9,12);
    STEP(0xCA, c,d,a,b, 10,10,17);
    STEP(0xCA, b,c,d,a, 11,11,22);
    STEP(0xCA, a,b,c,d, 12,12, 7);
    STEP(0xCA, d,a,b,c, 13,13,12);
    STEP(0xCA, c,d,a,b, 14,14,17);
    STEP(0xCA, b,c,d,a, 15,15,22);

    STEP(0xE4, a,b,c,d, 16, 1, 5);
    STEP(0xE4, d,a,b,c, 17, 6, 9);
    STEP(0xE4, c,d,a,b, 18,11,14);
    STEP(0xE4, b,c,d,a, 19, 0,20);
    STEP(0xE4, a,b,c,d, 20, 5, 5);
    STEP(0xE4, d,a,b,c, 21,10, 9);
    STEP(0xE4, c,d,a,b, 22,15,14);
    STEP(0xE4, b,c,d,a, 23, 4,20);
    STEP(0xE4, a,b,c,d, 24, 9, 5);
    STEP(0xE4, d,a,b,c, 25,14, 9);
    STEP(0xE4, c,d,a,b, 26, 3,14);
    STEP(0xE4, b,c,d,a, 27, 8,20);
    STEP(0xE4, a,b,c,d, 28,13, 5);
    STEP(0xE4, d,a,b,c, 29, 2, 9);
    STEP(0xE4, c,d,a,b, 30, 7,14);
    STEP(0xE4, b,c,d,a, 31,12,20);

    STEP(0x96, a,b,c,d, 32, 5, 4);
    STEP(0x96, d,a,b,c, 33, 8,11);
    STEP(0x96, c,d,a,b, 34,11,16);
    STEP(0x96, b,c,d,a, 35,14,23);
    STEP(0x96, a,b,c,d, 36, 1, 4);
    STEP(0x96, d,a,b,c, 37, 4,11);
    STEP(0x96, c,d,a,b, 38, 7,16);
    STEP(0x96, b,c,d,a, 39,10,23);
    STEP(0x96, a,b,c,d, 40,13, 4);
    STEP(0x96, d,a,b,c, 41, 0,11);
    STEP(0x96, c,d,a,b, 42, 3,16);
    STEP(0x96, b,c,d,a, 43, 6,23);
    STEP(0x96, a,b,c,d, 44, 9, 4);
    STEP(0x96, d,a,b,c, 45,12,11);
    STEP(0x96, c,d,a,b, 46,15,16);
    STEP(0x96, b,c,d,a, 47, 2,23);

    STEP(0x39, a,b,c,d, 48, 0, 6);
    STEP(0x39, d,a,b,c, 49, 7,10);
    STEP(0x39, c,d,a,b, 50,14,15);
    STEP(0x39, b,c,d,a, 51, 5,21);
    STEP(0x39, a,b,c,d, 52,12, 6);
    STEP(0x39, d,a,b,c, 53, 3,10);
    STEP(0x39, c,d,a,b, 54,10,15);
    STEP(0x39, b,c,d,a, 55, 1,21);
    STEP(0x39, a,b,c,d, 56, 8, 6);
    STEP(0x39, d,a,b,c, 57,15,10);
    STEP(0x39, c,d,a,b, 58, 6,15);
    STEP(0x39, b,c,d,a, 59,13,21);
    STEP(0x39, a,b,c,d, 60, 4, 6);
    STEP(0x39, d,a,b,c, 61,11,10);
    STEP(0x39, c,d,a,b, 62, 2,15);
    STEP(0x39, b,c,d,a, 63, 9,21);

    st[0] = _mm512_add_epi32(st[0], a);
    st[1] = _mm512_add_epi32(st[1], b);
    st[2] = _mm512_add_epi32(st[2], c);
    st[3] = _mm512_add_epi32(st[3], d);
}

#define STEP2(TT, A1,B1,C1,D1, A2,B2,C2,D2, WI, GI, S) { \
    __m512i w1 = _mm512_add_epi32(wvv[WI], vword1[GI]); \
    __m512i w2 = _mm512_add_epi32(wvv[WI], vword2[GI]); \
    __m512i f1 = _mm512_ternarylogic_epi32(B1, C1, D1, TT); \
    __m512i f2 = _mm512_ternarylogic_epi32(B2, C2, D2, TT); \
    __m512i t1 = _mm512_add_epi32(_mm512_add_epi32(A1, f1), w1); \
    __m512i t2 = _mm512_add_epi32(_mm512_add_epi32(A2, f2), w2); \
    A1 = _mm512_add_epi32(B1, _mm512_rol_epi32(t1, S)); \
    A2 = _mm512_add_epi32(B2, _mm512_rol_epi32(t2, S)); }

static void md5_block16x2_folded(__m512i st1[4], __m512i st2[4], const __m512i wvv[64],
                                 const __m512i vword1[16], const __m512i vword2[16]) {
    __m512i a1 = st1[0], b1 = st1[1], c1 = st1[2], d1 = st1[3];
    __m512i a2 = st2[0], b2 = st2[1], c2 = st2[2], d2 = st2[3];

    STEP2(0xCA, a1,b1,c1,d1, a2,b2,c2,d2, 0, 0, 7);
    STEP2(0xCA, d1,a1,b1,c1, d2,a2,b2,c2, 1, 1,12);
    STEP2(0xCA, c1,d1,a1,b1, c2,d2,a2,b2, 2, 2,17);
    STEP2(0xCA, b1,c1,d1,a1, b2,c2,d2,a2, 3, 3,22);
    STEP2(0xCA, a1,b1,c1,d1, a2,b2,c2,d2, 4, 4, 7);
    STEP2(0xCA, d1,a1,b1,c1, d2,a2,b2,c2, 5, 5,12);
    STEP2(0xCA, c1,d1,a1,b1, c2,d2,a2,b2, 6, 6,17);
    STEP2(0xCA, b1,c1,d1,a1, b2,c2,d2,a2, 7, 7,22);
    STEP2(0xCA, a1,b1,c1,d1, a2,b2,c2,d2, 8, 8, 7);
    STEP2(0xCA, d1,a1,b1,c1, d2,a2,b2,c2, 9, 9,12);
    STEP2(0xCA, c1,d1,a1,b1, c2,d2,a2,b2, 10,10,17);
    STEP2(0xCA, b1,c1,d1,a1, b2,c2,d2,a2, 11,11,22);
    STEP2(0xCA, a1,b1,c1,d1, a2,b2,c2,d2, 12,12, 7);
    STEP2(0xCA, d1,a1,b1,c1, d2,a2,b2,c2, 13,13,12);
    STEP2(0xCA, c1,d1,a1,b1, c2,d2,a2,b2, 14,14,17);
    STEP2(0xCA, b1,c1,d1,a1, b2,c2,d2,a2, 15,15,22);

    STEP2(0xE4, a1,b1,c1,d1, a2,b2,c2,d2, 16, 1, 5);
    STEP2(0xE4, d1,a1,b1,c1, d2,a2,b2,c2, 17, 6, 9);
    STEP2(0xE4, c1,d1,a1,b1, c2,d2,a2,b2, 18,11,14);
    STEP2(0xE4, b1,c1,d1,a1, b2,c2,d2,a2, 19, 0,20);
    STEP2(0xE4, a1,b1,c1,d1, a2,b2,c2,d2, 20, 5, 5);
    STEP2(0xE4, d1,a1,b1,c1, d2,a2,b2,c2, 21,10, 9);
    STEP2(0xE4, c1,d1,a1,b1, c2,d2,a2,b2, 22,15,14);
    STEP2(0xE4, b1,c1,d1,a1, b2,c2,d2,a2, 23, 4,20);
    STEP2(0xE4, a1,b1,c1,d1, a2,b2,c2,d2, 24, 9, 5);
    STEP2(0xE4, d1,a1,b1,c1, d2,a2,b2,c2, 25,14, 9);
    STEP2(0xE4, c1,d1,a1,b1, c2,d2,a2,b2, 26, 3,14);
    STEP2(0xE4, b1,c1,d1,a1, b2,c2,d2,a2, 27, 8,20);
    STEP2(0xE4, a1,b1,c1,d1, a2,b2,c2,d2, 28,13, 5);
    STEP2(0xE4, d1,a1,b1,c1, d2,a2,b2,c2, 29, 2, 9);
    STEP2(0xE4, c1,d1,a1,b1, c2,d2,a2,b2, 30, 7,14);
    STEP2(0xE4, b1,c1,d1,a1, b2,c2,d2,a2, 31,12,20);

    STEP2(0x96, a1,b1,c1,d1, a2,b2,c2,d2, 32, 5, 4);
    STEP2(0x96, d1,a1,b1,c1, d2,a2,b2,c2, 33, 8,11);
    STEP2(0x96, c1,d1,a1,b1, c2,d2,a2,b2, 34,11,16);
    STEP2(0x96, b1,c1,d1,a1, b2,c2,d2,a2, 35,14,23);
    STEP2(0x96, a1,b1,c1,d1, a2,b2,c2,d2, 36, 1, 4);
    STEP2(0x96, d1,a1,b1,c1, d2,a2,b2,c2, 37, 4,11);
    STEP2(0x96, c1,d1,a1,b1, c2,d2,a2,b2, 38, 7,16);
    STEP2(0x96, b1,c1,d1,a1, b2,c2,d2,a2, 39,10,23);
    STEP2(0x96, a1,b1,c1,d1, a2,b2,c2,d2, 40,13, 4);
    STEP2(0x96, d1,a1,b1,c1, d2,a2,b2,c2, 41, 0,11);
    STEP2(0x96, c1,d1,a1,b1, c2,d2,a2,b2, 42, 3,16);
    STEP2(0x96, b1,c1,d1,a1, b2,c2,d2,a2, 43, 6,23);
    STEP2(0x96, a1,b1,c1,d1, a2,b2,c2,d2, 44, 9, 4);
    STEP2(0x96, d1,a1,b1,c1, d2,a2,b2,c2, 45,12,11);
    STEP2(0x96, c1,d1,a1,b1, c2,d2,a2,b2, 46,15,16);
    STEP2(0x96, b1,c1,d1,a1, b2,c2,d2,a2, 47, 2,23);

    STEP2(0x39, a1,b1,c1,d1, a2,b2,c2,d2, 48, 0, 6);
    STEP2(0x39, d1,a1,b1,c1, d2,a2,b2,c2, 49, 7,10);
    STEP2(0x39, c1,d1,a1,b1, c2,d2,a2,b2, 50,14,15);
    STEP2(0x39, b1,c1,d1,a1, b2,c2,d2,a2, 51, 5,21);
    STEP2(0x39, a1,b1,c1,d1, a2,b2,c2,d2, 52,12, 6);
    STEP2(0x39, d1,a1,b1,c1, d2,a2,b2,c2, 53, 3,10);
    STEP2(0x39, c1,d1,a1,b1, c2,d2,a2,b2, 54,10,15);
    STEP2(0x39, b1,c1,d1,a1, b2,c2,d2,a2, 55, 1,21);
    STEP2(0x39, a1,b1,c1,d1, a2,b2,c2,d2, 56, 8, 6);
    STEP2(0x39, d1,a1,b1,c1, d2,a2,b2,c2, 57,15,10);
    STEP2(0x39, c1,d1,a1,b1, c2,d2,a2,b2, 58, 6,15);
    STEP2(0x39, b1,c1,d1,a1, b2,c2,d2,a2, 59,13,21);
    STEP2(0x39, a1,b1,c1,d1, a2,b2,c2,d2, 60, 4, 6);
    STEP2(0x39, d1,a1,b1,c1, d2,a2,b2,c2, 61,11,10);
    STEP2(0x39, c1,d1,a1,b1, c2,d2,a2,b2, 62, 2,15);
    STEP2(0x39, b1,c1,d1,a1, b2,c2,d2,a2, 63, 9,21);

    st1[0] = _mm512_add_epi32(st1[0], a1);
    st1[1] = _mm512_add_epi32(st1[1], b1);
    st1[2] = _mm512_add_epi32(st1[2], c1);
    st1[3] = _mm512_add_epi32(st1[3], d1);
    st2[0] = _mm512_add_epi32(st2[0], a2);
    st2[1] = _mm512_add_epi32(st2[1], b2);
    st2[2] = _mm512_add_epi32(st2[2], c2);
    st2[3] = _mm512_add_epi32(st2[3], d2);
}

#undef STEP
#undef STEP2

static void group_core16(const fb_ctx *ctx,
                         const __m512i wvv_inner[2][64], const __m512i wvv_outer[2][64],
                         const __m512i vword_inner[2][16], __m512i vword_outer[2][16],
                         __m512i so[4]) {
    __m512i st[4];
    st[0] = _mm512_set1_epi32((int)ctx->mid[0]);
    st[1] = _mm512_set1_epi32((int)ctx->mid[1]);
    st[2] = _mm512_set1_epi32((int)ctx->mid[2]);
    st[3] = _mm512_set1_epi32((int)ctx->mid[3]);
    md5_block16_folded(st, wvv_inner[0], vword_inner[0]);
    if (ctx->nb_inner == 2) {
        md5_block16_folded(st, wvv_inner[1], vword_inner[1]);
    }

    __m512i Hx[8];
    hex_expand16(st, Hx);

    int r = ctx->outer_r;
    __m128i chi = _mm_cvtsi32_si128(8 * r);
    __m128i clo = _mm_cvtsi32_si128(32 - 8 * r);
    for (int vi = 0; vi < ctx->outer_nvar; vi++) {
        int m = ctx->outer_var_m[vi];
        __m512i hp;
        if (r == 0) {
            hp = Hx[m];
        } else {
            __m512i hh = (m < 8) ? _mm512_sll_epi32(Hx[m], chi) : _mm512_setzero_si512();
            __m512i ll = (m > 0) ? _mm512_srl_epi32(Hx[m - 1], clo) : _mm512_setzero_si512();
            hp = _mm512_or_si512(hh, ll);
        }
        vword_outer[ctx->outer_var[vi].block][ctx->outer_var[vi].wib] = hp;
    }

    so[0] = _mm512_set1_epi32((int)ctx->mid[0]);
    so[1] = _mm512_set1_epi32((int)ctx->mid[1]);
    so[2] = _mm512_set1_epi32((int)ctx->mid[2]);
    so[3] = _mm512_set1_epi32((int)ctx->mid[3]);
    md5_block16_folded(so, wvv_outer[0], vword_outer[0]);
    if (ctx->nb_outer == 2) {
        md5_block16_folded(so, wvv_outer[1], vword_outer[1]);
    }
}

static void group_core16_x2(const fb_ctx *ctx,
                            const __m512i wvv_inner[2][64], const __m512i wvv_outer[2][64],
                            const __m512i vword_inner1[2][16], const __m512i vword_inner2[2][16],
                            __m512i vword_outer1[2][16], __m512i vword_outer2[2][16],
                            __m512i so1[4], __m512i so2[4]) {
    __m512i st1[4], st2[4];
    for (int k = 0; k < 4; k++) {
        st1[k] = _mm512_set1_epi32((int)ctx->mid[k]);
        st2[k] = st1[k];
    }
    md5_block16x2_folded(st1, st2, wvv_inner[0], vword_inner1[0], vword_inner2[0]);
    if (ctx->nb_inner == 2) {
        md5_block16x2_folded(st1, st2, wvv_inner[1], vword_inner1[1], vword_inner2[1]);
    }

    __m512i Hx1[8], Hx2[8];
    hex_expand16(st1, Hx1);
    hex_expand16(st2, Hx2);

    int r = ctx->outer_r;
    __m128i chi = _mm_cvtsi32_si128(8 * r);
    __m128i clo = _mm_cvtsi32_si128(32 - 8 * r);
    for (int vi = 0; vi < ctx->outer_nvar; vi++) {
        int m = ctx->outer_var_m[vi];
        __m512i hp1, hp2;
        if (r == 0) {
            hp1 = Hx1[m];
            hp2 = Hx2[m];
        } else {
            __m512i hh1 = (m < 8) ? _mm512_sll_epi32(Hx1[m], chi) : _mm512_setzero_si512();
            __m512i ll1 = (m > 0) ? _mm512_srl_epi32(Hx1[m - 1], clo) : _mm512_setzero_si512();
            hp1 = _mm512_or_si512(hh1, ll1);
            __m512i hh2 = (m < 8) ? _mm512_sll_epi32(Hx2[m], chi) : _mm512_setzero_si512();
            __m512i ll2 = (m > 0) ? _mm512_srl_epi32(Hx2[m - 1], clo) : _mm512_setzero_si512();
            hp2 = _mm512_or_si512(hh2, ll2);
        }
        int blk = ctx->outer_var[vi].block;
        int wib = ctx->outer_var[vi].wib;
        vword_outer1[blk][wib] = hp1;
        vword_outer2[blk][wib] = hp2;
    }

    for (int k = 0; k < 4; k++) {
        so1[k] = _mm512_set1_epi32((int)ctx->mid[k]);
        so2[k] = so1[k];
    }
    md5_block16x2_folded(so1, so2, wvv_outer[0], vword_outer1[0], vword_outer2[0]);
    if (ctx->nb_outer == 2) {
        md5_block16x2_folded(so1, so2, wvv_outer[1], vword_outer1[1], vword_outer2[1]);
    }
}

static void prebroadcast16(const fb_ctx *ctx, __m512i wvv_inner[2][64], __m512i wvv_outer[2][64],
                           __m512i vword_inner[2][16], __m512i vword_outer[2][16]) {
    for (int blk = 0; blk < ctx->nb_inner; blk++) {
        for (int i = 0; i < 64; i++) {
            wvv_inner[blk][i] = _mm512_set1_epi32((int)ctx->wv_inner[blk][i]);
        }
    }
    for (int blk = 0; blk < ctx->nb_outer; blk++) {
        for (int i = 0; i < 64; i++) {
            wvv_outer[blk][i] = _mm512_set1_epi32((int)ctx->wv_outer[blk][i]);
        }
    }
    __m512i z = _mm512_setzero_si512();
    for (int blk = 0; blk < 2; blk++) {
        for (int w = 0; w < 16; w++) {
            vword_inner[blk][w] = z;
            vword_outer[blk][w] = z;
        }
    }
}

static void fill_inner_vword16(const fb_ctx *ctx, int d[FB_SECRET_LEN], __m512i vword_inner[2][16]) {
    uint32_t lw[FB_INNER_MAX_VAR][16];
    for (int vi = 0; vi < ctx->inner_nvar; vi++) {
        for (int j = 0; j < 16; j++) {
            lw[vi][j] = 0;
        }
    }
    for (int j = 0; j < 16; j++) {
        for (int sb = 0; sb < FB_SECRET_LEN; sb++) {
            uint32_t ch = FB_ALPHABET[d[sb]];
            lw[ctx->inner_map_var[sb]][j] |= ch << ctx->inner_map_shift[sb];
        }
        int p = FB_SECRET_LEN - 1;
        while (p >= 0) {
            if (++d[p] < FB_ALPHABET_SIZE) {
                break;
            }
            d[p] = 0;
            p--;
        }
    }
    for (int vi = 0; vi < ctx->inner_nvar; vi++) {
        vword_inner[ctx->inner_var[vi].block][ctx->inner_var[vi].wib] =
            _mm512_loadu_si512((const void *)lw[vi]);
    }
}

static inline void ser16w(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint8_t o[16]) {
    o[0] = (uint8_t)a; o[1] = (uint8_t)(a >> 8); o[2] = (uint8_t)(a >> 16); o[3] = (uint8_t)(a >> 24);
    o[4] = (uint8_t)b; o[5] = (uint8_t)(b >> 8); o[6] = (uint8_t)(b >> 16); o[7] = (uint8_t)(b >> 24);
    o[8] = (uint8_t)c; o[9] = (uint8_t)(c >> 8); o[10] = (uint8_t)(c >> 16); o[11] = (uint8_t)(c >> 24);
    o[12] = (uint8_t)d; o[13] = (uint8_t)(d >> 8); o[14] = (uint8_t)(d >> 16); o[15] = (uint8_t)(d >> 24);
}

static inline long long fb_check16(const __m512i so[4], __m512i expA, const fb_ctx *ctx, long long base) {
    __mmask16 mask = _mm512_cmpeq_epi32_mask(so[0], expA);
    if (!mask) {
        return -1;
    }
    uint32_t oA[16], oB[16], oC[16], oD[16];
    _mm512_storeu_si512((void *)oA, so[0]);
    _mm512_storeu_si512((void *)oB, so[1]);
    _mm512_storeu_si512((void *)oC, so[2]);
    _mm512_storeu_si512((void *)oD, so[3]);
    long long found = -1;
    for (int j = 0; j < 16; j++) {
        if (mask & (1 << j)) {
            uint8_t db[16];
            ser16w(oA[j], oB[j], oC[j], oD[j], db);
            if (memcmp(db, ctx->expected, 16) == 0) {
                long long cand = base + j;
                if (found < 0 || cand < found) {
                    found = cand;
                }
            }
        }
    }
    return found;
}

long long fb_scan_avx512(const fb_ctx *ctx, long long lo, long long hi, volatile int *stop) {
    if (lo >= hi) {
        return -1;
    }
    __m512i wvv_inner[2][64], wvv_outer[2][64];
    __m512i vword_inner1[2][16], vword_outer1[2][16];
    __m512i vword_inner2[2][16], vword_outer2[2][16];
    prebroadcast16(ctx, wvv_inner, wvv_outer, vword_inner1, vword_outer1);
    __m512i zero = _mm512_setzero_si512();
    for (int blk = 0; blk < 2; blk++) {
        for (int w = 0; w < 16; w++) {
            vword_inner2[blk][w] = zero;
            vword_outer2[blk][w] = zero;
        }
    }
    __m512i expA = _mm512_set1_epi32((int)ctx->expected_a);

    int d[FB_SECRET_LEN];
    long long idx = lo;
    for (int p = FB_SECRET_LEN - 1; p >= 0; p--) {
        d[p] = (int)(idx % FB_ALPHABET_SIZE);
        idx /= FB_ALPHABET_SIZE;
    }

    long long n = lo;
    long long groups = 0;
    while (n + 32 <= hi) {
        fill_inner_vword16(ctx, d, vword_inner1);
        fill_inner_vword16(ctx, d, vword_inner2);
        __m512i so1[4], so2[4];
        group_core16_x2(ctx, wvv_inner, wvv_outer, vword_inner1, vword_inner2,
                        vword_outer1, vword_outer2, so1, so2);
        long long f1 = fb_check16(so1, expA, ctx, n);
        if (f1 >= 0) {
            if (stop) {
                FB_STOP_STORE(stop, 1);
            }
            return f1;
        }
        long long f2 = fb_check16(so2, expA, ctx, n + 16);
        if (f2 >= 0) {
            if (stop) {
                FB_STOP_STORE(stop, 1);
            }
            return f2;
        }
        n += 32;
        groups++;
        if (stop && (groups & 0x7FF) == 0 && FB_STOP_LOAD(stop)) {
            return -1;
        }
    }
    while (n + 16 <= hi) {
        fill_inner_vword16(ctx, d, vword_inner1);
        __m512i so[4];
        group_core16(ctx, wvv_inner, wvv_outer, vword_inner1, vword_outer1, so);
        long long f = fb_check16(so, expA, ctx, n);
        if (f >= 0) {
            if (stop) {
                FB_STOP_STORE(stop, 1);
            }
            return f;
        }
        n += 16;
    }

    if (n < hi) {
        long long r = fb_scan_avx2(ctx, n, hi, stop);
        if (r >= 0) {
            return r;
        }
    }
    return -1;
}

void fb_group16_avx512(const fb_ctx *ctx, long long base, uint8_t out[16][16]) {
    __m512i wvv_inner[2][64], wvv_outer[2][64], vword_inner[2][16], vword_outer[2][16];
    prebroadcast16(ctx, wvv_inner, wvv_outer, vword_inner, vword_outer);
    int d[FB_SECRET_LEN];
    long long idx = base;
    for (int p = FB_SECRET_LEN - 1; p >= 0; p--) {
        d[p] = (int)(idx % FB_ALPHABET_SIZE);
        idx /= FB_ALPHABET_SIZE;
    }
    fill_inner_vword16(ctx, d, vword_inner);
    __m512i so[4];
    group_core16(ctx, wvv_inner, wvv_outer, vword_inner, vword_outer, so);
    uint32_t oA[16], oB[16], oC[16], oD[16];
    _mm512_storeu_si512((void *)oA, so[0]);
    _mm512_storeu_si512((void *)oB, so[1]);
    _mm512_storeu_si512((void *)oC, so[2]);
    _mm512_storeu_si512((void *)oD, so[3]);
    for (int j = 0; j < 16; j++) {
        ser16w(oA[j], oB[j], oC[j], oD[j], out[j]);
    }
}

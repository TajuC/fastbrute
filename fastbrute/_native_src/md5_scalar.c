#include "brute.h"
#include <string.h>

static inline uint32_t rotl(uint32_t x, int c) {
    return (x << c) | (x >> (32 - c));
}

static inline uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void md5_compress(uint32_t st[4], const uint8_t block[64]) {
    uint32_t M[16];
    memcpy(M, block, 64);
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3];
    for (int i = 0; i < 64; i++) {
        uint32_t f;
        int g;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) & 15;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) & 15;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) & 15;
        }
        uint32_t tmp = d;
        d = c;
        c = b;
        b = b + rotl(a + f + FB_K[i] + M[g], FB_SH[i]);
        a = tmp;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
}

void fb_hex16(const uint32_t digest[4], uint8_t out[32]) {
    static const char hexd[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    uint8_t b[16];
    for (int i = 0; i < 4; i++) {
        b[4 * i + 0] = (uint8_t)(digest[i]);
        b[4 * i + 1] = (uint8_t)(digest[i] >> 8);
        b[4 * i + 2] = (uint8_t)(digest[i] >> 16);
        b[4 * i + 3] = (uint8_t)(digest[i] >> 24);
    }
    for (int i = 0; i < 16; i++) {
        out[2 * i + 0] = (uint8_t)hexd[b[i] >> 4];
        out[2 * i + 1] = (uint8_t)hexd[b[i] & 15];
    }
}

static void fb_precompute(fb_ctx *ctx) {
    size_t L = ctx->tail_len;

    for (int blk = 0; blk < ctx->nb_inner; blk++) {
        for (int i = 0; i < 64; i++) {
            ctx->wv_inner[blk][i] = FB_K[i] + rd32(ctx->inner_tmpl + 64 * blk + 4 * FB_G[i]);
        }
    }
    for (int blk = 0; blk < ctx->nb_outer; blk++) {
        for (int i = 0; i < 64; i++) {
            ctx->wv_outer[blk][i] = FB_K[i] + rd32(ctx->outer_tmpl + 64 * blk + 4 * FB_G[i]);
        }
    }

    ctx->inner_nvar = 0;
    for (int sb = 0; sb < FB_SECRET_LEN; sb++) {
        size_t p = L + (size_t)sb;
        int gw = (int)(p / 4);
        int blk = gw / 16;
        int wib = gw % 16;
        int vi = -1;
        for (int k = 0; k < ctx->inner_nvar; k++) {
            if (ctx->inner_var[k].block == blk && ctx->inner_var[k].wib == wib) {
                vi = k;
                break;
            }
        }
        if (vi < 0) {
            vi = ctx->inner_nvar++;
            ctx->inner_var[vi].block = blk;
            ctx->inner_var[vi].wib = wib;
        }
        ctx->inner_map_var[sb] = vi;
        ctx->inner_map_shift[sb] = (int)((p % 4) * 8);
    }

    int r = (int)(L % 4);
    int w0 = (int)(L / 4);
    int mmax = (r > 0) ? 8 : 7;
    ctx->outer_r = r;
    ctx->outer_nvar = 0;
    for (int m = 0; m <= mmax; m++) {
        int gw = w0 + m;
        int vi = ctx->outer_nvar++;
        ctx->outer_var[vi].block = gw / 16;
        ctx->outer_var[vi].wib = gw % 16;
        ctx->outer_var_m[vi] = m;
    }
}

void fb_setup(fb_ctx *ctx, const uint8_t *challenge, size_t clen, const uint8_t expected[16]) {
    memset(ctx, 0, sizeof *ctx);
    uint32_t st[4] = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};
    size_t full = clen & ~(size_t)63;
    for (size_t off = 0; off < full; off += 64) {
        md5_compress(st, challenge + off);
    }
    memcpy(ctx->mid, st, sizeof st);
    size_t L = clen - full;
    ctx->tail_len = L;
    memcpy(ctx->expected, expected, 16);
    ctx->expected_a = (uint32_t)expected[0]
                    | ((uint32_t)expected[1] << 8)
                    | ((uint32_t)expected[2] << 16)
                    | ((uint32_t)expected[3] << 24);

    size_t di = L + FB_SECRET_LEN;
    ctx->nb_inner = (int)((di + 9 + 63) / 64);
    memcpy(ctx->inner_tmpl, challenge + full, L);
    ctx->inner_tmpl[di] = 0x80;
    uint64_t bits_i = (uint64_t)(clen + FB_SECRET_LEN) * 8u;
    size_t lp_i = (size_t)ctx->nb_inner * 64 - 8;
    for (int k = 0; k < 8; k++) {
        ctx->inner_tmpl[lp_i + k] = (uint8_t)(bits_i >> (8 * k));
    }

    size_t dcnt = L + 32;
    ctx->nb_outer = (int)((dcnt + 9 + 63) / 64);
    memcpy(ctx->outer_tmpl, challenge + full, L);
    ctx->outer_tmpl[dcnt] = 0x80;
    uint64_t bits_o = (uint64_t)(clen + 32) * 8u;
    size_t lp_o = (size_t)ctx->nb_outer * 64 - 8;
    for (int k = 0; k < 8; k++) {
        ctx->outer_tmpl[lp_o + k] = (uint8_t)(bits_o >> (8 * k));
    }

    fb_precompute(ctx);
}

void fb_hash_one(const fb_ctx *ctx, const uint8_t cand[FB_SECRET_LEN], uint8_t out[16]) {
    size_t L = ctx->tail_len;
    uint8_t inbuf[128], outbuf[128];
    memcpy(inbuf, ctx->inner_tmpl, (size_t)ctx->nb_inner * 64);
    for (int i = 0; i < FB_SECRET_LEN; i++) {
        inbuf[L + i] = cand[i];
    }
    uint32_t st[4];
    memcpy(st, ctx->mid, sizeof st);
    md5_compress(st, inbuf);
    if (ctx->nb_inner == 2) {
        md5_compress(st, inbuf + 64);
    }
    uint8_t hx[32];
    fb_hex16(st, hx);
    memcpy(outbuf, ctx->outer_tmpl, (size_t)ctx->nb_outer * 64);
    memcpy(outbuf + L, hx, 32);
    uint32_t so[4];
    memcpy(so, ctx->mid, sizeof so);
    md5_compress(so, outbuf);
    if (ctx->nb_outer == 2) {
        md5_compress(so, outbuf + 64);
    }
    for (int i = 0; i < 4; i++) {
        out[4 * i + 0] = (uint8_t)(so[i]);
        out[4 * i + 1] = (uint8_t)(so[i] >> 8);
        out[4 * i + 2] = (uint8_t)(so[i] >> 16);
        out[4 * i + 3] = (uint8_t)(so[i] >> 24);
    }
}

long long fb_scan_scalar(const fb_ctx *ctx, long long lo, long long hi, volatile int *stop) {
    if (lo >= hi) {
        return -1;
    }
    size_t L = ctx->tail_len;
    int nbi = ctx->nb_inner;
    int nbo = ctx->nb_outer;
    uint8_t inbuf[128], outbuf[128];
    memcpy(inbuf, ctx->inner_tmpl, (size_t)nbi * 64);
    memcpy(outbuf, ctx->outer_tmpl, (size_t)nbo * 64);

    int d[FB_SECRET_LEN];
    long long idx = lo;
    for (int p = FB_SECRET_LEN - 1; p >= 0; p--) {
        d[p] = (int)(idx % FB_ALPHABET_SIZE);
        idx /= FB_ALPHABET_SIZE;
    }

    long long counter = 0;
    for (long long n = lo; n < hi;) {
        for (int i = 0; i < FB_SECRET_LEN; i++) {
            inbuf[L + i] = FB_ALPHABET[d[i]];
        }
        uint32_t st[4];
        memcpy(st, ctx->mid, sizeof st);
        md5_compress(st, inbuf);
        if (nbi == 2) {
            md5_compress(st, inbuf + 64);
        }
        uint8_t hx[32];
        fb_hex16(st, hx);
        memcpy(outbuf + L, hx, 32);
        uint32_t so[4];
        memcpy(so, ctx->mid, sizeof so);
        md5_compress(so, outbuf);
        if (nbo == 2) {
            md5_compress(so, outbuf + 64);
        }
        counter++;
        if (so[0] == ctx->expected_a) {
            uint8_t db[16];
            for (int i = 0; i < 4; i++) {
                db[4 * i + 0] = (uint8_t)(so[i]);
                db[4 * i + 1] = (uint8_t)(so[i] >> 8);
                db[4 * i + 2] = (uint8_t)(so[i] >> 16);
                db[4 * i + 3] = (uint8_t)(so[i] >> 24);
            }
            if (memcmp(db, ctx->expected, 16) == 0) {
                if (stop) {
                    FB_STOP_STORE(stop, 1);
                }
                return n;
            }
        }
        if (stop && (counter & 0xFFFF) == 0 && FB_STOP_LOAD(stop)) {
            return -1;
        }
        n++;
        if (n >= hi) {
            break;
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
    return -1;
}

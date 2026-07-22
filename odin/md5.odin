package main

SECRET_LEN :: 5
BASE :: 62
KEYSPACE :: 916132832

ALPHABET := [62]u8{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
	'U', 'V', 'W', 'X', 'Y', 'Z',
}

HEX := "0123456789abcdef"

IV := [4]u32{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476}

K := [64]u32{
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
}

SH := [64]u32{
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
}

SCHED := [64]int{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12,
	5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2,
	0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9,
}

Word_Ref :: struct {
	block: int,
	wib:   int,
}

Ctx :: struct {
	mid:             [4]u32,
	tail_len:        int,
	nb_inner:        int,
	nb_outer:        int,
	inner_tmpl:      [128]u8,
	outer_tmpl:      [128]u8,
	expected:        [16]u8,
	expected_a:      u32,
	wv_inner:        [2][64]u32,
	wv_outer:        [2][64]u32,
	inner_nvar:      int,
	inner_var:       [3]Word_Ref,
	inner_map_var:   [SECRET_LEN]int,
	inner_map_shift: [SECRET_LEN]int,
	outer_r:         int,
	outer_nvar:      int,
	outer_var:       [9]Word_Ref,
	outer_var_m:     [9]int,
}

rotl :: proc(x: u32, c: u32) -> u32 {
	return (x << c) | (x >> (32 - c))
}

read_u32le :: proc(b: []u8) -> u32 {
	return u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) | (u32(b[3]) << 24)
}

md5_compress :: proc(st: ^[4]u32, block: []u8) {
	M: [16]u32
	for i in 0 ..< 16 {
		M[i] = read_u32le(block[i * 4:])
	}
	a, b, c, d := st[0], st[1], st[2], st[3]
	for i in 0 ..< 64 {
		f: u32
		g: int
		if i < 16 {
			f = (b & c) | ((~b) & d)
			g = i
		} else if i < 32 {
			f = (d & b) | ((~d) & c)
			g = (5 * i + 1) & 15
		} else if i < 48 {
			f = (b ~ c) ~ d
			g = (3 * i + 5) & 15
		} else {
			f = c ~ (b | (~d))
			g = (7 * i) & 15
		}
		tmp := d
		d = c
		c = b
		b = b + rotl(a + f + K[i] + M[g], SH[i])
		a = tmp
	}
	st[0] += a
	st[1] += b
	st[2] += c
	st[3] += d
}

digest_bytes :: proc(st: [4]u32) -> [16]u8 {
	out: [16]u8
	for i in 0 ..< 4 {
		out[4 * i + 0] = u8(st[i])
		out[4 * i + 1] = u8(st[i] >> 8)
		out[4 * i + 2] = u8(st[i] >> 16)
		out[4 * i + 3] = u8(st[i] >> 24)
	}
	return out
}

hex16 :: proc(digest: [4]u32, out: []u8) {
	b := digest_bytes(digest)
	for i in 0 ..< 16 {
		out[2 * i + 0] = HEX[b[i] >> 4]
		out[2 * i + 1] = HEX[b[i] & 15]
	}
}

precompute :: proc(ctx: ^Ctx) {
	L := ctx.tail_len
	for blk in 0 ..< ctx.nb_inner {
		for i in 0 ..< 64 {
			ctx.wv_inner[blk][i] = K[i] + read_u32le(ctx.inner_tmpl[64 * blk + 4 * SCHED[i]:])
		}
	}
	for blk in 0 ..< ctx.nb_outer {
		for i in 0 ..< 64 {
			ctx.wv_outer[blk][i] = K[i] + read_u32le(ctx.outer_tmpl[64 * blk + 4 * SCHED[i]:])
		}
	}

	ctx.inner_nvar = 0
	for sb in 0 ..< SECRET_LEN {
		p := L + sb
		gw := p / 4
		blk := gw / 16
		wib := gw % 16
		vi := -1
		for k in 0 ..< ctx.inner_nvar {
			if ctx.inner_var[k].block == blk && ctx.inner_var[k].wib == wib {
				vi = k
				break
			}
		}
		if vi < 0 {
			vi = ctx.inner_nvar
			ctx.inner_nvar += 1
			ctx.inner_var[vi] = Word_Ref{blk, wib}
		}
		ctx.inner_map_var[sb] = vi
		ctx.inner_map_shift[sb] = (p % 4) * 8
	}

	r := L % 4
	w0 := L / 4
	mmax := 7
	if r > 0 {
		mmax = 8
	}
	ctx.outer_r = r
	ctx.outer_nvar = 0
	for m in 0 ..= mmax {
		gw := w0 + m
		ctx.outer_var[ctx.outer_nvar] = Word_Ref{gw / 16, gw % 16}
		ctx.outer_var_m[ctx.outer_nvar] = m
		ctx.outer_nvar += 1
	}
}

setup :: proc(challenge: []u8, expected: [16]u8) -> Ctx {
	ctx: Ctx
	st := IV
	full := (len(challenge) / 64) * 64
	off := 0
	for off < full {
		md5_compress(&st, challenge[off:off + 64])
		off += 64
	}
	ctx.mid = st
	L := len(challenge) - full
	ctx.tail_len = L
	ctx.expected = expected
	ctx.expected_a =
		u32(expected[0]) | (u32(expected[1]) << 8) | (u32(expected[2]) << 16) | (u32(expected[3]) << 24)

	di := L + SECRET_LEN
	ctx.nb_inner = (di + 9 + 63) / 64
	copy(ctx.inner_tmpl[:L], challenge[full:])
	ctx.inner_tmpl[di] = 0x80
	bits_i := u64(len(challenge) + SECRET_LEN) * 8
	lp_i := ctx.nb_inner * 64 - 8
	for k in 0 ..< 8 {
		ctx.inner_tmpl[lp_i + k] = u8(bits_i >> (u64(k) * 8))
	}

	dcnt := L + 32
	ctx.nb_outer = (dcnt + 9 + 63) / 64
	copy(ctx.outer_tmpl[:L], challenge[full:])
	ctx.outer_tmpl[dcnt] = 0x80
	bits_o := u64(len(challenge) + 32) * 8
	lp_o := ctx.nb_outer * 64 - 8
	for k in 0 ..< 8 {
		ctx.outer_tmpl[lp_o + k] = u8(bits_o >> (u64(k) * 8))
	}

	precompute(&ctx)
	return ctx
}

hash_one :: proc(ctx: ^Ctx, cand: [5]u8) -> [16]u8 {
	L := ctx.tail_len
	inbuf := ctx.inner_tmpl
	for i in 0 ..< SECRET_LEN {
		inbuf[L + i] = cand[i]
	}
	st := ctx.mid
	md5_compress(&st, inbuf[:64])
	if ctx.nb_inner == 2 {
		md5_compress(&st, inbuf[64:128])
	}
	hx: [32]u8
	hex16(st, hx[:])
	outbuf := ctx.outer_tmpl
	for i in 0 ..< 32 {
		outbuf[L + i] = hx[i]
	}
	so := ctx.mid
	md5_compress(&so, outbuf[:64])
	if ctx.nb_outer == 2 {
		md5_compress(&so, outbuf[64:128])
	}
	return digest_bytes(so)
}

md5_full :: proc(msg: []u8) -> [16]u8 {
	st := IV
	full := (len(msg) / 64) * 64
	off := 0
	for off < full {
		md5_compress(&st, msg[off:off + 64])
		off += 64
	}
	L := len(msg) - full
	buf: [128]u8
	copy(buf[:L], msg[full:])
	buf[L] = 0x80
	nb := (L + 9 + 63) / 64
	bits := u64(len(msg)) * 8
	lp := nb * 64 - 8
	for k in 0 ..< 8 {
		buf[lp + k] = u8(bits >> (u64(k) * 8))
	}
	md5_compress(&st, buf[:64])
	if nb == 2 {
		md5_compress(&st, buf[64:128])
	}
	return digest_bytes(st)
}

candidate_at :: proc(index: i64) -> [5]u8 {
	cand: [5]u8
	rem := index
	for p := SECRET_LEN - 1; p >= 0; p -= 1 {
		cand[p] = ALPHABET[int(rem % BASE)]
		rem /= BASE
	}
	return cand
}

hex_val :: proc(c: u8) -> (u8, bool) {
	switch {
	case c >= '0' && c <= '9':
		return c - '0', true
	case c >= 'a' && c <= 'f':
		return c - 'a' + 10, true
	case c >= 'A' && c <= 'F':
		return c - 'A' + 10, true
	}
	return 0, false
}

hex_decode :: proc(s: string, out: []u8) -> int {
	if len(s) % 2 != 0 {
		return -1
	}
	n := len(s) / 2
	if n > len(out) {
		n = len(out)
	}
	for i in 0 ..< n {
		hi, ok1 := hex_val(s[2 * i])
		lo, ok2 := hex_val(s[2 * i + 1])
		if !ok1 || !ok2 {
			return -1
		}
		out[i] = (hi << 4) | lo
	}
	return n
}

bytes_to_hex :: proc(b: []u8, out: []u8) {
	for i in 0 ..< len(b) {
		out[2 * i + 0] = HEX[b[i] >> 4]
		out[2 * i + 1] = HEX[b[i] & 15]
	}
}

package main

import "core:simd"

rol :: proc(x: simd.u32x8, s: u32) -> simd.u32x8 {
	return simd.bit_or(simd.shl(x, splat(s)), simd.shr(x, splat(32 - s)))
}

nib_ascii :: proc(n: simd.u32x8) -> simd.u32x8 {
	m := simd.bit_and(simd.add(n, splat(0x06060606)), splat(0x10101010))
	corr := simd.bit_or(
		simd.bit_or(simd.shl(m, splat(1)), simd.shr(m, splat(2))),
		simd.bit_or(simd.shr(m, splat(3)), simd.shr(m, splat(4))),
	)
	return simd.add(n, simd.add(splat(0x30303030), corr))
}

hex_expand8 :: proc(st: ^[4]simd.u32x8, hout: ^[8]simd.u32x8) {
	m4 := splat(0xf)
	for k in 0 ..< 4 {
		w := st[k]
		nlo := simd.bit_or(
			simd.bit_or(
				simd.bit_and(simd.shr(w, splat(4)), m4),
				simd.shl(simd.bit_and(w, m4), splat(8)),
			),
			simd.bit_or(
				simd.shl(simd.bit_and(simd.shr(w, splat(12)), m4), splat(16)),
				simd.shl(simd.bit_and(simd.shr(w, splat(8)), m4), splat(24)),
			),
		)
		nhi := simd.bit_or(
			simd.bit_or(
				simd.bit_and(simd.shr(w, splat(20)), m4),
				simd.shl(simd.bit_and(simd.shr(w, splat(16)), m4), splat(8)),
			),
			simd.bit_or(
				simd.shl(simd.bit_and(simd.shr(w, splat(28)), m4), splat(16)),
				simd.shl(simd.bit_and(simd.shr(w, splat(24)), m4), splat(24)),
			),
		)
		hout[2 * k] = nib_ascii(nlo)
		hout[2 * k + 1] = nib_ascii(nhi)
	}
}

md5_block8_folded :: proc(st: ^[4]simd.u32x8, wvv: ^[64]simd.u32x8, vword: ^[16]simd.u32x8) {
	ones := splat(0xffffffff)
	a, b, c, d := st[0], st[1], st[2], st[3]
	{
		ff := simd.bit_xor(d, simd.bit_and(b, simd.bit_xor(c, d)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[0], vword[0]))
		a = simd.add(b, rol(t, 7))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(a, simd.bit_xor(b, c)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[1], vword[1]))
		d = simd.add(a, rol(t, 12))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(d, simd.bit_xor(a, b)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[2], vword[2]))
		c = simd.add(d, rol(t, 17))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(c, simd.bit_xor(d, a)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[3], vword[3]))
		b = simd.add(c, rol(t, 22))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(b, simd.bit_xor(c, d)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[4], vword[4]))
		a = simd.add(b, rol(t, 7))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(a, simd.bit_xor(b, c)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[5], vword[5]))
		d = simd.add(a, rol(t, 12))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(d, simd.bit_xor(a, b)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[6], vword[6]))
		c = simd.add(d, rol(t, 17))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(c, simd.bit_xor(d, a)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[7], vword[7]))
		b = simd.add(c, rol(t, 22))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(b, simd.bit_xor(c, d)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[8], vword[8]))
		a = simd.add(b, rol(t, 7))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(a, simd.bit_xor(b, c)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[9], vword[9]))
		d = simd.add(a, rol(t, 12))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(d, simd.bit_xor(a, b)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[10], vword[10]))
		c = simd.add(d, rol(t, 17))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(c, simd.bit_xor(d, a)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[11], vword[11]))
		b = simd.add(c, rol(t, 22))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(b, simd.bit_xor(c, d)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[12], vword[12]))
		a = simd.add(b, rol(t, 7))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(a, simd.bit_xor(b, c)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[13], vword[13]))
		d = simd.add(a, rol(t, 12))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(d, simd.bit_xor(a, b)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[14], vword[14]))
		c = simd.add(d, rol(t, 17))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(c, simd.bit_xor(d, a)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[15], vword[15]))
		b = simd.add(c, rol(t, 22))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(d, simd.bit_xor(b, c)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[16], vword[1]))
		a = simd.add(b, rol(t, 5))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(c, simd.bit_xor(a, b)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[17], vword[6]))
		d = simd.add(a, rol(t, 9))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(b, simd.bit_xor(d, a)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[18], vword[11]))
		c = simd.add(d, rol(t, 14))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(a, simd.bit_xor(c, d)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[19], vword[0]))
		b = simd.add(c, rol(t, 20))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(d, simd.bit_xor(b, c)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[20], vword[5]))
		a = simd.add(b, rol(t, 5))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(c, simd.bit_xor(a, b)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[21], vword[10]))
		d = simd.add(a, rol(t, 9))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(b, simd.bit_xor(d, a)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[22], vword[15]))
		c = simd.add(d, rol(t, 14))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(a, simd.bit_xor(c, d)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[23], vword[4]))
		b = simd.add(c, rol(t, 20))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(d, simd.bit_xor(b, c)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[24], vword[9]))
		a = simd.add(b, rol(t, 5))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(c, simd.bit_xor(a, b)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[25], vword[14]))
		d = simd.add(a, rol(t, 9))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(b, simd.bit_xor(d, a)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[26], vword[3]))
		c = simd.add(d, rol(t, 14))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(a, simd.bit_xor(c, d)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[27], vword[8]))
		b = simd.add(c, rol(t, 20))
	}
	{
		ff := simd.bit_xor(c, simd.bit_and(d, simd.bit_xor(b, c)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[28], vword[13]))
		a = simd.add(b, rol(t, 5))
	}
	{
		ff := simd.bit_xor(b, simd.bit_and(c, simd.bit_xor(a, b)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[29], vword[2]))
		d = simd.add(a, rol(t, 9))
	}
	{
		ff := simd.bit_xor(a, simd.bit_and(b, simd.bit_xor(d, a)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[30], vword[7]))
		c = simd.add(d, rol(t, 14))
	}
	{
		ff := simd.bit_xor(d, simd.bit_and(a, simd.bit_xor(c, d)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[31], vword[12]))
		b = simd.add(c, rol(t, 20))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(b, c), d)
		t := simd.add(simd.add(a, ff), simd.add(wvv[32], vword[5]))
		a = simd.add(b, rol(t, 4))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(a, b), c)
		t := simd.add(simd.add(d, ff), simd.add(wvv[33], vword[8]))
		d = simd.add(a, rol(t, 11))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(d, a), b)
		t := simd.add(simd.add(c, ff), simd.add(wvv[34], vword[11]))
		c = simd.add(d, rol(t, 16))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(c, d), a)
		t := simd.add(simd.add(b, ff), simd.add(wvv[35], vword[14]))
		b = simd.add(c, rol(t, 23))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(b, c), d)
		t := simd.add(simd.add(a, ff), simd.add(wvv[36], vword[1]))
		a = simd.add(b, rol(t, 4))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(a, b), c)
		t := simd.add(simd.add(d, ff), simd.add(wvv[37], vword[4]))
		d = simd.add(a, rol(t, 11))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(d, a), b)
		t := simd.add(simd.add(c, ff), simd.add(wvv[38], vword[7]))
		c = simd.add(d, rol(t, 16))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(c, d), a)
		t := simd.add(simd.add(b, ff), simd.add(wvv[39], vword[10]))
		b = simd.add(c, rol(t, 23))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(b, c), d)
		t := simd.add(simd.add(a, ff), simd.add(wvv[40], vword[13]))
		a = simd.add(b, rol(t, 4))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(a, b), c)
		t := simd.add(simd.add(d, ff), simd.add(wvv[41], vword[0]))
		d = simd.add(a, rol(t, 11))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(d, a), b)
		t := simd.add(simd.add(c, ff), simd.add(wvv[42], vword[3]))
		c = simd.add(d, rol(t, 16))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(c, d), a)
		t := simd.add(simd.add(b, ff), simd.add(wvv[43], vword[6]))
		b = simd.add(c, rol(t, 23))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(b, c), d)
		t := simd.add(simd.add(a, ff), simd.add(wvv[44], vword[9]))
		a = simd.add(b, rol(t, 4))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(a, b), c)
		t := simd.add(simd.add(d, ff), simd.add(wvv[45], vword[12]))
		d = simd.add(a, rol(t, 11))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(d, a), b)
		t := simd.add(simd.add(c, ff), simd.add(wvv[46], vword[15]))
		c = simd.add(d, rol(t, 16))
	}
	{
		ff := simd.bit_xor(simd.bit_xor(c, d), a)
		t := simd.add(simd.add(b, ff), simd.add(wvv[47], vword[2]))
		b = simd.add(c, rol(t, 23))
	}
	{
		ff := simd.bit_xor(c, simd.bit_or(b, simd.bit_xor(d, ones)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[48], vword[0]))
		a = simd.add(b, rol(t, 6))
	}
	{
		ff := simd.bit_xor(b, simd.bit_or(a, simd.bit_xor(c, ones)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[49], vword[7]))
		d = simd.add(a, rol(t, 10))
	}
	{
		ff := simd.bit_xor(a, simd.bit_or(d, simd.bit_xor(b, ones)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[50], vword[14]))
		c = simd.add(d, rol(t, 15))
	}
	{
		ff := simd.bit_xor(d, simd.bit_or(c, simd.bit_xor(a, ones)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[51], vword[5]))
		b = simd.add(c, rol(t, 21))
	}
	{
		ff := simd.bit_xor(c, simd.bit_or(b, simd.bit_xor(d, ones)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[52], vword[12]))
		a = simd.add(b, rol(t, 6))
	}
	{
		ff := simd.bit_xor(b, simd.bit_or(a, simd.bit_xor(c, ones)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[53], vword[3]))
		d = simd.add(a, rol(t, 10))
	}
	{
		ff := simd.bit_xor(a, simd.bit_or(d, simd.bit_xor(b, ones)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[54], vword[10]))
		c = simd.add(d, rol(t, 15))
	}
	{
		ff := simd.bit_xor(d, simd.bit_or(c, simd.bit_xor(a, ones)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[55], vword[1]))
		b = simd.add(c, rol(t, 21))
	}
	{
		ff := simd.bit_xor(c, simd.bit_or(b, simd.bit_xor(d, ones)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[56], vword[8]))
		a = simd.add(b, rol(t, 6))
	}
	{
		ff := simd.bit_xor(b, simd.bit_or(a, simd.bit_xor(c, ones)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[57], vword[15]))
		d = simd.add(a, rol(t, 10))
	}
	{
		ff := simd.bit_xor(a, simd.bit_or(d, simd.bit_xor(b, ones)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[58], vword[6]))
		c = simd.add(d, rol(t, 15))
	}
	{
		ff := simd.bit_xor(d, simd.bit_or(c, simd.bit_xor(a, ones)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[59], vword[13]))
		b = simd.add(c, rol(t, 21))
	}
	{
		ff := simd.bit_xor(c, simd.bit_or(b, simd.bit_xor(d, ones)))
		t := simd.add(simd.add(a, ff), simd.add(wvv[60], vword[4]))
		a = simd.add(b, rol(t, 6))
	}
	{
		ff := simd.bit_xor(b, simd.bit_or(a, simd.bit_xor(c, ones)))
		t := simd.add(simd.add(d, ff), simd.add(wvv[61], vword[11]))
		d = simd.add(a, rol(t, 10))
	}
	{
		ff := simd.bit_xor(a, simd.bit_or(d, simd.bit_xor(b, ones)))
		t := simd.add(simd.add(c, ff), simd.add(wvv[62], vword[2]))
		c = simd.add(d, rol(t, 15))
	}
	{
		ff := simd.bit_xor(d, simd.bit_or(c, simd.bit_xor(a, ones)))
		t := simd.add(simd.add(b, ff), simd.add(wvv[63], vword[9]))
		b = simd.add(c, rol(t, 21))
	}
	st[0] = simd.add(st[0], a)
	st[1] = simd.add(st[1], b)
	st[2] = simd.add(st[2], c)
	st[3] = simd.add(st[3], d)
}

package main

import "base:intrinsics"
import "core:simd"

splat :: proc(x: u32) -> simd.u32x8 {
	return simd.from_array([8]u32{x, x, x, x, x, x, x, x})
}

scan_simd :: proc(ctx: ^Ctx, lo: i64, hi: i64, found: ^i64, stop: ^i32) {
	if lo >= hi {
		return
	}

	wvv_inner: [2][64]simd.u32x8
	wvv_outer: [2][64]simd.u32x8
	vword_inner: [2][16]simd.u32x8
	vword_outer: [2][16]simd.u32x8
	for blk in 0 ..< ctx.nb_inner {
		for i in 0 ..< 64 {
			wvv_inner[blk][i] = splat(ctx.wv_inner[blk][i])
		}
	}
	for blk in 0 ..< ctx.nb_outer {
		for i in 0 ..< 64 {
			wvv_outer[blk][i] = splat(ctx.wv_outer[blk][i])
		}
	}
	zero := splat(0)
	for blk in 0 ..< 2 {
		for w in 0 ..< 16 {
			vword_inner[blk][w] = zero
			vword_outer[blk][w] = zero
		}
	}
	expA := ctx.expected_a

	d: [SECRET_LEN]int
	rem := lo
	for p := SECRET_LEN - 1; p >= 0; p -= 1 {
		d[p] = int(rem % BASE)
		rem /= BASE
	}

	n := lo
	groups: i64 = 0
	for n + 8 <= hi {
		lw: [3][8]u32
		for vi in 0 ..< ctx.inner_nvar {
			for j in 0 ..< 8 {
				lw[vi][j] = 0
			}
		}
		for j in 0 ..< 8 {
			for sb in 0 ..< SECRET_LEN {
				ch := u32(ALPHABET[d[sb]])
				lw[ctx.inner_map_var[sb]][j] |= ch << u32(ctx.inner_map_shift[sb])
			}
			p := SECRET_LEN - 1
			for p >= 0 {
				d[p] += 1
				if d[p] < BASE {
					break
				}
				d[p] = 0
				p -= 1
			}
		}
		for vi in 0 ..< ctx.inner_nvar {
			vword_inner[ctx.inner_var[vi].block][ctx.inner_var[vi].wib] = simd.from_array(lw[vi])
		}

		st: [4]simd.u32x8
		st[0] = splat(ctx.mid[0])
		st[1] = splat(ctx.mid[1])
		st[2] = splat(ctx.mid[2])
		st[3] = splat(ctx.mid[3])
		md5_block8_folded(&st, &wvv_inner[0], &vword_inner[0])
		if ctx.nb_inner == 2 {
			md5_block8_folded(&st, &wvv_inner[1], &vword_inner[1])
		}

		hx: [8]simd.u32x8
		hex_expand8(&st, &hx)
		r := ctx.outer_r
		for vi in 0 ..< ctx.outer_nvar {
			m := ctx.outer_var_m[vi]
			hp: simd.u32x8
			if r == 0 {
				hp = hx[m]
			} else {
				hh := zero
				if m < 8 {
					hh = simd.shl(hx[m], splat(u32(8 * r)))
				}
				ll := zero
				if m > 0 {
					ll = simd.shr(hx[m - 1], splat(u32(32 - 8 * r)))
				}
				hp = simd.bit_or(hh, ll)
			}
			vword_outer[ctx.outer_var[vi].block][ctx.outer_var[vi].wib] = hp
		}

		so: [4]simd.u32x8
		so[0] = splat(ctx.mid[0])
		so[1] = splat(ctx.mid[1])
		so[2] = splat(ctx.mid[2])
		so[3] = splat(ctx.mid[3])
		md5_block8_folded(&so, &wvv_outer[0], &vword_outer[0])
		if ctx.nb_outer == 2 {
			md5_block8_folded(&so, &wvv_outer[1], &vword_outer[1])
		}

		OA := simd.to_array(so[0])
		hit := false
		for j in 0 ..< 8 {
			if OA[j] == expA {
				hit = true
				break
			}
		}
		if hit {
			OB := simd.to_array(so[1])
			OC := simd.to_array(so[2])
			OD := simd.to_array(so[3])
			for j in 0 ..< 8 {
				if OA[j] == expA && digest_bytes([4]u32{OA[j], OB[j], OC[j], OD[j]}) == ctx.expected {
					record_hit(found, stop, n + i64(j))
					return
				}
			}
		}

		n += 8
		groups += 1
		if (groups & 0x1fff) == 0 && intrinsics.atomic_load(stop) != 0 {
			return
		}
	}

	for n < hi {
		if scan_one(ctx, n) {
			record_hit(found, stop, n)
			return
		}
		n += 1
	}
}

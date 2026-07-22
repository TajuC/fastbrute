package main

import "core:fmt"
import "core:os"
import "core:time"

selftest_scalar :: proc() -> bool {
	ok := true
	{
		d := md5_full([]u8{})
		buf: [32]u8
		bytes_to_hex(d[:], buf[:])
		if string(buf[:]) != "d41d8cd98f00b204e9800998ecf8427e" {
			fmt.println("FAIL md5 empty")
			ok = false
		}
	}
	{
		msg := [3]u8{'a', 'b', 'c'}
		d := md5_full(msg[:])
		buf: [32]u8
		bytes_to_hex(d[:], buf[:])
		if string(buf[:]) != "900150983cd24fb0d6963f7d28e17f72" {
			fmt.println("FAIL md5 abc")
			ok = false
		}
	}
	for v in TEST_VECTORS {
		clen := len(v.challenge) / 2
		ch := make([]u8, clen)
		defer delete(ch)
		hex_decode(v.challenge, ch)
		exp: [16]u8
		hex_decode(v.expected, exp[:])
		ctx := setup(ch, exp)
		cand: [5]u8
		for i in 0 ..< 5 {
			cand[i] = v.candidate[i]
		}
		if hash_one(&ctx, cand) != exp {
			fmt.printf("FAIL hash_one L=%d\n", clen)
			ok = false
		}
	}
	return ok
}

selftest :: proc() -> bool {
	ok := selftest_scalar()
	Ls := []int{0, 1, 2, 3, 17, 23, 24, 45, 50, 51, 52, 55, 56, 59, 60, 61, 62, 63, 64, 65, 100, 127, 128}
	idxs := []i64{0, 1, 123457}
	for L in Ls {
		ch := make([]u8, L)
		defer delete(ch)
		for i in 0 ..< L {
			ch[i] = u8((i * 7 + 3) & 255)
		}
		for idx in idxs {
			secret := candidate_at(idx)
			seed := setup(ch, [16]u8{})
			exp := hash_one(&seed, secret)
			ctx := setup(ch, exp)
			got, hit := find_secret(&ctx, 4)
			if !hit || got != idx {
				fmt.printf("FAIL search L=%d idx=%d got=%d hit=%v\n", L, idx, got, hit)
				ok = false
			}
		}
	}
	return ok
}

main :: proc() {
	args := os.args
	if len(args) >= 2 && args[1] == "test" {
		if selftest() {
			fmt.println("ALL SELF-TESTS PASSED")
			os.exit(0)
		}
		fmt.println("SELF-TESTS FAILED")
		os.exit(1)
	}
	if len(args) < 3 {
		fmt.printf("usage: %s <expected_hex_32> <challenge>\n", args[0])
		fmt.printf("       %s test\n", args[0])
		os.exit(2)
	}

	if len(args[1]) != 32 {
		fmt.printf("error: expected must be exactly 32 hex characters (got %d)\n", len(args[1]))
		os.exit(2)
	}
	exp: [16]u8
	if hex_decode(args[1], exp[:]) != 16 {
		fmt.println("error: expected must contain only hex characters")
		os.exit(2)
	}
	arg := args[2]
	challenge: []u8
	if len(arg) >= 2 && arg[0] == '0' && arg[1] == 'x' {
		challenge = make([]u8, (len(arg) - 2) / 2)
		if hex_decode(arg[2:], challenge) < 0 {
			fmt.println("error: challenge hex is malformed")
			os.exit(2)
		}
	} else {
		challenge = transmute([]u8)arg
	}

	ctx := setup(challenge, exp)
	nthreads := os.get_processor_core_count()
	start := time.tick_now()
	idx, hit := find_secret(&ctx, nthreads)
	elapsed := time.duration_seconds(time.tick_since(start))
	if hit {
		s := candidate_at(idx)
		fmt.printf("secret: %s  (candidate #%d, %.2fs)\n", string(s[:]), idx + 1, elapsed)
	} else {
		fmt.println("no match in the keyspace")
		os.exit(1)
	}
}

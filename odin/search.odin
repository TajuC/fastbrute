package main

import "base:intrinsics"
import "core:thread"

Shared :: struct {
	ctx:    ^Ctx,
	total:  i64,
	cursor: i64,
	found:  i64,
	stop:   i32,
}

CHUNK :: 1 << 20

record_hit :: proc(found: ^i64, stop: ^i32, idx: i64) {
	for {
		old := intrinsics.atomic_load(found)
		if old >= 0 && old <= idx {
			break
		}
		_, ok := intrinsics.atomic_compare_exchange_strong(found, old, idx)
		if ok {
			break
		}
	}
	intrinsics.atomic_store(stop, 1)
}

scan_one :: proc(ctx: ^Ctx, n: i64) -> bool {
	return hash_one(ctx, candidate_at(n)) == ctx.expected
}

worker_proc :: proc(t: ^thread.Thread) {
	sh := cast(^Shared)t.data
	for {
		if intrinsics.atomic_load(&sh.stop) != 0 {
			break
		}
		lo := intrinsics.atomic_load(&sh.cursor)
		if lo >= sh.total {
			break
		}
		hi := lo + CHUNK
		if hi > sh.total {
			hi = sh.total
		}
		_, ok := intrinsics.atomic_compare_exchange_strong(&sh.cursor, lo, hi)
		if !ok {
			continue
		}
		scan_simd(sh.ctx, lo, hi, &sh.found, &sh.stop)
	}
}

find_secret :: proc(ctx: ^Ctx, nthreads: int) -> (i64, bool) {
	sh: Shared
	sh.ctx = ctx
	sh.total = i64(KEYSPACE)
	sh.cursor = 0
	sh.found = -1
	sh.stop = 0

	nt := nthreads
	if nt < 1 {
		nt = 1
	}
	if nt > 4096 {
		nt = 4096
	}

	threads := make([]^thread.Thread, nt)
	defer delete(threads)
	for i in 0 ..< nt {
		t := thread.create(worker_proc)
		t.data = &sh
		threads[i] = t
		thread.start(t)
	}
	for i in 0 ..< nt {
		thread.join(threads[i])
		thread.destroy(threads[i])
	}
	idx := intrinsics.atomic_load(&sh.found)
	return idx, idx >= 0
}

#include "brute.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

int fb_has_avx2(void) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
#elif defined(_MSC_VER)
    int regs[4];
    __cpuidex(regs, 1, 0);
    if ((regs[2] & (1 << 27)) == 0) {
        return 0;
    }
    unsigned long long xcr0 = _xgetbv(0);
    if ((xcr0 & 0x6) != 0x6) {
        return 0;
    }
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 5)) != 0;
#else
    return 0;
#endif
}

int fb_has_avx512(void) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f")
        && __builtin_cpu_supports("avx512vl")
        && __builtin_cpu_supports("avx512bw")
        && __builtin_cpu_supports("avx512dq");
#elif defined(_MSC_VER)
    int regs[4];
    __cpuidex(regs, 1, 0);
    int osxsave = (regs[2] & (1 << 27)) != 0;
    if (!osxsave) {
        return 0;
    }
    unsigned long long xcr0 = _xgetbv(0);
    if ((xcr0 & 0xE6) != 0xE6) {
        return 0;
    }
    __cpuidex(regs, 7, 0);
    int f = (regs[1] & (1 << 16)) != 0;
    int dq = (regs[1] & (1 << 17)) != 0;
    int bw = (regs[1] & (1 << 30)) != 0;
    int vl = (regs[1] & (1u << 31)) != 0;
    return f && dq && bw && vl;
#else
    return 0;
#endif
}

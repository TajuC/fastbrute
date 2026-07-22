#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include "brute.h"

typedef long long (*fb_scan_fn)(const fb_ctx *, long long, long long, volatile int *);

static long long total_space() {
    long long t = 1;
    for (int i = 0; i < FB_SECRET_LEN; i++) {
        t *= FB_ALPHABET_SIZE;
    }
    return t;
}

static const char *isa_name(fb_scan_fn fn) {
    if (fn == fb_scan_avx512) {
        return "avx512";
    }
    if (fn == fb_scan_avx2) {
        return "avx2";
    }
    return "scalar";
}

static fb_scan_fn select_scan(void) {
    const char *forced = std::getenv("FASTBRUTE_ISA");
    if (forced) {
        if (std::strcmp(forced, "scalar") == 0) {
            return fb_scan_scalar;
        }
        if (std::strcmp(forced, "avx2") == 0) {
            return fb_has_avx2() ? fb_scan_avx2 : fb_scan_scalar;
        }
        if (std::strcmp(forced, "avx512") == 0) {
            return fb_has_avx512() ? fb_scan_avx512 : (fb_has_avx2() ? fb_scan_avx2 : fb_scan_scalar);
        }
    }
    if (fb_has_avx512()) {
        return fb_scan_avx512;
    }
    if (fb_has_avx2()) {
        return fb_scan_avx2;
    }
    return fb_scan_scalar;
}

static const long long FB_CHUNK = 1 << 20;

static long long run_search(const fb_ctx *ctx, long long total, long long nth, fb_scan_fn scan) {
    std::atomic<long long> found(-1);
    std::atomic<long long> cursor(0);
    volatile int stop = 0;

    auto worker = [&]() {
        for (;;) {
            if (FB_STOP_LOAD(&stop)) {
                break;
            }
            long long lo = cursor.fetch_add(FB_CHUNK, std::memory_order_relaxed);
            if (lo >= total) {
                break;
            }
            long long hi = lo + FB_CHUNK;
            if (hi > total) {
                hi = total;
            }
            long long r = scan(ctx, lo, hi, &stop);
            if (r >= 0) {
                long long prev = found.load(std::memory_order_relaxed);
                while (prev < 0 || r < prev) {
                    if (found.compare_exchange_weak(prev, r, std::memory_order_relaxed)) {
                        break;
                    }
                }
                FB_STOP_STORE(&stop, 1);
                break;
            }
        }
    };

    std::vector<std::thread> pool;
    try {
        pool.reserve((size_t)nth);
        for (long long i = 0; i < nth; i++) {
            pool.emplace_back(worker);
        }
    } catch (...) {
        FB_STOP_STORE(&stop, 1);
        for (auto &t : pool) {
            if (t.joinable()) {
                t.join();
            }
        }
        return -2;
    }
    for (auto &t : pool) {
        t.join();
    }
    return found.load();
}

static PyObject *py_find(PyObject *self, PyObject *args) {
    Py_buffer chal, exp;
    int threads = 0;
    if (!PyArg_ParseTuple(args, "y*y*|i", &chal, &exp, &threads)) {
        return NULL;
    }
    if (exp.len != 16) {
        PyBuffer_Release(&chal);
        PyBuffer_Release(&exp);
        PyErr_SetString(PyExc_ValueError, "expected must be exactly 16 bytes");
        return NULL;
    }

    fb_ctx ctx;
    fb_setup(&ctx, (const uint8_t *)chal.buf, (size_t)chal.len, (const uint8_t *)exp.buf);
    PyBuffer_Release(&chal);
    PyBuffer_Release(&exp);

    long long total = total_space();
    fb_scan_fn scan = select_scan();
    long long hw = std::thread::hardware_concurrency();
    if (hw < 1) {
        hw = 1;
    }
    long long nth = threads > 0 ? (long long)threads : hw;
    long long cap = hw * 4;
    if (nth > cap) {
        nth = cap;
    }
    if (nth > total) {
        nth = 1;
    }

    long long idx;
    Py_BEGIN_ALLOW_THREADS
    idx = run_search(&ctx, total, nth, scan);
    Py_END_ALLOW_THREADS

    if (idx == -2) {
        PyErr_SetString(PyExc_RuntimeError, "failed to create worker threads");
        return NULL;
    }
    if (idx < 0) {
        Py_RETURN_NONE;
    }
    uint8_t secret[FB_SECRET_LEN];
    long long q = idx;
    for (int p = FB_SECRET_LEN - 1; p >= 0; p--) {
        secret[p] = FB_ALPHABET[q % FB_ALPHABET_SIZE];
        q /= FB_ALPHABET_SIZE;
    }
    return Py_BuildValue("(y#L)", (const char *)secret, (Py_ssize_t)FB_SECRET_LEN,
                         (long long)(idx + 1));
}

static PyObject *py_hash_one(PyObject *self, PyObject *args) {
    Py_buffer chal, cand;
    if (!PyArg_ParseTuple(args, "y*y*", &chal, &cand)) {
        return NULL;
    }
    if (cand.len != FB_SECRET_LEN) {
        PyBuffer_Release(&chal);
        PyBuffer_Release(&cand);
        PyErr_SetString(PyExc_ValueError, "candidate must be 5 bytes");
        return NULL;
    }
    uint8_t zero[16] = {0};
    fb_ctx ctx;
    fb_setup(&ctx, (const uint8_t *)chal.buf, (size_t)chal.len, zero);
    uint8_t out[16];
    fb_hash_one(&ctx, (const uint8_t *)cand.buf, out);
    PyBuffer_Release(&chal);
    PyBuffer_Release(&cand);
    return PyBytes_FromStringAndSize((const char *)out, 16);
}

static PyObject *build_group(const fb_ctx *ctx, long long base, int lanes, const uint8_t *digs) {
    PyObject *list = PyList_New(lanes);
    if (!list) {
        return NULL;
    }
    for (int j = 0; j < lanes; j++) {
        PyObject *item = PyBytes_FromStringAndSize((const char *)(digs + 16 * j), 16);
        if (!item) {
            Py_DECREF(list);
            return NULL;
        }
        PyList_SET_ITEM(list, j, item);
    }
    return list;
}

static PyObject *py_group8(PyObject *self, PyObject *args) {
    Py_buffer chal;
    long long base;
    if (!PyArg_ParseTuple(args, "y*L", &chal, &base)) {
        return NULL;
    }
    if (!fb_has_avx2()) {
        PyBuffer_Release(&chal);
        PyErr_SetString(PyExc_RuntimeError, "group8 requires AVX2, which this CPU does not support");
        return NULL;
    }
    if (base < 0 || base > total_space() - 8) {
        PyBuffer_Release(&chal);
        PyErr_SetString(PyExc_ValueError, "base out of range");
        return NULL;
    }
    uint8_t zero[16] = {0};
    fb_ctx ctx;
    fb_setup(&ctx, (const uint8_t *)chal.buf, (size_t)chal.len, zero);
    PyBuffer_Release(&chal);
    uint8_t out[8][16];
    fb_group8_avx2(&ctx, base, out);
    return build_group(&ctx, base, 8, &out[0][0]);
}

static PyObject *py_group16(PyObject *self, PyObject *args) {
    Py_buffer chal;
    long long base;
    if (!PyArg_ParseTuple(args, "y*L", &chal, &base)) {
        return NULL;
    }
    if (!fb_has_avx512()) {
        PyBuffer_Release(&chal);
        PyErr_SetString(PyExc_RuntimeError, "group16 requires AVX-512, which this CPU does not support");
        return NULL;
    }
    if (base < 0 || base > total_space() - 16) {
        PyBuffer_Release(&chal);
        PyErr_SetString(PyExc_ValueError, "base out of range");
        return NULL;
    }
    uint8_t zero[16] = {0};
    fb_ctx ctx;
    fb_setup(&ctx, (const uint8_t *)chal.buf, (size_t)chal.len, zero);
    PyBuffer_Release(&chal);
    uint8_t out[16][16];
    fb_group16_avx512(&ctx, base, out);
    return build_group(&ctx, base, 16, &out[0][0]);
}

static PyObject *py_has_avx2(PyObject *self, PyObject *args) {
    return PyBool_FromLong(fb_has_avx2());
}

static PyObject *py_has_avx512(PyObject *self, PyObject *args) {
    return PyBool_FromLong(fb_has_avx512());
}

static PyObject *py_backend(PyObject *self, PyObject *args) {
    return PyUnicode_FromString(isa_name(select_scan()));
}

static PyMethodDef methods[] = {
    {"find", py_find, METH_VARARGS, "find(challenge, expected, threads=0) -> (secret, index) | None"},
    {"hash_one", py_hash_one, METH_VARARGS, "hash_one(challenge, candidate) -> 16-byte outer digest"},
    {"group8", py_group8, METH_VARARGS, "group8(challenge, base) -> list of 8 outer digests"},
    {"group16", py_group16, METH_VARARGS, "group16(challenge, base) -> list of 16 outer digests"},
    {"has_avx2", py_has_avx2, METH_NOARGS, "has_avx2() -> bool"},
    {"has_avx512", py_has_avx512, METH_NOARGS, "has_avx512() -> bool"},
    {"backend", py_backend, METH_NOARGS, "backend() -> active isa name"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "_native", NULL, -1, methods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit__native(void) {
    return PyModule_Create(&moduledef);
}

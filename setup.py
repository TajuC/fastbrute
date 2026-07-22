import os

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

PER_FILE = {
    "md5_avx2.c": {"msvc": ["/arch:AVX2"], "unix": ["-mavx2", "-mbmi"]},
    "md5_avx512.c": {
        "msvc": ["/arch:AVX512"],
        "unix": ["-mavx512f", "-mavx512vl", "-mavx512bw", "-mavx512dq"],
    },
}


class BuildExt(build_ext):
    def build_extensions(self):
        ct = self.compiler.compiler_type
        if ct == "msvc":
            base = ["/O2", "/Ob3", "/Oi", "/Ot", "/GL", "/Gy", "/Gw", "/EHsc"]
            link = ["/LTCG", "/OPT:REF", "/OPT:ICF"]
        else:
            base = ["-O3", "-flto", "-funroll-loops", "-fvisibility=hidden", "-pthread"]
            link = ["-flto", "-pthread"]
        for ext in self.extensions:
            ext.extra_compile_args = list(base)
            ext.extra_link_args = list(link)

        key = "msvc" if ct == "msvc" else "unix"
        original = self.compiler.compile

        def compile(sources, output_dir=None, macros=None, include_dirs=None,
                    debug=0, extra_preargs=None, extra_postargs=None, depends=None):
            objects = []
            post = list(extra_postargs or [])
            for src in sources:
                name = os.path.basename(src)
                extra = post + PER_FILE.get(name, {}).get(key, [])
                objects.extend(original([src], output_dir, macros, include_dirs, debug,
                                        extra_preargs, extra, depends))
            return objects

        self.compiler.compile = compile
        try:
            super().build_extensions()
        finally:
            self.compiler.compile = original


ext = Extension(
    "fastbrute._native",
    sources=[
        "fastbrute/_native_src/bruteforce.cpp",
        "fastbrute/_native_src/cpu_detect.c",
        "fastbrute/_native_src/md5_tables.c",
        "fastbrute/_native_src/md5_scalar.c",
        "fastbrute/_native_src/md5_avx2.c",
        "fastbrute/_native_src/md5_avx512.c",
    ],
    include_dirs=["fastbrute/_native_src"],
    language="c++",
)

setup(
    ext_modules=[ext],
    cmdclass={"build_ext": BuildExt},
)

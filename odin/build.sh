#!/bin/sh
odin build . -out:sf -o:aggressive -no-bounds-check -disable-assert -no-type-assert -disable-red-zone -use-single-module -microarch:x86-64-v3

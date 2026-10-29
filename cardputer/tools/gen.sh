#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M. Geisler (genkigenki)
# X0X's generated headers for the Cardputer build (the same ones host/build_host.sh makes), into build/gen.
# Python with Pillow and fontTools ($PYTHON, default python3); the built-in break needs a host C compiler:
# cc if there is one, else Docker (gcc:13).
set -e
cd "$(dirname "$0")/../.."
PY="${PYTHON:-python3}"
mkdir -p build/gen build/host
[ -f build/gen/felucca_font.h ] || "$PY" tools/gen_font.py build/gen/felucca_font.h >/dev/null
[ -f build/gen/x0x_drum_samples.h ] || "$PY" tools/gen_drum_samples.py build/gen/x0x_drum_samples.h >/dev/null
"$PY" tools/gen_words.py firmware/words.tsv web/words build/gen/ann_words.h >/dev/null
if [ ! -f build/gen/x0x_break_bank.h ]; then
    if command -v "${CC:-cc}" >/dev/null 2>&1; then
        sh tools/gen_builtin_break.sh build/gen >/dev/null
    else
        W=$(pwd -W 2>/dev/null || pwd)
        MSYS_NO_PATHCONV=1 docker run --rm -v "$W:/w" -w /w gcc:13 sh tools/gen_builtin_break.sh build/gen >/dev/null
    fi
fi
ls build/gen

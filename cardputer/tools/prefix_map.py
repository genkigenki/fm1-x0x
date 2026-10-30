# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M. Geisler (genkigenki)
# PlatformIO pre-script: the builder's own folders (the PlatformIO core with the framework, the project) out of
# the image. Asserts embed __FILE__, so a release would carry the builder's user name ("C:/Users/<name>/...").
Import("env")  # noqa: F821  (SCons)
import os

def _maps(path, to):
    p = os.path.abspath(path)
    return [f"-ffile-prefix-map={v}={to}" for v in {p, p.replace("\\", "/")}]

flags = _maps(env.subst("$PROJECT_CORE_DIR"), "pio") + _maps(env.subst("$PROJECT_DIR"), "project")  # noqa: F821
flags += _maps(os.path.dirname(env.subst("$PROJECT_DIR")), "src")  # noqa: F821  (the firmware's sources, ../)
env.Append(CCFLAGS=flags, ASFLAGS=flags)  # noqa: F821

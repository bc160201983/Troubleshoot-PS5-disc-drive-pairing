#!/usr/bin/env bash
# PROPOSED build for driveprobe-compare.elf (read-only diagnostic). Mirrors
# build-status.sh exactly (same toolchain, flags, libraries, linker script).
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
sdk_root="${PS5_PAYLOAD_SDK:-$PWD/tools/ps5-payload-sdk}"
compiler="${CC:-clang-18}"
linker="${LD:-ld.lld-18}"
"$compiler" -target x86_64-unknown-freebsd -fPIC -fno-stack-protector -fno-plt \
  -nobuiltininc -isystem "$sdk_root/target/include" -O2 -Wall -Wextra -Werror \
  -c driveprobe-compare.c -o driveprobe-compare.o
"$linker" -m elf_x86_64 -pie -T driveprobe.ld --eh-frame-hdr \
  -z max-page-size=0x4000 --hash-style=gnu -L "$sdk_root/target/lib" \
  "$sdk_root/target/lib/crt1.o" driveprobe-compare.o \
  -lc -lkernel_web -lSceLibcInternal -lSceNet -o driveprobe-compare.elf
sha256sum driveprobe-compare.elf > driveprobe-compare.elf.sha256
python3 validate_elf.py --sdk driveprobe-compare.elf

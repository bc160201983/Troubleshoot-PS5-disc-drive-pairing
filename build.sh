#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
sdk_root="${PS5_PAYLOAD_SDK:-$PWD/tools/ps5-payload-sdk}"
compiler="${CC:-clang-18}"
linker="${LD:-ld.lld-18}"
flags=(-target x86_64-unknown-freebsd -ffreestanding -fno-builtin -fPIC
       -fno-stack-protector -fno-plt -nostdlib -nobuiltininc
       -isystem "$sdk_root/target/include" -I vendor -Wall -Wextra -Werror -O1)
"$compiler" "${flags[@]}" -c driveprobe.c -o driveprobe.o
"$compiler" "${flags[@]}" -c vendor/_start.S -o start.o
"$compiler" "${flags[@]}" -c resolver.S -o resolver.o
"$linker" -m elf_x86_64 -pie --no-dynamic-linker --eh-frame-hdr \
    -z max-page-size=0x4000 --hash-style=gnu -T driveprobe.ld -e _start \
    start.o resolver.o driveprobe.o -o driveprobe.elf
python3 validate_elf.py
sha256sum driveprobe.elf > driveprobe.elf.sha256

$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
$probeCompiler=Get-ChildItem tools -Directory -Filter 'llvm-mingw-*' | Select-Object -First 1
& "$($probeCompiler.FullName)/bin/clang.exe" -target x86_64-unknown-freebsd -fPIC -fno-stack-protector -fno-plt -nobuiltininc -isystem tools/ps5-payload-sdk/target/include -O2 -Wall -Wextra -Werror -c driveprobe-sdk.c -o driveprobe-sdk.o
if($LASTEXITCODE) {throw 'Compilation failed'}
& "$($probeCompiler.FullName)/bin/ld.lld.exe" -m elf_x86_64 -pie -T driveprobe.ld --eh-frame-hdr -z max-page-size=0x4000 --hash-style=gnu -L tools/ps5-payload-sdk/target/lib tools/ps5-payload-sdk/target/lib/crt1.o driveprobe-sdk.o -lc -lkernel_web -lSceLibcInternal -lSceNet -o driveprobe-sdk.elf
if($LASTEXITCODE) {throw 'Link failed'}
Get-FileHash driveprobe-sdk.elf

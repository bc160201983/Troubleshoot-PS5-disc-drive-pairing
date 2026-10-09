param([string]$ToolsDirectory = (Join-Path $PSScriptRoot 'tools'))
$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$compilerRoot = Get-ChildItem -LiteralPath $ToolsDirectory -Directory -Filter 'llvm-mingw-*' | Select-Object -First 1
if (!$compilerRoot) { throw 'Portable LLVM toolchain is missing from tools.' }
$clang = Join-Path $compilerRoot.FullName 'bin/clang.exe'
$linker = Join-Path $compilerRoot.FullName 'bin/ld.lld.exe'
$sdkRoot = Join-Path $ToolsDirectory 'ps5-payload-sdk'
$headers = Join-Path $sdkRoot 'target/include'
$flags = @('-target','x86_64-unknown-freebsd','-ffreestanding','-fno-builtin','-fPIC','-fno-stack-protector','-fno-plt','-nostdlib','-nobuiltininc','-isystem',$headers,'-I',(Join-Path $project 'vendor'),'-Wall','-Wextra','-Werror','-O1')
& $clang @flags -c (Join-Path $project 'driveprobe.c') -o (Join-Path $project 'driveprobe.o')
if ($LASTEXITCODE) { throw 'Probe compilation failed.' }
& $clang @flags -c (Join-Path $project 'vendor/_start.S') -o (Join-Path $project 'start.o')
if ($LASTEXITCODE) { throw 'Entry point compilation failed.' }
& $clang @flags -c (Join-Path $project 'resolver.S') -o (Join-Path $project 'resolver.o')
if ($LASTEXITCODE) { throw 'Symbol resolver compilation failed.' }
& $linker -m elf_x86_64 -pie --no-dynamic-linker --eh-frame-hdr -z max-page-size=0x4000 --hash-style=gnu -T (Join-Path $project 'driveprobe.ld') -e _start (Join-Path $project 'start.o') (Join-Path $project 'resolver.o') (Join-Path $project 'driveprobe.o') -o (Join-Path $project 'driveprobe.elf')
if ($LASTEXITCODE) { throw 'ELF linking failed.' }
& (Join-Path $compilerRoot.FullName 'bin/llvm-readelf.exe') -h -l (Join-Path $project 'driveprobe.elf')
Get-FileHash -LiteralPath (Join-Path $project 'driveprobe.elf') -Algorithm SHA256


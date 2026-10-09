# PS5 DriveProbe

Diagnostic ELF payloads for already exploited PS5 consoles running elfldr. This project does not pair or register a disc drive.

## Working SDK variant

`driveprobe-sdk.elf` was executed successfully on firmware 7.00 and 11.40. It enumerates device names, reads path metadata, and issues only the published GET_STATUS query on `/dev/ddd` opened read-only. The report is written to `/data/driveprobe-sdk.txt` for retrieval through your existing FTP server.

See [console findings](CONSOLE-FINDINGS.md) for the measured raw status values, their uncertain interpretation, and research references. No offline pairing fix has been demonstrated.

The standard SDK runtime resolves symbols through kernel access and performs process permission setup. The diagnostic itself does not read protected keys, generate registration requests, change pairing records, send SCSI commands, or update drive firmware.

## Build

GitHub Actions downloads the pinned ps5-payload SDK v0.43 and builds both variants. Successful runs provide `ps5-driveprobe-elf`, including the ELFs and SHA-256 checksums. CI verifies compilation and transport tests; console execution is verified separately.

For Ubuntu 24.04, install clang-18, lld-18, curl, unzip, and Python 3, then:

```sh
mkdir -p tools
curl -fL https://github.com/ps5-payload-dev/sdk/releases/download/v0.43/ps5-payload-sdk.zip -o tools/sdk.zip
echo 'a9cc9929f21b2b2c5d5b309f3bab4997067c45281c0622cf4838b1aecba66fcb  tools/sdk.zip' | sha256sum --check
unzip tools/sdk.zip -d tools
bash build-sdk.sh
```

On Windows, extract SDK v0.43 under `tools/ps5-payload-sdk` and the portable LLVM-MinGW toolchain under `tools/llvm-mingw-*`, then run `./build-sdk.ps1`.

Send `driveprobe-sdk.elf` to your existing elfldr on TCP 9021, wait a few seconds, and retrieve `/data/driveprobe-sdk.txt` via FTP. An ELF upload alone is not evidence of successful execution; check the report's firmware and END marker.

## Experimental custom-runtime variant

The older `driveprobe.elf`, built by `build.sh` or `build.ps1`, avoids the standard SDK CRT and kernel access. It did not produce reports on the tested consoles and remains experimental. `collect.py` and its TCP 9022 transport apply only to that variant. `ftp_baseline.py` provides directory-only diagnostics through an existing FTP server.

Raw console logs, IP addresses, toolchains, firmware binaries, and private research remain local and are excluded from publication.

## License and sources

GPL-3.0-or-later; see LICENSE. The loader declarations, entry assembly and adapted linker script originate from [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk) commit `10b539de8f2b6773a9a78caa7576a97dbd99b29a` and retain their notices. SDK headers retain their respective licenses.

Reference loader: [elfldr](https://github.com/ps5-payload-dev/elfldr). Pairing-status ABI reference: [ddd_pair_dump](https://git.etawen.dev/earthonion/ddd_pair_dump), inspected commit `5fe02e0`.

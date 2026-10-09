# PS5-DriveProbe

Experimental read-only baseline diagnostics for an already exploited PS5 with a compatible ELF loader. **This does not pair or register a disc drive. PS5 runtime compatibility remains unverified.**

The probe reads selected OS sysctls, immediate `/dev` directory names, and metadata for exploratory optical-drive and mount paths. It does not open drive devices, send device commands, read cryptographic keys, access kernel memory, change pairing records, or write console files. A custom startup routine avoids the standard SDK CRT's process-permission patches. The existing jailbreak and loader have their own behavior outside this payload's control.

## Build and download

The **Actions** tab provides a **Build and test DriveProbe** workflow. Each successful run uploads `ps5-driveprobe-elf`, containing the compiled payload and SHA-256 checksum.

Actions verifies compilation, ELF structure, and local log-transfer behavior. **A green Actions run does not mean the payload works on a PS5 or that registration status has been measured.** Hosted runners cannot access your console.

For Ubuntu 24.04:

```sh
sudo apt-get install clang-18 lld-18 curl unzip python3
mkdir -p tools
curl -fL https://github.com/ps5-payload-dev/sdk/releases/download/v0.43/ps5-payload-sdk.zip -o tools/sdk.zip
echo 'a9cc9929f21b2b2c5d5b309f3bab4997067c45281c0622cf4838b1aecba66fcb  tools/sdk.zip' | sha256sum --check
unzip tools/sdk.zip -d tools
bash build.sh
python3 -m unittest test_collect -v
```

For Windows, extract the SDK v0.43 ZIP and the [portable LLVM-MinGW toolchain](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261006) under `tools`, then run `./build.ps1`. The toolchain should be named `llvm-mingw-20261006-ucrt-x86_64`. Use Python 3.11 or newer for the collection scripts and ELF validator.

## Collect a log

Start your existing ELF loader on TCP 9021. Replace the example address with your console's LAN IPv4 address; confirm the firmware in Settings before selecting a label. Keep the same disc-insertion condition on both consoles.

```sh
python collect.py 192.168.1.50 fw700
python collect.py 192.168.1.51 fw1140
```

The sender uploads `driveprobe.elf`, keeps the loader connection open, prints available loader responses, and retrieves the report from the console's TCP 9022. The payload waits up to 60 seconds for a client. Timestamped reports are saved under `logs/`. No PC listener or firewall rule is required. The report is unencrypted on your LAN.

If no report arrives, record any startup notification, loader response, or console error. Uploading bytes is not proof of successful execution. The supported startup layouts are a direct callable loader symbol resolver and the verified `getpid` syscall stub used by the reference elfldr. No kernel-memory fallback is implemented.

An FTP-only fallback reads directory names and permissions through an existing FTP server on port 2121:

```sh
python ftp_baseline.py 192.168.1.50 fw700
```

This uses anonymous FTP login and never downloads device file contents. Its process permissions may differ from the payload's. Logs stay local and are excluded from Git.

## Current limitations

Initial experimental uploads to consoles labeled 7.00 and 11.40 returned no payload reports or notifications. Subsequent source review found and corrected loader argument interpretation, segment alignment, and startup zero-initialization issues. The latest source adds loader stdout traces; these corrections still require console verification.

FTP baselines showed `cd0`, `driveauth`, `icc_bddrive`, and `lvd0` on both consoles with matching listed permissions. Device-node presence does not prove authentication or registration. Differences in firmware, hardware, connected peripherals, and process permissions can affect observations.

No verified registration-status service or safe drive-command interface is queried. Do not copy pairing records based on these logs.

## Source and license

GPL-3.0-or-later; see `LICENSE`. Vendored loader argument declarations and assembly entry, plus the adapted linker script, originate from [ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk) commit `10b539de8f2b6773a9a78caa7576a97dbd99b29a` and retain upstream copyright notices. The linker script adds explicit page alignment for the read-only data segment. SDK v0.43 provides FreeBSD ABI headers, which retain their respective licenses.

Reference loader: [ps5-payload-dev/elfldr](https://github.com/ps5-payload-dev/elfldr).

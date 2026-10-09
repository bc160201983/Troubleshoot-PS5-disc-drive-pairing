# Verified console findings — 9 October 2026

The original custom-runtime payload accepted TCP uploads but produced no reports. Rebuilding with the standard ps5-payload SDK v0.43 CRT produced complete reports on both consoles. This establishes that ELF payloads do work with their loaders.

The new `driveprobe-sdk.elf` writes only `/data/driveprobe-sdk.txt`. It enumerates `/dev`, reads path metadata and queries `/dev/ddd` using the published GET_STATUS ioctl (`0xC0284406`), opened O_RDONLY. It does not generate nonces or registration requests, submit registration data, issue SCSI commands, change pairing, or update drive firmware. The SDK runtime performs temporary process permission setup and kernel-based symbol resolution; therefore it does not satisfy the original custom probe's stricter no-kernel-access claim.

| Console firmware | GET_STATUS return | Service status | Raw output |
|---|---:|---:|---:|
| 7.00 (`07000044`) | 0 | 0 | 2 |
| 11.40 (`11400005`) | 0 | 0 | 6 |

Both consoles expose `/dev/ddd`, `/dev/driveauth`, `/dev/icc_bddrive`, and `/dev/cd0`.

External research maps 2 to NOT_PAIRED and 6 to FACTORY_PAIRED_TO_ANOTHER. The working 11.40 console returned 6 with an empty-input query, so do not treat this label as proof that its installed drive is paired elsewhere. The mapping and the query's missing certificate input require further investigation.

Primary reference: https://git.etawen.dev/earthonion/ddd_pair_dump — inspected commit 5fe02e0. Its published code dumps a nonce, a drive certificate, a registration request, and status; it is not a demonstrated restore or initial offline-pairing implementation.

No offline pairing fix has been demonstrated. No registration files or protected keys were copied between consoles. Firmware binaries and raw private reports are excluded from this public repository.

Build the corrected variant with `bash build-sdk.sh` after installing the pinned SDK, or `build-sdk.ps1` with the local portable toolchain. Send it to elfldr on TCP 9021, then retrieve `/data/driveprobe-sdk.txt` over FTP. The old TCP 9022 collector is only for the experimental custom-runtime variant.

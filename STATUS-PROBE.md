# Native drive-status diagnostic

`driveprobe-status.elf` reports four native status queries through elfldr stdout. It writes no console report file, so FTP is not required. It issues no power-control, pairing, SCSI, or firmware-update commands.

The diagnostic accepts only the firmware identifiers tested for 7.00 and 11.40. Other identifiers stop before querying the drive. Missing APIs are reported as unavailable.

Output buffers match the verified API widths: one byte for power, four bytes for attachment, two bytes for extended status, and one byte for chucking status. Extended status bits and chucking values are printed without assigning meanings to them. These values do not themselves establish a valid pairing certificate or identify a repair.

Build with the same pinned SDK as the other probes:

```sh
bash build-status.sh
```

Send the ELF to the existing elfldr socket and keep that connection open after sending. Half-close the sending side, then read the response until it closes. Check both the firmware identifier and the complete `BEGIN`/`END` markers; an upload alone does not verify execution. The GitHub Actions artifact includes the ELF and its checksum.

This is a diagnostic, not an offline pairing fix. Keep reports private if they are combined with device logs or other identifying information.

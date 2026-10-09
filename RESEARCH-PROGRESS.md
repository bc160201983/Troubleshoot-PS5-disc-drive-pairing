# Pairing investigation progress — 9 October 2026

The standard-SDK diagnostic executes on firmware 7.00 and 11.40. The published SDK build passed GitHub Actions run [37968459109](https://github.com/bc160201983/Troubleshoot-PS5-disc-drive-pairing/actions/runs/37968459109).

A follow-up local diagnostic supplied the drive certificate to GET_STATUS. On 11.40, it returned **5**, matching the user's **FACTORY_PAIRED: 5** Info-screen screenshot. The empty-input result of 6 should therefore not be interpreted as the installed drive being paired elsewhere.

On 7.00, the native attachment query succeeded and reported a drive attached. Empty-input GET_STATUS returned 2. Nonce generation succeeded, but both basic drive identification and the certificate challenge returned CAM status 0x2a. That value has no assigned meaning in the SDK's bundled FreeBSD headers. Temporary process permission checks and matching the observed queue settings did not resolve it.

A separate normal drive power-on experiment on 7.00 returned success, but `/dev/cd0` subsequently became absent. Pairing remained unmodified. Further drive operations stopped pending a normal console restart and restarting its existing jailbreak/loader. This is an unsuccessful diagnostic experiment, not a repair procedure.

Private local analysis of the owner's system shell identified the certificate-aware status path and a server registration-response validation step. Firmware binaries, raw certificates, serial numbers, and network addresses have not been published. The experimental certificate/power diagnostic remains local.

No verified offline pairing solution has been demonstrated. Current evidence does not support copying arbitrary files or another console's registration data. The next live test needs a fresh 7.00 runtime state; the protocol/storage analysis also remains incomplete.

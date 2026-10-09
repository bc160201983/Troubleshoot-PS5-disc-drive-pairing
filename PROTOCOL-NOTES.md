# Pairing protocol notes — 9 October 2026

These findings come from private, owner-authorized local analysis of the two consoles' system shell executables. No firmware binaries, secret values, or extracted instruction listings are published.

## Observed flow

1. Detect the detachable drive through the native ICC attachment query.
2. Open `/dev/ddd` and obtain a 16-byte nonce.
3. Retrieve the installed drive's 224-byte certificate using a challenge/response through the optical drive's CAM device.
4. Query pairing status with that certificate. An empty-input status query can return a different result and should not identify the installed drive's pairing relationship.
5. Generate a registration request through SBL. The observed request-generation wrapper accepts the certificate and reserves 400 bytes of output.
6. The HTTP pairing client obtains an access token and a separate console token. It sends an octet-stream request with Authorization and X-Psn-Console-Token headers to the drive-pair endpoint. No token values have been collected or published.
7. The returned registration data is passed to SBL's registration-response check. The observed 7.00 wrapper submits 352 bytes to ioctl 0xc0284403. Our diagnostics have never invoked this state-changing operation.

This identifies where the host hands registration data to the secure service. It does not establish the response's complete format, signature algorithm, storage location, replay properties, or whether a host-side bypass can make physical discs usable.

## Firmware comparison

The inspected 11.40 shell includes explicit factory-pairing messages and paths. The inspected 7.00 shell lacks those message strings; the examined registration-client decision handles the earlier states 0–4. This is evidence of implementation differences, not proof that every 7.00 component lacks factory-pairing support. Copying a 5/FACTORY_PAIRED state from 11.40 to 7.00 is not a validated method.

The independent community summary at https://alex-free.github.io/ps5-disc-drive-pairing-explained/ likewise distinguishes Slim consoles originally shipped on 7.x from later factory-paired models. That page explicitly describes an interpretation of community reports; treat its model/reset rules as secondary evidence.

## Driver failure

On 7.00, the native attachment query reports an attached drive and empty-input status returns 2. Both standard INQUIRY and the certificate challenge return CAM status 0x2a, which is unassigned in the bundled FreeBSD header. The failure survived a normal restart. Temporary process permissions, mapped bus fields, and matching the observed CAM settings did not resolve it.

The owner reports using WebKit with Poops, with no kstuff or ShadowMount currently running. Active interference from those payloads is therefore not supported by the current test setup. The archived public kstuff source inspected did not identify this error's meaning.

## Open questions

- What code returns CAM status 0x2a, and is it a transport, authorization, or device-state failure?
- Which setup steps does the native pairing path perform that our diagnostic lacks?
- What binds a validated registration response to the console, drive, and firmware?
- Where does SBL retain accepted pairing state, and can it be legitimately restored on the same hardware?

No offline pairing fix has been demonstrated. The current work establishes a reproducible diagnostic failure and a protocol boundary to investigate, rather than an ordinary settings file that can be safely copied between consoles.

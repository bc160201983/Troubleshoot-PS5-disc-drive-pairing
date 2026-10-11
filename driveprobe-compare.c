/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * driveprobe-compare.c  —  PROPOSED, read-only side-by-side drive diagnostic.
 * STATUS: pending owner approval. Not built, uploaded, or executed yet.
 *
 * Purpose: capture, in one pass and via elfldr stdout only, the drive
 * initialization / power / attachment / authentication *status* signals needed
 * to compare firmware 7.00 vs 11.40. It issues ONLY read-style queries.
 *
 * Explicitly DOES NOT, by construction (no such calls exist below):
 *   - set or force BD drive power (no sceKernelIccControlBDPowerState, no power-on)
 *   - issue SCSI SEND KEY / certificate challenge or any SCSI write
 *   - generate nonces, registration requests, or call any register / check-register ioctl
 *   - write pairing data, reset hardware, update firmware, or touch protected storage
 *   - write any file on the console (output goes to the loader's stdout socket)
 *
 * Firmware-guarded: runs its queries only on the two tested identifiers.
 */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <ps5/kernel.h>

/* ddd GET_STATUS ABI (read-only query), identical layout to driveprobe-sdk.c. */
struct ddd_query {
    uint8_t  pad[4];
    int32_t  in_size;
    uint8_t *in_buf;
    int32_t  out_size;
    uint8_t *out_buf;
    uint32_t status;
    uint32_t pad2;
};
_Static_assert(sizeof(struct ddd_query) == 40, "DDD ABI must be 40 bytes");

static intptr_t find_symbol(const char *name) {
    intptr_t p = kernel_dynlib_dlsym(-1, 1, name);
    return p ? p : kernel_dynlib_dlsym(-1, 0x2001, name);
}

int main(void) {
    uint32_t fw = kernel_get_fw_version();
    printf("BEGIN DriveProbe compare (read-only)\nfirmware=%08x\n", fw);
    fflush(stdout);

    if (fw != 0x07000044 && fw != 0x11400005) {
        printf("guard=unsupported firmware; no queries executed\nEND DriveProbe compare\n");
        fflush(stdout);
        return 1;
    }

    /* ---- Native ICC status reads (no control/set calls) ---- */
    int (*power)(unsigned char *)     = (int (*)(unsigned char *))find_symbol("sceKernelIccGetBDPowerState");
    int (*attached)(int *)            = (int (*)(int *))find_symbol("sceKernelIccIsExtBDDriveAttached");
    int (*extended)(uint16_t *)       = (int (*)(uint16_t *))find_symbol("sceKernelIccGetExtBDDriveStatus");
    int (*chucking)(unsigned char *)  = (int (*)(unsigned char *))find_symbol("sceKernelIccGetBDDriveChuckingState");

    if (power)    { unsigned char v = 0xfe; int rc = power(&v);    printf("icc_power_rc=%08x raw=%02x\n", rc, v); }
    else            printf("icc_power_api=unavailable\n");
    if (attached) { int v = -1;            int rc = attached(&v); printf("icc_attachment_rc=%08x raw=%d\n", rc, v); }
    else            printf("icc_attachment_api=unavailable\n");
    if (extended) { uint16_t v = 0xffff;   int rc = extended(&v); printf("icc_extended_status_rc=%08x raw=%04x\n", rc, v); }
    else            printf("icc_extended_status_api=unavailable\n");
    if (chucking) { unsigned char v = 0xfe; int rc = chucking(&v); printf("icc_chucking_rc=%08x raw=%02x\n", rc, v); }
    else            printf("icc_chucking_api=unavailable\n");
    fflush(stdout);

    /* ---- Device-node metadata (lstat only; no open of the drive transport) ---- */
    const char *paths[] = { "/dev/ddd", "/dev/driveauth", "/dev/icc_bddrive", "/dev/cd0" };
    for (unsigned i = 0; i < 4; i++) {
        struct stat s;
        if (lstat(paths[i], &s)) printf("path=%s errno=%d\n", paths[i], errno);
        else printf("path=%s mode=%o rdev=%u\n", paths[i], (unsigned)s.st_mode, (unsigned)s.st_rdev);
    }
    fflush(stdout);

    /* ---- ddd pairing status: published GET_STATUS, empty input, O_RDONLY ---- */
    /* Read-only: in_size=0/in_buf=NULL; only the published GET_STATUS ioctl is used. */
    int fd = open("/dev/ddd", O_RDONLY);
    if (fd < 0) {
        printf("ddd_open_errno=%d\n", errno);
    } else {
        uint32_t state = 0xffffffff;
        struct ddd_query q = {0};
        q.out_size = 4;
        q.out_buf  = (uint8_t *)&state;
        errno = 0;
        int rc = ioctl(fd, 0xC0284406UL, &q);
        int se = errno;
        printf("ddd_get_status_rc=%d errno=%d service_status=%08x state_raw=%08x\n", rc, se, q.status, state);
        close(fd);
    }

    printf("END DriveProbe compare\n");
    return fflush(stdout) ? 1 : 0;
}

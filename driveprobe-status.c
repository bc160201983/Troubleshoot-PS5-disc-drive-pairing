/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only status queries, with output returned through elfldr stdout.
 * No drive-power, pairing, SCSI, or firmware-update commands.
 */
#include <stdio.h>
#include <stdint.h>
#include <ps5/kernel.h>
static intptr_t find_symbol(const char *name) {
    intptr_t p = kernel_dynlib_dlsym(-1, 1, name);
    return p ? p : kernel_dynlib_dlsym(-1, 0x2001, name);
}
int main(void) {
    uint32_t fw = kernel_get_fw_version();
    printf("BEGIN DriveProbe native status\nfirmware=%08x\n", fw);
    fflush(stdout);
    if (fw != 0x07000044 && fw != 0x11400005) {
        printf("guard=unsupported firmware; no status queries executed\nEND DriveProbe native status\n");
        fflush(stdout);
        return 1;
    }
    int (*power)(unsigned char *) = (int (*)(unsigned char *))find_symbol("sceKernelIccGetBDPowerState");
    int (*attached)(int *) = (int (*)(int *))find_symbol("sceKernelIccIsExtBDDriveAttached");
    int (*extended)(uint16_t *) = (int (*)(uint16_t *))find_symbol("sceKernelIccGetExtBDDriveStatus");
    int (*chucking)(unsigned char *) = (int (*)(unsigned char *))find_symbol("sceKernelIccGetBDDriveChuckingState");
    if (power) {
        unsigned char value = 0xfe;
        int rc = power(&value);
        printf("power_rc=%08x raw_value=%02x\n", rc, value);
    } else printf("power_api=unavailable\n");
    if (attached) {
        int value = -1;
        int rc = attached(&value);
        printf("attachment_rc=%08x raw_value=%d\n", rc, value);
    } else printf("attachment_api=unavailable\n");
    if (extended) {
        uint16_t value = 0xffff;
        int rc = extended(&value);
        printf("extended_status_rc=%08x raw_value=%04x\n", rc, value);
    } else printf("extended_status_api=unavailable\n");
    if (chucking) {
        unsigned char value = 0xfe;
        int rc = chucking(&value);
        printf("chucking_status_rc=%08x raw_value=%02x\n", rc, value);
    } else printf("chucking_status_api=unavailable\n");
    printf("END DriveProbe native status\n");
    return fflush(stdout) ? 1 : 0;
}

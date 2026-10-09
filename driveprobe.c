/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only baseline probe. Uses SDK headers and loader ABI, not SDK CRT.
 * No kernel-memory access, pairing commands, device reads, or disk writes.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdarg.h>
#include "payload.h"

static int (*resolve)(int, const char *, void *);
static int (*p_vsnprintf)(char *, size_t, const char *, va_list);
static int *(*p_error)(void);
static int (*p_sysctl)(const char *, void *, size_t *, const void *, size_t);
static int (*p_open)(const char *, int, ...);
static int (*p_close)(int);
static int (*p_lstat)(const char *, struct stat *);
static int (*p_getdirentries)(int, char *, int, long *);
static int (*p_socket)(int, int, int);
static int (*p_bind)(int, const struct sockaddr *, socklen_t);
static int (*p_listen)(int, int);
static int (*p_accept)(int, struct sockaddr *, socklen_t *);
static int (*p_fcntl)(int, int, ...);
static int (*p_usleep)(unsigned int);
static ssize_t (*p_send)(int, const void *, size_t, int);
static int (*p_notify)(int, void *, size_t, int);
static char report[65536];
static size_t used;
extern unsigned char __bss_start[], __bss_end[];
void *probe_syscall_instruction;
int probe_dynlib_dlsym(int, const char *, void *);
long probe_write_stdout(const char *, size_t);

static void startup_trace(const char *text) {
    if (!probe_syscall_instruction) return;
    size_t length=0;
    while (text[length]) ++length;
    probe_write_stdout(text, length);
}

void *memset(void *dst, int value, size_t count) {
    unsigned char *p=dst;
    for (size_t i=0; i<count; ++i) p[i]=(unsigned char)value;
    return dst;
}

static int lookup(const char *name, void *out) {
    const int handles[] = {1, 0x2001, 2};
    for (unsigned i = 0; i < sizeof(handles)/sizeof(handles[0]); ++i)
        if (!resolve(handles[i], name, out) && *(void **)out) return 0;
    return -1;
}
#define LOAD(name) lookup(#name, &p_##name)
static int err(void) { return p_error ? *p_error() : -1; }
static void logline(const char *fmt, ...) {
    if (used >= sizeof(report)-1) return;
    va_list ap;
    va_start(ap, fmt);
    int n = p_vsnprintf(report+used, sizeof(report)-used, fmt, ap);
    va_end(ap);
    if (n > 0) used += (size_t)n < sizeof(report)-used ? (size_t)n : sizeof(report)-used-1;
}
static void notification(const char *text) {
    if (!p_notify) return;
    struct { char padding[45]; char message[3075]; } req = {0};
    size_t i=0;
    for (; text[i] && i<sizeof(req.message)-1; ++i) req.message[i]=text[i];
    p_notify(0, &req, sizeof(req), 0);
}
static void system_info(void) {
    const char *names[] = {"kern.ostype", "kern.osrelease", "kern.version", "hw.machine"};
    if (!p_sysctl) { logline("sysctlbyname=unavailable\n"); return; }
    for (unsigned i=0; i<sizeof(names)/sizeof(names[0]); ++i) {
        char value[1024] = {0}; size_t length=sizeof(value)-1;
        int rc=p_sysctl(names[i], value, &length, 0, 0);
        if (rc) logline("%s: rc=%d errno=%d\n", names[i], rc, err());
        else {
            for (size_t j=0; j<sizeof(value)-1; ++j)
                if (value[j]=='\n' || value[j]=='\r') value[j]=' ';
            logline("%s=%s\n", names[i], value);
        }
    }
    unsigned int sdk=0; size_t length=sizeof(sdk);
    int rc=p_sysctl("kern.sdk_version", &sdk, &length, 0, 0);
    logline("kern.sdk_version: rc=%d length=%lu value=0x%08x (not a verified firmware label)\n", rc, (unsigned long)length, sdk);
}
static void device_names(void) {
    if (!p_open || !p_getdirentries) { logline("dev_listing=unavailable\n"); return; }
    int fd=p_open("/dev", O_RDONLY | O_DIRECTORY);
    if (fd<0) { logline("dev_open: errno=%d\n", err()); return; }
    char buffer[4096] __attribute__((aligned(8))); long base=0;
    for (int batch=0; batch<64; ++batch) {
        int n=p_getdirentries(fd, buffer, sizeof(buffer), &base);
        if (n<=0) { logline("dev_listing_end: rc=%d errno=%d\n", n, n<0?err():0); break; }
        for (int off=0; off<n;) {
            if (n-off<8) { logline("dev_record=short\n"); goto done; }
            struct dirent *d=(struct dirent *)(buffer+off);
            if (d->d_reclen<9 || d->d_reclen>n-off || d->d_namlen+8>=d->d_reclen) {
                logline("dev_record=unexpected ABI; stopped\n"); goto done;
            }
            logline("dev_entry=%.*s type=%u\n", (int)d->d_namlen, d->d_name, (unsigned)d->d_type);
            off+=d->d_reclen;
        }
    }
done:
    p_close(fd);
}
static void path_metadata(void) {
    /* Exploratory names, NOT confirmed drive/registration interfaces. */
    const char *paths[]={"/dev/cd0", "/dev/acd0", "/dev/bd0", "/dev/bdvd", "/mnt/disc", "/mnt/disc0"};
    if (!p_lstat) { logline("lstat=unavailable\n"); return; }
    for (unsigned i=0; i<sizeof(paths)/sizeof(paths[0]); ++i) {
        struct stat st={0}; int rc=p_lstat(paths[i], &st);
        if (rc) logline("path=%s rc=%d errno=%d\n", paths[i], rc, err());
        else logline("path=%s mode=0%o rdev=%u\n", paths[i], (unsigned)st.st_mode, (unsigned)st.st_rdev);
    }
}
int __crt_start(payload_args_t *args) {
    for (unsigned char *p=__bss_start; p<__bss_end; ++p) *p=0;
    if (!args || !args->sys_dynlib_dlsym) return -1;
    resolve=args->sys_dynlib_dlsym;
    /* Current elfldr passes getpid, whose syscall instruction is at +0xa.
     * Only use this route when the instruction and syscall number match.
     * Older loaders may provide a callable dlsym function directly. */
    const unsigned char *stub=(const unsigned char *)resolve;
    if (stub[0]==0x48 && stub[1]==0xc7 && stub[2]==0xc0 &&
        stub[3]==20 && !stub[4] && !stub[5] && !stub[6] &&
        stub[10]==0x0f && stub[11]==0x05) {
        probe_syscall_instruction=(void *)(stub+10);
        resolve=probe_dynlib_dlsym;
    }
    startup_trace("DriveProbe: entry reached; verified loader syscall stub\n");
    lookup("sceKernelSendNotificationRequest", &p_notify);
    startup_trace(p_notify ? "DriveProbe: notification symbol resolved\n" : "DriveProbe: notification symbol unavailable\n");
    notification("DriveProbe: custom entry reached");
    if (LOAD(vsnprintf)) { startup_trace("DriveProbe: vsnprintf unavailable\n"); notification("DriveProbe: vsnprintf unavailable"); return -2; }
    lookup("__error", &p_error);
    lookup("sysctlbyname", &p_sysctl);
    LOAD(open); LOAD(lstat); LOAD(getdirentries);
    #define REQUIRED(name) if (LOAD(name)) { notification("DriveProbe: missing " #name); return -3; }
    REQUIRED(close); REQUIRED(socket); REQUIRED(bind); REQUIRED(listen);
    REQUIRED(accept); REQUIRED(fcntl); REQUIRED(usleep); REQUIRED(send);
    notification("DriveProbe: collecting metadata");
    logline("PS5-DriveProbe v0.1\npolicy=metadata only; no SDK CRT patches; no kernel memory; no device commands; no console disk writes\nregistration_state=NOT MEASURED\n");
    system_info(); notification("DriveProbe: OS metadata collected");
    device_names(); notification("DriveProbe: device names collected");
    path_metadata();
    logline("END PS5-DriveProbe\n");
    int server=p_socket(AF_INET, SOCK_STREAM, 0);
    if (server<0) return -4;
    struct sockaddr_in addr={0};
    addr.sin_len=sizeof(addr); addr.sin_family=AF_INET;
    addr.sin_port=(unsigned short)((9022>>8) | ((9022&255)<<8));
    if (p_bind(server, (struct sockaddr *)&addr, sizeof(addr)) || p_listen(server, 1) || p_fcntl(server, F_SETFL, O_NONBLOCK)<0) {
        p_close(server); notification("DriveProbe: cannot listen on port 9022"); return -5;
    }
    notification("DriveProbe: log ready on TCP 9022 for 60 seconds");
    int client=-1;
    for (int i=0; i<600 && client<0; ++i) {
        client=p_accept(server, 0, 0);
        if (client<0) p_usleep(100000);
    }
    int result=-6;
    if (client>=0) {
        p_fcntl(client, F_SETFL, O_NONBLOCK);
        size_t offset=0;
        for (int i=0; i<600 && offset<used; ++i) {
            ssize_t n=p_send(client, report+offset, used-offset, MSG_NOSIGNAL);
            if (n>0) offset+=(size_t)n;
            else if (n<0 && (err()==35 || err()==4)) p_usleep(100000);
            else break;
        }
        result=offset==used?0:-7;
        p_close(client);
    }
    p_close(server);
    if (args->payloadout) *args->payloadout=result;
    return result;
}

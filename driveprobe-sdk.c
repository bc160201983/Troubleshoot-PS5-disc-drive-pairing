/* Diagnostic: metadata only. Standard SDK runtime performs process setup. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ps5/kernel.h>
struct ddd_query { uint8_t pad[4]; int32_t in_size; uint8_t *in_buf; int32_t out_size; uint8_t *out_buf; uint32_t status; uint32_t pad2; };
_Static_assert(sizeof(struct ddd_query)==40,"DDD ABI must be 40 bytes");
int main(void) {
    FILE *f=fopen("/data/driveprobe-sdk.txt","w");
    if(!f) {printf("DriveProbe report open failed: %d\n",errno); return 1;}
    fprintf(f,"DriveProbe SDK baseline\nfirmware_hex=%08x\nscope=directory names and metadata; no registration commands\n",kernel_get_fw_version());
    DIR *d=opendir("/dev");
    if(d) {struct dirent *e; while((e=readdir(d))) fprintf(f,"dev_entry=%s\n",e->d_name); closedir(d);}
    else fprintf(f,"dev_listing_errno=%d\n",errno);
    const char *paths[]={"/dev/ddd","/dev/driveauth","/dev/icc_bddrive","/dev/cd0"};
    for(unsigned i=0;i<4;i++) {struct stat s; int r=lstat(paths[i],&s); if(r) fprintf(f,"path=%s errno=%d\n",paths[i],errno); else fprintf(f,"path=%s mode=%o rdev=%u\n",paths[i],(unsigned)s.st_mode,(unsigned)s.st_rdev);}
    /* Only published GET_STATUS; no nonce, register, check-register or SCSI. */
    int fd=open("/dev/ddd",O_RDONLY);
    if(fd<0) fprintf(f,"ddd_open_errno=%d\n",errno);
    else {
        uint32_t state=0xffffffff;
        struct ddd_query q={0}; q.out_size=4; q.out_buf=(uint8_t *)&state;
        errno=0; int rc=ioctl(fd,0xC0284406UL,&q); int saved_errno=errno;
        fprintf(f,"ddd_get_status_rc=%d errno=%d service_status=%08x state_raw=%08x\n",rc,saved_errno,q.status,state);
        if(!rc && !q.status && state<7) {
            const char *names[]={"PAIRED","PAIRED_TO_ANOTHER","NOT_PAIRED","REGISTER_INCOMPLETE","DELETE_INCOMPLETE","FACTORY_PAIRED","FACTORY_PAIRED_TO_ANOTHER"};
            fprintf(f,"ddd_pairing_state=%s (mapping from external research; not independently verified)\n",names[state]);
        }
        close(fd);
    }
    fprintf(f,"END DriveProbe SDK baseline\n");
    int fail=fflush(f); if(fsync(fileno(f))) fail=1; if(fclose(f)) fail=1;
    printf("DriveProbe baseline finished: %d\n",fail); return fail?1:0;
}


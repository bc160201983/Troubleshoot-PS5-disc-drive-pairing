"""Read FTP server identity and /dev listing; no device files are downloaded."""
import argparse
import datetime
import ftplib
from pathlib import Path

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host')
    parser.add_argument('label',choices=['fw700','fw1140'])
    args=parser.parse_args()
    lines=[f'operator_label={args.label}',f'source_ip={args.host}',
           'source=FTP directory listing (different process permissions from DriveProbe)',
           'registration_state=NOT MEASURED']
    with ftplib.FTP(timeout=5) as ftp:
        ftp.connect(args.host,2121)
        lines.append(ftp.getwelcome())
        ftp.login()
        try: lines.append(ftp.sendcmd('SYST'))
        except ftplib.error_perm as exc: lines.append(str(exc))
        lines.append('LIST /dev:')
        try: ftp.retrlines('LIST /dev',lines.append)
        except ftplib.all_errors as exc: lines.append(f'Listing failed: {exc}')
    folder=Path(__file__).with_name('logs'); folder.mkdir(exist_ok=True)
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    out=folder/f'ftp-baseline-{args.label}-{stamp}.txt'
    out.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(f'Saved {out}\n'+'\n'.join(lines))

if __name__=='__main__': main()

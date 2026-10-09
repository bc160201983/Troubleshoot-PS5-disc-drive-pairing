"""Send a loader-compatible ELF and retrieve its read-only report."""
import argparse
import datetime
import ipaddress
from pathlib import Path
import socket
import select
import time

def collect(host, elf, loader_port=9021, report_port=9022, timeout=65):
    ip = ipaddress.ip_address(host)
    if ip.version != 4 or not ip.is_private or ip.is_loopback or ip.is_unspecified:
        raise ValueError('Use the PS5 private LAN IPv4 address.')
    data = Path(elf).read_bytes()
    if data[:5] != b'\x7fELF\x02':
        raise ValueError('Expected a compiled 64-bit ELF payload.')
    with socket.create_connection((host, loader_port), timeout=5) as loader:
        loader.sendall(data)
        loader.shutdown(socket.SHUT_WR)
        print(f'Uploaded {len(data)} bytes to {host}:{loader_port}; waiting for report on {report_port}.', flush=True)
        return fetch_report(host, report_port, timeout, loader)

def fetch_report(host, report_port, timeout, loader=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if loader is not None and select.select([loader], [], [], 0)[0]:
            reply=loader.recv(4096)
            if reply:
                print('Loader response: '+reply.decode('utf-8',errors='replace').rstrip('\x00\r\n'),flush=True)
            else:
                loader=None
        try:
            conn = socket.create_connection((host, report_port), timeout=2)
        except OSError:
            time.sleep(0.25)
            continue
        with conn:
            conn.settimeout(10)
            result = bytearray()
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                result.extend(chunk)
                if len(result) > 65536:
                    raise ValueError('Report exceeds expected size.')
        text = result.decode('utf-8', errors='replace')
        if not text.startswith('PS5-DriveProbe v0.1\n') or not text.endswith('END PS5-DriveProbe\n'):
            raise ValueError('Incomplete or unexpected report; not saved as a successful capture.')
        return text
    raise TimeoutError('No DriveProbe report on 9022. Check loader ABI, screen notification, and LAN access.')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host')
    parser.add_argument('label', choices=['fw700', 'fw1140', 'unidentified'])
    parser.add_argument('--elf', default=str(Path(__file__).with_name('driveprobe.elf')))
    args = parser.parse_args()
    text = collect(args.host, args.elf)
    folder = Path(__file__).with_name('logs')
    folder.mkdir(exist_ok=True)
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    out = folder / f'driveprobe-{args.label}-{stamp}.txt'
    out.write_text(f'operator_label={args.label}\nsource_ip={args.host}\n{ text }', encoding='utf-8')
    print(f'Saved {out}\n{text}')

if __name__ == '__main__':
    main()

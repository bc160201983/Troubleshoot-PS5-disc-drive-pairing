"""Local transport checks; these do not validate PS5 runtime behavior."""
import socket
import tempfile
import threading
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
import collect

class TransportTest(unittest.TestCase):
    def exercise(self, report):
        payload=b'\x7fELF\x02'+b'local-test'
        received=[]
        listeners=[]
        for _ in range(2):
            s=socket.socket(); s.bind(('127.0.0.1',0)); s.listen(1); s.settimeout(5)
            listeners.append(s)
        loader, logger=listeners
        ports=[s.getsockname()[1] for s in listeners]
        def serve():
            with loader, logger:
                conn,_=loader.accept()
                with conn:
                    data=bytearray()
                    while True:
                        block=conn.recv(1024)
                        if not block: break
                        data.extend(block)
                    received.append(bytes(data))
                conn,_=logger.accept()
                with conn:
                    conn.sendall(report[:10]); conn.sendall(report[10:])
        worker=threading.Thread(target=serve,daemon=True); worker.start()
        with tempfile.TemporaryDirectory(dir=Path(__file__).parent) as folder:
            elf=Path(folder)/'test.elf'; elf.write_bytes(payload)
            # Only the LAN-address guard is replaced for this loopback test.
            with patch.object(collect.ipaddress,'ip_address',return_value=SimpleNamespace(version=4,is_private=True,is_loopback=False,is_unspecified=False)):
                try: result=collect.collect('127.0.0.1',elf,*ports,timeout=3)
                finally: worker.join(5)
        self.assertEqual(received,[payload])
        return result
    def test_complete_capture(self):
        report=b'PS5-DriveProbe v0.1\nregistration_state=NOT MEASURED\nEND PS5-DriveProbe\n'
        self.assertEqual(self.exercise(report),report.decode())
    def test_truncated_capture_rejected(self):
        with self.assertRaisesRegex(ValueError,'Incomplete'):
            self.exercise(b'PS5-DriveProbe v0.1\ncut off')
    def test_public_address_rejected(self):
        with self.assertRaises(ValueError): collect.collect('8.8.8.8','unused.elf')

if __name__=='__main__': unittest.main()

"""Regression checks for selecting the requested ELF and validating SDK imports."""
from contextlib import redirect_stdout
from pathlib import Path
import io
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from validate_elf import validate


def fixture(relocation=None, symbol=1):
    if relocation is None:
        data=bytearray(128)
        shoff=0
        shcount=0
        memsize=len(data)
    else:
        data=bytearray(464)
        shoff=208
        shcount=4
        memsize=528
        # Two dynamic symbols; symbol 1 is an undefined import named printf.
        struct.pack_into('<IBBHQQ',data,152,1,0x12,0,0,0,0)
        data[176:184]=b'\0printf\0'
        struct.pack_into('<QQq',data,184,512,(symbol<<32)|relocation,0)
        struct.pack_into('<IIQQQQIIQQ',data,shoff+64,0,11,0,128,128,48,2,1,8,24)
        struct.pack_into('<IIQQQQIIQQ',data,shoff+128,0,3,0,176,176,8,0,0,1,0)
        struct.pack_into('<IIQQQQIIQQ',data,shoff+192,0,4,0,184,184,24,1,0,8,24)
    ident=b'\x7fELF\x02\x01\x01'+bytes(9)
    struct.pack_into('<16sHHIQQQIHHHHHH',data,0,ident,3,62,1,0,64,shoff,0,64,56,1,64,shcount,0)
    struct.pack_into('<IIQQQQQQ',data,64,1,7,0,0,0,len(data),memsize,0x4000)
    return bytes(data)


class ValidateElfTests(unittest.TestCase):
    def test_cli_uses_requested_file_instead_of_default(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as directory:
            root=Path(directory)
            script=root/'validate_elf.py'
            shutil.copyfile(Path(__file__).with_name('validate_elf.py'),script)
            (root/'driveprobe.elf').write_bytes(b'not an ELF')
            requested=root/'requested.elf'
            requested.write_bytes(fixture())
            result=subprocess.run([sys.executable,str(script),str(requested)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertIn('requested.elf',result.stdout)

    def test_no_argument_preserves_default_file(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as directory:
            root=Path(directory)
            script=root/'validate_elf.py'
            shutil.copyfile(Path(__file__).with_name('validate_elf.py'),script)
            (root/'driveprobe.elf').write_bytes(fixture())
            result=subprocess.run([sys.executable,str(script)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertIn('driveprobe.elf',result.stdout)

    def test_import_relocations_require_sdk_mode(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as directory:
            path=Path(directory)/'sdk.elf'
            path.write_bytes(fixture(6))
            with self.assertRaisesRegex(AssertionError,'Unsupported relocation'):
                validate(path)
            with redirect_stdout(io.StringIO()):
                validate(path,allow_imports=True)

    def test_sdk_mode_rejects_invalid_import_symbol(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as directory:
            path=Path(directory)/'bad-symbol.elf'
            path.write_bytes(fixture(6,symbol=2))
            with self.assertRaisesRegex(AssertionError,'Invalid relocation symbol index'):
                validate(path,allow_imports=True)

    def test_sdk_mode_rejects_unknown_relocations(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as directory:
            path=Path(directory)/'unknown.elf'
            path.write_bytes(fixture(99))
            with self.assertRaisesRegex(AssertionError,'Unsupported relocation'):
                validate(path,allow_imports=True)


if __name__=='__main__':
    unittest.main()


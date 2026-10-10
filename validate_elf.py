"""Check loader-critical structure before transmitting a payload."""
from pathlib import Path
import argparse
import struct

def validate(path, allow_imports=False):
    data=Path(path).read_bytes()
    assert data[:6]==b'\x7fELF\x02\x01', 'Expected little-endian ELF64'
    header=struct.unpack_from('<16sHHIQQQIHHHHHH',data)
    _,kind,machine,_,entry,phoff,shoff,_,_,phsize,phcount,shsize,shcount,_=header
    assert kind==3 and machine==62, 'Expected x86-64 PIE'
    assert phsize==56 and shsize==64
    loads=[]
    for i in range(phcount):
        ptype,flags,offset,addr,_,filesz,memsz,align=struct.unpack_from('<IIQQQQQQ',data,phoff+i*phsize)
        if ptype==1:
            assert addr%0x4000==0, 'Loader mprotect requires page-aligned segment addresses'
            assert align==0x4000 and (offset-addr)%align==0
            assert filesz<=memsz and offset+filesz<=len(data)
            loads.append((addr,addr+memsz,flags))
    assert any(start<=entry<end and flags&1 for start,end,flags in loads)
    for i in range(shcount):
        _,stype,_,_,offset,size,link,_,_,entsize=struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*shsize)
        if stype==4:
            assert entsize==24
            for j in range(size//entsize):
                target,info,addend=struct.unpack_from('<QQq',data,offset+j*entsize)
                reltype=info & 0xffffffff
                symbol=info >> 32
                assert reltype==8 or (allow_imports and reltype in (1,6,7)), 'Unsupported relocation; use --sdk for native imports'
                assert any(start<=target and target+8<=end for start,end,_ in loads)
                if reltype==8:
                    assert symbol==0, 'RELATIVE relocation must not reference a symbol'
                    # A BSS-end sentinel may legitimately be one past its segment.
                    assert any(start<=addend<=end for start,end,_ in loads)
                else:
                    assert link<shcount, 'Invalid relocation symbol-table link'
                    symsec=struct.unpack_from('<IIQQQQIIQQ',data,shoff+link*shsize)
                    assert symsec[1] in (2,11) and symsec[9]==24
                    assert symbol<symsec[5]//24, 'Invalid relocation symbol index'
    print(f'ELF validation passed: {path}: {len(data)} bytes, {len(loads)} aligned load segments.')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path',nargs='?',type=Path,default=Path(__file__).with_name('driveprobe.elf'))
    parser.add_argument('--sdk',action='store_true',help='Accept native SDK import relocations as well as RELATIVE relocations')
    args=parser.parse_args()
    validate(args.path,allow_imports=args.sdk)

"""Check loader-critical structure before transmitting a payload."""
from pathlib import Path
import struct

def validate(path):
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
        _,stype,_,_,offset,size,_,_,_,entsize=struct.unpack_from('<IIQQQQIIQQ',data,shoff+i*shsize)
        if stype==4:
            assert entsize==24
            for j in range(size//entsize):
                target,info,addend=struct.unpack_from('<QQq',data,offset+j*entsize)
                assert info==8, 'Payload should contain only RELATIVE relocations'
                assert any(start<=target and target+8<=end for start,end,_ in loads)
                # A BSS-end sentinel may legitimately be one past its segment.
                assert any(start<=addend<=end for start,end,_ in loads)
    print(f'ELF validation passed: {len(data)} bytes, {len(loads)} aligned load segments.')

if __name__=='__main__': validate(Path(__file__).with_name('driveprobe.elf'))

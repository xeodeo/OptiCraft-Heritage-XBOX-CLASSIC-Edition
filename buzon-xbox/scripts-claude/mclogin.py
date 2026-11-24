import socket,struct,sys,json,zlib
host,port,proto=sys.argv[1],int(sys.argv[2]),int(sys.argv[3])
sys.stdout.reconfigure(encoding='utf-8')
def varint(n):
    out=b''
    while True:
        b=n&0x7f; n>>=7
        out+=bytes([b|(0x80 if n else 0)])
        if not n: return out
def rv(s):
    n=0;sh=0
    while True:
        c=s.recv(1)
        if not c: raise EOFError
        b=c[0]; n|=(b&0x7f)<<sh; sh+=7
        if not b&0x80: return n
s=socket.create_connection((host,port),timeout=10)
hs=varint(0)+varint(proto)+varint(len(host))+host.encode()+struct.pack('>H',port)+varint(2)
s.sendall(varint(len(hs))+hs)
name=b'OCtestXbox'
ls=varint(0)+varint(len(name))+name
s.sendall(varint(len(ls))+ls)
l=rv(s); data=b''
while len(data)<l: data+=s.recv(l-len(data))
pid=data[0]
print('proto',proto,'-> packet id',pid, data[1:300])

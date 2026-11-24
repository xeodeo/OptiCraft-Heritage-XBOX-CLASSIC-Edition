import socket,struct,json,sys
host,port=sys.argv[1],int(sys.argv[2])
def varint(n):
    out=b''
    while True:
        b=n&0x7f; n>>=7
        out+=bytes([b|(0x80 if n else 0)])
        if not n: return out
def rv(s):
    n=0;sh=0
    while True:
        b=s.recv(1)[0]; n|=(b&0x7f)<<sh; sh+=7
        if not b&0x80: return n
s=socket.create_connection((host,port),timeout=8)
hs=varint(0)+varint(47)+varint(len(host))+host.encode()+struct.pack('>H',port)+varint(1)
s.sendall(varint(len(hs))+hs); s.sendall(varint(1)+varint(0))
rv(s); rv(s); l=rv(s); data=b''
while len(data)<l: data+=s.recv(l-len(data))
d=json.loads(data)
import sys; sys.stdout.reconfigure(encoding="utf-8")
print(d.get('version'), d.get('players',{}).get('online'),'/',d.get('players',{}).get('max'))
print(str(d.get('description'))[:150])

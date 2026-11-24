# Fake 1.12.2 server: replays a recorded session (h340 harness .rec file) to
# whatever connects, so the game can be tested against real SMP data offline.
import socket, struct, sys, zlib, time, threading

REC = sys.argv[1]
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 25567

def varint(v):
    out = b''
    while True:
        b = v & 0x7f
        v >>= 7
        if v:
            out += bytes([b | 0x80])
        else:
            return out + bytes([b])

def read_varint(sock):
    v = 0
    for i in range(5):
        b = sock.recv(1)
        if not b:
            raise EOFError
        v |= (b[0] & 0x7f) << (7 * i)
        if not b[0] & 0x80:
            return v
    raise ValueError

def frames():
    d = open(REC, 'rb').read()
    i = 0
    while i < len(d):
        L = struct.unpack('<I', d[i:i + 4])[0]
        i += 4
        yield d[i:i + L]
        i += L

def serve(conn):
    try:
        conn.settimeout(120)
        # handshake + login start (or status request)
        n = read_varint(conn); hs = conn.recv(n)
        state = hs[-1]
        if state == 1:
            n = read_varint(conn); conn.recv(n)
            js = b'{"version":{"name":"1.12.2","protocol":340},"players":{"max":75,"online":32},"description":{"text":"replay SMP"}}'
            body = varint(0) + varint(len(js)) + js
            conn.sendall(varint(len(body)) + body)
            return
        n = read_varint(conn); conn.recv(n)
        compressed = False
        count = 0
        for f in frames():
            if not compressed:
                conn.sendall(varint(len(f)) + f)
                if f[0] == 0x03:
                    compressed = True
            else:
                if len(f) >= 256:
                    z = zlib.compress(f)
                    inner = varint(len(f)) + z
                else:
                    inner = varint(0) + f
                conn.sendall(varint(len(inner)) + inner)
            count += 1
            if count % 50 == 0:
                time.sleep(0.02)
        print('replayed', count, 'frames', flush=True)
        # keep the connection alive with keepalives
        while True:
            time.sleep(5)
            f = varint(0x1F) + struct.pack('>q', int(time.time()))
            inner = varint(0) + f
            conn.sendall(varint(len(inner)) + inner)
    except Exception as e:
        print('conn end', e, flush=True)
    finally:
        conn.close()

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('0.0.0.0', PORT))
s.listen(4)
print('listening', PORT, flush=True)
while True:
    c, a = s.accept()
    print('client', a, flush=True)
    threading.Thread(target=serve, args=(c,), daemon=True).start()

# Fake 1.12.2 server for the "crash after dying on the SMP" repro (encargo 125):
# replays the recorded SMP session, then sends what the SMP sent on respawn:
# Respawn into dimension -1 and immediately back to 0 (each with a teleport),
# then resends the recorded chunk data so the client has terrain again.
import socket, struct, sys, zlib, time

REC = sys.argv[1]
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 25567
DELAY = float(sys.argv[3]) if len(sys.argv) > 3 else 12.0
ROUNDS = int(sys.argv[4]) if len(sys.argv) > 4 else 3


def varint(v):
    out = b''
    while True:
        b = v & 0x7f
        v >>= 7
        if v:
            out += bytes([b | 0x80])
        else:
            return out + bytes([b])


def read_varint_bytes(d, i):
    v = 0
    for k in range(5):
        b = d[i]
        i += 1
        v |= (b & 0x7f) << (7 * k)
        if not b & 0x80:
            return v, i
    raise ValueError


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


def send_c(conn, f):
    if len(f) >= 256:
        inner = varint(len(f)) + zlib.compress(f)
    else:
        inner = varint(0) + f
    conn.sendall(varint(len(inner)) + inner)


def mcstr(s):
    b = s.encode()
    return varint(len(b)) + b


def respawn(dim):
    return varint(0x35) + struct.pack('>iBB', dim, 1, 0) + mcstr('default')


def poslook(x, y, z, tid):
    return varint(0x2F) + struct.pack('>dddffb', x, y, z, 0.0, 0.0, 0) + varint(tid)


def serve(conn):
    try:
        conn.settimeout(300)
        n = read_varint(conn); hs = conn.recv(n)
        if hs[-1] == 1:
            n = read_varint(conn); conn.recv(n)
            js = b'{"version":{"name":"1.12.2","protocol":340},"players":{"max":75,"online":1},"description":{"text":"respawn repro"}}'
            body = varint(0) + varint(len(js)) + js
            conn.sendall(varint(len(body)) + body)
            return
        n = read_varint(conn); conn.recv(n)
        compressed = False
        chunks = []
        spawn = (0.0, 80.0, 0.0)
        for f in frames():
            pid, _ = read_varint_bytes(f, 0)
            if not compressed:
                conn.sendall(varint(len(f)) + f)
                if f[0] == 0x03:
                    compressed = True
                continue
            send_c(conn, f)
            if pid == 0x20:
                chunks.append(f)
            elif pid == 0x2F and len(f) > 25:
                _, j = read_varint_bytes(f, 0)
                spawn = struct.unpack('>ddd', f[j:j + 24])
        print('replayed; chunks', len(chunks), 'spawn', spawn, flush=True)
        tid = 1000
        for r in range(ROUNDS):
            time.sleep(DELAY)
            print('round', r, ': respawn -1 then 0', flush=True)
            send_c(conn, respawn(-1))
            send_c(conn, poslook(spawn[0], spawn[1], spawn[2], tid)); tid += 1
            send_c(conn, respawn(0))
            send_c(conn, poslook(spawn[0], spawn[1], spawn[2], tid)); tid += 1
            for k, f in enumerate(chunks):
                send_c(conn, f)
                if k % 40 == 0:
                    time.sleep(0.02)
        print('done rounds', flush=True)
        while True:
            time.sleep(5)
            send_c(conn, varint(0x1F) + struct.pack('>q', int(time.time())))
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
    serve(c)

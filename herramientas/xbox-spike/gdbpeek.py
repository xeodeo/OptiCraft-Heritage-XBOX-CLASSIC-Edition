"""Minimal GDB remote-protocol client for xemu's gdbstub (-s, tcp:1234).

Samples the guest x86 registers a few times, resolves EIP against a linker
map file, and dumps the top of the stack resolved the same way, to find where
an Xbox title is hung or spinning.

usage: python gdbpeek.py <map file> [samples]
"""
import bisect
import re
import socket
import sys
import time

REGS = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "eip", "eflags"]


def load_map(path):
    syms = []
    pat = re.compile(r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+\S*\s*(\S*)")
    with open(path, errors="replace") as f:
        for line in f:
            m = pat.match(line)
            if m:
                syms.append((int(m.group(2), 16), m.group(1), m.group(3)))
    syms.sort()
    return syms


def resolve(syms, addr, keys):
    i = bisect.bisect_right(keys, addr) - 1
    if i < 0:
        return "?"
    a, name, obj = syms[i]
    return f"{name}+0x{addr - a:x} ({obj})"


class Gdb:
    def __init__(self, host="127.0.0.1", port=1235):
        self.s = socket.create_connection((host, port), timeout=5)
        self.buf = b""

    def _read_packet(self):
        while True:
            while b"#" not in self.buf or len(self.buf) < self.buf.index(b"#") + 3:
                self.buf += self.s.recv(65536)
            start = self.buf.find(b"$")
            end = self.buf.index(b"#", start)
            payload = self.buf[start + 1:end]
            self.buf = self.buf[end + 3:]
            self.s.sendall(b"+")
            return payload.decode(errors="replace")

    def cmd(self, text):
        csum = sum(text.encode()) & 0xFF
        self.s.sendall(f"${text}#{csum:02x}".encode())
        # swallow acks
        while True:
            if not self.buf:
                self.buf += self.s.recv(65536)
            if self.buf[:1] == b"+":
                self.buf = self.buf[1:]
                continue
            break
        return self._read_packet()

    def interrupt(self):
        self.s.sendall(b"\x03")
        return self._read_packet()

    def regs(self):
        raw = self.cmd("g")
        # A stale stop reply (T../S..) can precede the register dump when the
        # previous session left a continue in flight; read past it.
        for _ in range(4):
            if len(raw) >= 80 and all(c in "0123456789abcdefABCDEFxX" for c in raw[:80]):
                break
            raw = self._read_packet()
        out = {}
        for i, name in enumerate(REGS):
            chunk = raw[i * 8:(i + 1) * 8]
            out[name] = int.from_bytes(bytes.fromhex(chunk), "little")
        return out

    def read(self, addr, length):
        raw = self.cmd(f"m{addr:x},{length:x}")
        if raw.startswith("E"):
            return None
        return bytes.fromhex(raw)


def main():
    syms = load_map(sys.argv[1])
    keys = [s[0] for s in syms]
    samples = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    g = Gdb()
    print('stop reason:', g.cmd('?'))
    for n in range(samples):
        if n > 0:
            g.interrupt()
        r = g.regs()
        print(f"[{n}] eip={r['eip']:08x} {resolve(syms, r['eip'], keys)}")
        print("     " + " ".join(f"{k}={r[k]:08x}" for k in REGS if k != "eip"))
        if n == samples - 1:
            stack = g.read(r["esp"], 256)
            if stack:
                print("     stack words that resolve into the image:")
                for i in range(0, len(stack), 4):
                    w = int.from_bytes(stack[i:i + 4], "little")
                    if keys and keys[0] <= w < keys[-1] + 0x10000:
                        print(f"       esp+{i:03x}: {w:08x} {resolve(syms, w, keys)}")
        g.s.sendall(b"$c#63")
        time.sleep(0.5)


if __name__ == "__main__":
    main()

import sys
p = sys.argv[1] + '/src/net/minecraft/src/ProtocolTranslator340Packets.cpp'
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
a = '''			if (flags & 0x01) x += s.posX;
			if (flags & 0x02) y += s.posY;
			if (flags & 0x04) z += s.posZ;'''.replace('\n', nl)
assert s.count(a) == 1
b = '''			const double prevX = s.posX, prevY = s.posY, prevZ = s.posZ;
			if (flags & 0x01) x += s.posX;
			if (flags & 0x02) y += s.posY;
			if (flags & 0x04) z += s.posZ;
			// Server corrections (rubber-banding near slabs, 23-11): log how far the
			// server moved us and from where, to see which movement it rejects.
			MC_LOG_WARN("net340", "server teleport flags=%u from %.3f %.3f %.3f to %.3f %.3f %.3f (dy=%.3f)\\n",
			            static_cast<unsigned>(flags), prevX, prevY, prevZ, x, y, z, y - prevY);'''.replace('\n', nl)
s = s.replace(a, b)
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')

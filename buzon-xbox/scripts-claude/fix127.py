import sys
p = sys.argv[1] + '/src/net/minecraft/src/ProtocolTranslator340.cpp'
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
a = '''	switch (id)
	{
	case 1: // granite/diorite/andesite'''.replace('\n', nl)
assert s.count(a) == 1
b = '''	// 1.2.5 has no top-half slabs and no upside-down stairs. Shown as a bottom slab /
	// normal stair, the client collided half a block lower than the server, so the
	// player stuck to or bounced off them (user 23-11 on the 1.12.2 SMP). Walking ON
	// them is what matters: a top slab becomes the full double slab of its material and
	// an upside-down stair the full block of its material.
	if ((meta & 8) != 0 && (id == 44 || id == 126 || id == 182 || id == 205))
	{
		uint32_t sm = meta & 7;
		if (id == 44)
			sm = sm == 6 ? 4 : (sm == 7 ? 0 : sm);
		else if (id == 126)
			sm = 2;
		else if (id == 182)
			sm = 1;
		else
			sm = 0;
		outId = 43;
		outMeta = static_cast<uint8_t>(sm);
		return;
	}
	if ((meta & 4) != 0)
	{
		uint8_t full = 0, fullMeta = 0;
		switch (id)
		{
		case 53: case 134: case 135: case 136: case 163: case 164: full = 5; break; // wooden -> planks
		case 67: full = 4; break;                 // cobblestone
		case 108: full = 45; break;               // brick
		case 109: case 203: full = 98; break;     // stone brick (purpur -> stone brick)
		case 114: full = 112; break;              // nether brick
		case 128: case 180: full = 24; break;     // sandstone
		case 156: full = 98; break;               // quartz -> stone brick
		default: break;
		}
		if (full != 0)
		{
			outId = full;
			outMeta = fullMeta;
			return;
		}
	}
	switch (id)
	{
	case 1: // granite/diorite/andesite'''.replace('\n', nl)
s = s.replace(a, b)
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')

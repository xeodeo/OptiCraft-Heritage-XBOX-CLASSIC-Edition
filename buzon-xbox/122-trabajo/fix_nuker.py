import sys
p = sys.argv[1]
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
s = s.replace('\r\n', '\n')
old = '''            s_px = std::floor(p->posX); s_pz = std::floor(p->posZ); s_py = std::floor(p->boundingBox->minY);
            s_count0 = probeCountBlocks(w, (int)s_px, (int)s_py, (int)s_pz);
'''
new = '''            s_px = std::floor(p->posX); s_pz = std::floor(p->posZ); s_py = std::floor(p->boundingBox->minY);
            for (int dx = 2; dx <= 3; ++dx)
                for (int dy = 0; dy <= 1; ++dy)
                    for (int dz = -1; dz <= 1; ++dz)
                        w->setBlockAndMetadataWithNotify((int)s_px + dx, (int)s_py + dy, (int)s_pz + dz, 3, 0);
            s_count0 = probeCountBlocks(w, (int)s_px, (int)s_py, (int)s_pz);
'''
assert s.count(old) == 1
s = s.replace(old, new)
open(p, 'w', encoding='utf-8', newline='').write(s.replace('\n', nl))
print('ok')

import struct

pak = r'C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\pak-prueba\assets.pak'
with open(pak, 'rb') as f:
    magic, ver, count, tbl_off, names_off, names_len, align, _ = struct.unpack('>4sIIIIIII', f.read(32))
    print('Magic:', magic)
    print('Version:', ver)
    print('Entry count:', count)
    print('Table offset:', tbl_off)
    print('Names offset:', names_off)
    print('Names length:', names_len)
    print('Align:', align)
    
    f.seek(tbl_off)
    entries = [struct.unpack('>IIII', f.read(16)) for _ in range(count)]
    f.seek(names_off)
    names_block = f.read(names_len)
    
    keys = []
    for h, n_off, d_off, sz in entries:
        name = names_block[n_off:].split(b'\0')[0].decode('utf-8')
        keys.append(name)
    
    # Check required keys
    required = [
        'assets/terrain.png',
        'assets/gui/buttons_xbox.png',
        'assets/legacy/crafting_icons.png',
        'assets/skins/LegacySteve.png',
    ]
    
    # Check for at least one resources/newmusic/*.ogg
    newmusic_keys = [k for k in keys if k.startswith('resources/newmusic/') and k.endswith('.ogg')]
    
    print()
    print('=== Required keys check ===')
    for req in required:
        found = req in keys
        status = 'YES' if found else 'NO'
        print('  {}: {}'.format(req, status))
    
    print()
    print('resources/newmusic/*.ogg found:', len(newmusic_keys))
    for k in newmusic_keys[:5]:
        print('  {}'.format(k))
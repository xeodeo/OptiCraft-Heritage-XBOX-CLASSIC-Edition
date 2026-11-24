import sys
for p in sys.argv[1:]:
    b = open(p, 'rb').read()
    b = b.replace(b'"\xc3\x82\xc2\xa7"', b'"\\xc2\\xa7"')
    open(p, 'wb').write(b)
    print(p, b.count(b'\xc3'))

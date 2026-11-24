import sys,re,bisect
mp=sys.argv[1]; addrs=[int(a,16) for a in sys.argv[2:]]
syms=[]
for line in open(mp,errors='ignore'):
    m=re.match(r'\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+(f\s+)?(\S+)',line)
    if m: syms.append((int(m.group(2),16),m.group(1),m.group(4)))
syms.sort()
keys=[s[0] for s in syms]
for a in addrs:
    i=bisect.bisect_right(keys,a)-1
    if i>=0: print(hex(a),syms[i][1][:90],'+'+hex(a-syms[i][0]),syms[i][2])

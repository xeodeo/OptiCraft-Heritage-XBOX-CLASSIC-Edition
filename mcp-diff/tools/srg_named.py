import csv,sys,re
conf=sys.argv[1]
def load(n):
    with open(conf+'/'+n,encoding='utf-8') as f:
        return {r['searge']:r['name'] for r in csv.DictReader(f)}
F=load('fields.csv'); M=load('methods.csv')
out=[]
for line in open(conf+'/joined.srg',encoding='utf-8'):
    line=line.split(' #')[0].rstrip('\n')
    p=line.split(' ')
    if p[0]=='FD:':
        own,_,n=p[2].rpartition('/'); p[2]=own+'/'+F.get(n,n)
    elif p[0]=='MD:':
        own,_,n=p[3].rpartition('/'); p[3]=own+'/'+M.get(n,n)
    out.append(' '.join(p))
open(conf+'/joined-named.srg','w',encoding='utf-8').write('\n'.join(out)+'\n')
print(len(out))

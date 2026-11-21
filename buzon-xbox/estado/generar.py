# Genera buzon-xbox/ESTADO-OPTICRAFT.html desde los .json de esta carpeta (uno por tarea + _meta.json).
# Uso: python -I generar.py    (luego abrir el HTML). Estados: probado, por_probar, en_marcha, falta, fallo, aparcado.
import json, os, glob
d = os.path.dirname(os.path.abspath(__file__))
tareas = []
for p in sorted(glob.glob(os.path.join(d, '*.json'))):
    if os.path.basename(p) == '_meta.json':
        continue
    t = json.load(open(p, encoding='utf-8'))
    t['id'] = os.path.basename(p)[:-5]
    tareas.append(t)
meta = json.load(open(os.path.join(d, '_meta.json'), encoding='utf-8'))
datos = json.dumps({'tareas': tareas, 'meta': meta}, ensure_ascii=False).replace('</', r'<\/')
html = open(os.path.join(d, 'plantilla.html'), encoding='utf-8').read().replace('/*DATOS*/', datos)
out = os.path.join(os.path.dirname(d), 'ESTADO-OPTICRAFT.html')
open(out, 'w', encoding='utf-8').write(html)
print(out, len(tareas), 'tareas')

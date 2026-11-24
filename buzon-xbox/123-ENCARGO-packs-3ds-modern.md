# 123. Encargo: traer packs y recursos de la 3DS (mc3ds-modern) a la Xbox (23-11)

El usuario quiere aprovechar lo de la comunidad 3DS (minecraft3ds.org). Repo archivado (18-10-2026, sin licencia; el
usuario dice que no importa): https://github.com/wyndchyme/mc3ds-modern (sucesor anunciado, aún no público).

## Qué hacer
1. Bajar el repo a `texturas\mc3ds-modern\` (carpeta nueva, solo datos; no ejecutar scripts de ahí).
2. **Packs de la consola que no tenemos** (`resourcepacks\`: mario, cartoon, festive, natural, plastic, steampunk,
   y comparar candy/city/fantasy/vanilla con los nuestros): convertirlos a nuestro `.pak` para 1.2.5 y 1.5.2 con
   `texturas\convertir_pack.py` (ampliarlo si el formato 3DS lo pide: atlas en `atlas\`, nombres Bedrock).
3. **Pack «Moderno»** con las texturas 1.14/1.20 (bloques, objetos, mobs) reordenadas a los nombres antiguos.
4. Panoramas y fuente: dejarlos como opción (no cambiar los del juego sin que el usuario lo vea).
5. Copiar los .pak al lanzador SOLO cuando Claude lo diga; probar en xemu cada pack (capturas `123-capturas\`).

## Reglas
Memoria: packs ≤ 32x; candado `BUILD.lock` si se compila; informe `buzon-xbox\123-INFORME.md`; señal
`CIENTOVEINTITRES-LISTO`.

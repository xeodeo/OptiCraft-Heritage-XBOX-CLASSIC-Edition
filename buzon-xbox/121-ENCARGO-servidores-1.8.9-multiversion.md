# 121. Encargo: entrar a servidores 1.8.9 (y multiversión) desde la Xbox (23-11)

Pedido del usuario (23-11): que el cliente de Xbox pueda jugar en servidores de **1.2.5, 1.5.2 y 1.8.9**, como hace
erluza en la 3DS/PS2 («se conecta a server 1.8.9 pero mejorado»).

## Lo que ya existe (erluza, rama `erluza/main` en el repo `OptiCraftHeritageEdition`, remoto `erluza`)
- `4fa0bd38` proxy externo en Python 1.2.5→1.8.9 (MVP, solo referencia).
- `4fd75801` «1.8 server support»: traductor DENTRO del cliente: `ProtocolTranslator189.{h,cpp}`,
  `ProtocolTranslator189IO.{h,cpp}`, cambios en `NetworkManager`, `NetClientHandler`, `GuiMultiplayer`,
  `ProtocolVersion.h`, tests `tests/ProtocolTranslator189Tests.cpp`.
- `65c980c1` «feat(ps2): add Minecraft 1.8.9 multi-version support»: versión mejorada (+2287 líneas en el traductor),
  cifrado `EncryptionAdapter_189`, `CryptoStreamBuf`, autenticación `MbedtlsAuthTransport`, `PlatformAuth`.
- Ver también commits posteriores de `erluza/main` que toquen esos ficheros (`git log erluza/main -- src/net/minecraft/src/ProtocolTranslator189.cpp`).
- Nosotros YA tenemos mbedTLS 3.6.2 en Xbox (`src/xbox/system/XboxTls.*`, `external/mbedtls`, `src/xbox/system/mbedtls_config.h`):
  reutilizarlo para el cifrado AES/CFB8 y RSA del login 1.8.9 (añadir al config lo que falte: AES, CFB, RSA, SHA1).

## Qué hacer
1. **1.2.5** (rama desde `opti30-erluza`): portar el traductor 1.8.9 de erluza (la versión de `65c980c1` o posterior) a
   Xbox. En «Multijugador», al añadir/editar servidor: versión del servidor (1.2.5 / 1.8.9 / Auto por ping), en las dos
   interfaces. Servidores en modo offline primero; online-mode (cuentas Microsoft) solo si erluza ya lo resolvió y cabe.
2. **1.5.2** (rama desde `port152-int`): adaptar el traductor a los paquetes del cliente 1.5.2 (protocolo 61) para
   entrar a 1.8.9. Después, si cabe, traducción hacia abajo para entrar a servidores 1.2.5 (protocolo 29).
3. Memoria: 64 MB; el traductor no debe copiar chunks enteros más de una vez (1.8.9 manda chunks en otro formato:
   convertir sección a sección). 60 fps en juego.
4. Probar con servidores en el PC: `servidor-1.2.5\iniciar.bat` (ya hay), y montar uno 1.8.9 portable igual
   (Java 8 en `servidor-1.2.5\jre8`; jar oficial 1.8.9 de Mojang) en `servidor-1.8.9\`, `online-mode=false`.

## Reglas
Candado `buzon-xbox\BUILD.lock`; commits con `git add <ficheros>`; sin push ni lanzador; nunca /fp:fast; dos interfaces;
mando; pantalla partida. Informe `buzon-xbox\121-INFORME.md`; señal de fin `CIENTOVEINTIUNO-LISTO` + commits.

## OBJETIVO CONCRETO DEL USUARIO (23-11): el SMP de la comunidad 3DS — medido por Claude
- Java: `minecraft3ds.org` (SRV → `usc-tx-p30-a11-s.kineticpanel.net:25568`): **Paper 1.12.2, protocolo 340**,
  32/75 jugadores. **Offline-mode** (tras el Login Start manda Set Compression umbral 256, sin Encryption Request).
  Con protocolo 47 (1.8.9) responde «Outdated client! Please use 1.12.2»: NO acepta 1.8.9.
- PE/3DS: `98.142.15.103:25685` (RakNet, MCPE 1.1.5 protocolo 113): es Bedrock, no nos sirve.
- Por tanto el traductor debe llegar a **1.12.2 (340)**: partir del 1.8.9 de erluza y subir a 1.9–1.12 (chunks con
  paleta por sección, teleport confirm, IDs de paquetes 1.12.2, metadatos de entidades, bloques nuevos → equivalentes
  antiguos más parecidos). Compresión zlib (ya tenemos zlib). Sin cifrado ni cuenta para este servidor.
- Probar contra ese servidor con cuidado (no hacer spam de conexiones: tiene «Connection throttled»), y antes contra un
  Paper 1.12.2 local en `servidor-1.12.2\` (offline-mode).

## REFERENCIA ENCONTRADA (Claude, 23-11): cliente 1.12 en C++ para 3DS, código público
- **Re-Craft-3DS** https://github.com/SomeRandoLameo/Re-Craft-3DS (GPL-3.0, C++20, archivado 05-10-2026): cliente
  Minecraft Java 1.10–1.12.x en 3DS, servidores offline; usa zlib, mbedTLS y curl.
- Su red viene de **mclib-3ds** https://github.com/SomeRandoLameo/mclib-3ds, port de **plushmonkey/mclib**
  https://github.com/plushmonkey/mclib (biblioteca C++ del protocolo Java: paquetes 1.10–1.12.2, chunks con paleta,
  compresión, cifrado). Ejemplo de uso: https://github.com/plushmonkey/Terracotta.
- Usarlo como referencia (o base) para la parte 1.12.2 del traductor: definiciones de paquetes de protocolo 340 y
  decodificación de secciones con paleta, adaptando a nuestra memoria (64 MB) y a nuestro mbedTLS/zlib.
- La actualización fan «NuMC3DS 1.10» (ItsPoofy, en la UniStore) que entra al SMP no tiene código público.

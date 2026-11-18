---
name: ps3-mando-xbox-plugin
description: Plugin mando Xbox/Mamba en PS3 (DS45PAD modificado) + personalización XEODEO de HEN y coldboot; estado completo en ps3\buzon\50-MANDO-XBOX-PLUGIN.txt
metadata:
  type: project
---
Estado al 18-11-2026; el detalle está en ps3\buzon\50-MANDO-XBOX-PLUGIN.txt (leerlo primero).
- El Mamba por receptor funciona. MK9 va perfecto gracias al módulo "mando oficial" (ds45pad_game.sprx, lo carga PS3MAPI) y a STICK_DPAD.
- El cuelgue de HEN en el navegador se resolvió quitando el plugin de boot_plugins.txt: ahora lo carga webMAN desde /dev_hdd0/autoexec.bat. El plugin está partido en cargador (ds45pad_vsh.sprx) + plugin real (ds45pad_main.sprx).
- XEODEO: el mensaje "Bienvenido XEODEO" está en el henplugin de PS3HEN.BIN; hay dos copias, en flash y en hdd, y la que se usa es la de flash. El logo del coldboot.raf también se cambió. Los originales quedan al lado como *_ORIG.
- Para escribir en la flash: webMAN /dev_blind?1, copy.ps3 a otro nombre, verificar y renombrar por FTP (copy.ps3 no sobrescribe).
- Pendiente: mapeo de Bluetooth (BT_APRENDER=1) y el texto "cobra" del encendido.
Relacionado: [[ps3-sdk]] [[ps3-subidas-juego-abierto]]

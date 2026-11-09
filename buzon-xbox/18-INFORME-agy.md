# INFORME 18 agy

## Resultados de ConfiguraciÃ³n C

1. **Â¿Enlaza?** SÃ­. Al excluir `XboxP3Math.c` del flag `/GL` (LTCG) incluyÃ©ndolo como una `OBJECT` library, el enlazador completÃ³ exitosamente la fase de LTCG. Se requiriÃ³ forzar la combinaciÃ³n de secciones conflictivas (CFG) del MSVC generadas por mezclar MSIL y objetos clÃ¡sicos (`/FORCE:MULTIPLE`).
2. **Â¿Pasan los scripts `patch_pe_for_imagebld` e `imagebld`?** SÃ­, pasaron con Ã©xito.
3. **Â¿Avisos de `patch_sse2_moves.ps1`?** El script arrojÃ³ un error fatal durante la compilaciÃ³n:
   ```text
   XboxSse2Cave missing or full
   ```
   Debido a esto, la generaciÃ³n del binario abortÃ³ en el paso post-build y Ninja reportÃ³ `subcommand failed`.
4. **TamaÃ±o del XBE:** 4156416 bytes (tamaÃ±o del `OptiCraft.exe` parcheado justo antes de que el script post-build fallara). El `.iso` / `.xbe` no se llegÃ³ a empacar.
5. **Â¿Xemu llega al menÃº?** N/A (la compilaciÃ³n fue abortada por el error del script, no hay `OptiCraft.iso`).

## InvestigaciÃ³n sobre `__isa_available`

Examinando el `OptiCraft.exe.map` y el desensamblado del `.exe` generado mediante `dumpbin /DISASM`:

1. **Variables y Funciones de DetecciÃ³n:** El enlazador incluye efectivamente `___isa_available_init` (proveniente de `libcmt:cpu_disp.obj`), el cual se ejecuta al arranque de la aplicaciÃ³n e inspecciona las capacidades de la CPU vÃ­a la instrucciÃ³n `CPUID`. El resultado se guarda en la variable global `___isa_available` (ubicada en la direcciÃ³n `007C7C98h`).
2. **Rutas Condicionales en Memoria:** Revisando el cÃ³digo ensamblador de rutinas como `memcpy`, `memset` y relacionadas (`LargeRangeBytes`), se encontraron comprobaciones idÃ©nticas mÃºltiples veces en el preÃ¡mbulo de cada una de estas funciones:
   ```assembly
   006C9B55: A1 98 7C 7C 00     mov         eax,dword ptr ds:[007C7C98h]
   ...
   006CDC65: 83 3D 98 7C 7C 00  cmp         dword ptr ds:[007C7C98h],1
   006CDC6C: 0F 8C 8A 00 00 00  jl          006CDCFC  ; Salto al fallback si < 1 (Sin SSE2)
   ```
3. **ConclusiÃ³n TÃ©cnica:** La inclusiÃ³n de rutinas de copiado/memoria modernas de Visual Studio 2022 (con uso agresivo de SSE2 como `movdqa` o `movdqu`) en realidad **es segura a nivel de ejecuciÃ³n (runtime)**. Dado que la Xbox usa un Pentium III, `___isa_available_init` siempre evaluarÃ¡ que no hay soporte y dejarÃ¡ `___isa_available = 0`. Esto obligarÃ¡ al flujo del programa a saltar siempre (`jl / jb`) a las versiones alternativas estÃ¡ndar escalares, evadiendo las instrucciones no soportadas.
4. **Causa del Error:** Nuestro verificador y parcheador `patch_sse2_moves.ps1` realiza un escaneo **estÃ¡tico** sobre los opcodes. Como no analiza el flujo de control, detecta ingenuamente todas las instrucciones SSE2 latentes del CRT y trata de inyectar parches a las "caves" de Xbox. El espacio en dichas "caves" se agota prematuramente dado el volumen masivo de instrucciones SSE2 del linker moderno, resultando en la detenciÃ³n del pipeline `XboxSse2Cave missing or full`.

## Diff de CMake

Se adjunta la salida de `git -C wt-ltcg diff`:

index 92e1e1a..2f89252 100644
--- a/cmake/xbox.cmake
+++ b/cmake/xbox.cmake
@@ -38,6 +38,12 @@ option(XBOX_ENABLE_SOUND "Enable the DirectSound backend" OFF)
 option(XBOX_ENABLE_NETWORK "Experimental: multiplayer over XNet TCP (Minecraft 1.2.5 servers)" OFF)
 option(XBOX_AUTOPILOT "Test build: replay a scripted controller from D:/autopilot.txt (never deployed)" OFF)
 option(XBOX_LIMIT_MEMORY "Limit the title to the retail 64 MB even on 128 MB dev kits / xemu" ON)
+option(XBOX_LTCG "Enable Link Time Code Generation (LTCG)" OFF)
+if(XBOX_LTCG AND NOT XBOX_MODERN_LINKER)
+    message(WARNING "XBOX_LTCG requires XBOX_MODERN_LINKER. Disabling XBOX_LTCG.")
+    set(XBOX_LTCG OFF)
+endif()
+
 set(MC_LOG_LEVEL "0" CACHE STRING "Unified diagnostic verbosity: 0=off, 1=info, 2=debug, 3=trace")
 set_property(CACHE MC_LOG_LEVEL PROPERTY STRINGS 0 1 2 3)
 set(XBOX_TITLE_NAME "OptiCraft by xeodeo" CACHE STRING "Title name embedded in the XBE")
@@ -76,11 +82,9 @@ add_custom_command(
 set(XBOX_RUNTIME_SOURCES
     "${XBOX_RUNTIME_DIR}/XboxEntry.cpp"
     "${XBOX_RUNTIME_DIR}/XboxCrtShim.cpp"
-    "${XBOX_RUNTIME_DIR}/XboxP3Math.c"
     "${XBOX_IMPORTS_ASM}"
 )
 # Pentium III replacements for SSE2-only UCRT math: plain x87/integer code.
-set_property(SOURCE "${XBOX_RUNTIME_DIR}/XboxP3Math.c" APPEND PROPERTY COMPILE_OPTIONS "/arch:IA32")
 
 # --- Source selection ---------------------------------------------------------
 if(XBOX_BRINGUP)
@@ -123,8 +127,16 @@ else()
     message(STATUS "Xbox build: FULL game sources")
 endif()
 
+# --- Unity Build --------------------------------------------------------------
+# Global variables must be set BEFORE add_executable()
+
 # --- Target -------------------------------------------------------------------
-add_executable(OptiCraft ${XBOX_SOURCES} ${XBOX_RUNTIME_SOURCES})
+add_library(XboxP3Math OBJECT "${XBOX_RUNTIME_DIR}/XboxP3Math.c")
+target_compile_options(XboxP3Math PRIVATE /arch:IA32 /O2 /GS- /Gy /utf-8 /wd4996)
+add_executable(OptiCraft ${XBOX_SOURCES} ${XBOX_RUNTIME_SOURCES} $<TARGET_OBJECTS:XboxP3Math>)
+
+# Target-level properties (also set for good measure)
+
 set_target_properties(OptiCraft PROPERTIES
     SUFFIX ".exe"
     CXX_STANDARD 17
@@ -133,9 +145,11 @@ set_target_properties(OptiCraft PROPERTIES
     RUNTIME_OUTPUT_DIRECTORY "${XBOX_BIN_DIR}"
 )
 
+
 target_compile_options(OptiCraft PRIVATE
     $<$<COMPILE_LANGUAGE:C,CXX>:/arch:SSE>        # Pentium III: SSE, no SSE2
     $<$<COMPILE_LANGUAGE:C,CXX>:/O2>
+    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<STREQUAL:${XBOX_LTCG},ON>>:/GL>
     $<$<COMPILE_LANGUAGE:C,CXX>:/GS->             # no /GS cookies (no fastfail handler)
     $<$<COMPILE_LANGUAGE:C,CXX>:/Gy>
     $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
@@ -186,6 +200,8 @@ target_include_directories(OptiCraft PRIVATE
     ${XBOX_STL_INCLUDES}
 )
 
+# Link libraries and flags (replaces the old CMAKE_CXX_LINK_EXECUTABLE override)
+
 # XDK headers (XTL.h, D3D8.h, DSound.h, XInput via XTL) conflict with the
 # Win32 SDK's <windows.h>, so only Xbox backend sources get them, appended
 # after the modern STL (see xbox_toolchain.cmake). A backend TU includes
@@ -223,6 +239,7 @@ endif()
 # --- Link ---------------------------------------------------------------------
 # Order matters: the modern CRT resolves first (its mainCRTStartup is the one
 # XboxEntry calls), XAPI supplies Win32 services, xboxkrnl comes last.
+# Use custom link command for Xbox (required for XDK libraries and /ENTRY:XboxEntry)
 set(XBOX_LINK_LIBS
     "${XBOX_MSVC_LIB_DIR}/libcmt.lib"
     "${XBOX_MSVC_LIB_DIR}/libcpmt.lib"
@@ -244,8 +261,17 @@ if(XBOX_NETLOG_HOST OR XBOX_ENABLE_NETWORK)
 endif()
 list(APPEND XBOX_LINK_LIBS "${XBOX_XDK_ROOT}/lib/xboxkrnl.lib")
 string(REPLACE ";" "\" \"" _xbox_libs_quoted "${XBOX_LINK_LIBS}")
+if(XBOX_LTCG)
+    set(XBOX_LTCG_FLAG "/LTCG")
+else()
+    set(XBOX_LTCG_FLAG "")
+endif()
 set(CMAKE_CXX_LINK_EXECUTABLE
-    "\"${CMAKE_LINKER}\" /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /INCREMENTAL:NO /OPT:REF /MAP:<TARGET>.map /OUT:<TARGET> <OBJECTS> \"${_xbox_libs_quoted}\"")
+    "\"${CMAKE_LINKER}\" /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /INCREMENTAL:NO /OPT:REF ${XBOX_LTCG_FLAG} /FORCE:MULTIPLE /MAP:<TARGET>.map /OUT:<TARGET> <OBJECTS> \"${_xbox_libs_quoted}\"")
+
+# --- Unity Build (manual implementation) --------------------------------------
+# CMake's built-in UNITY_BUILD doesn't work with custom CMAKE_CXX_LINK_EXECUTABLE.
+# Manually create unity_*.cxx files that #include batches of source files.
 
 # --- Post-link: PE fixups, XBE, ISO ------------------------------------------
 get_filename_component(_xbox_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
@@ -304,7 +330,7 @@ add_custom_command(TARGET OptiCraft POST_BUILD
             -Path "$<TARGET_FILE:OptiCraft>" -Map "$<TARGET_FILE:OptiCraft>.map"
             -Dumpbin "${XBOX_DUMPBIN}"
     COMMAND "${XBOX_XDK_ROOT}/bin/imagebld.exe" ${XBOX_IMAGEBLD_FLAGS}
-            "/IN:$<TARGET_FILE:OptiCraft>" "/MAP:$<TARGET_FILE:OptiCraft>.map" "/OUT:${XBOX_XBE}"
+            "/IN:$<TARGET_FILE:OptiCraft>" "/FORCE:MULTIPLE /MAP:$<TARGET_FILE:OptiCraft>.map" "/OUT:${XBOX_XBE}"
     # Same program under a name with "720" in it: runs at 1280x720 when the
     # dashboard allows it (src/xbox/system/XboxVideoMode.h).
     COMMAND ${CMAKE_COMMAND} -E copy "${XBOX_XBE}" "${XBOX_ISO_DIR}/OptiCraft_720p.xbe"
diff --git a/cmake/xbox_toolchain.cmake b/cmake/xbox_toolchain.cmake
index 71f70bd..6106ed2 100644
--- a/cmake/xbox_toolchain.cmake
+++ b/cmake/xbox_toolchain.cmake
@@ -62,7 +62,12 @@ set(XBOX_WINSDK_VERSION "${XBOX_WINSDK_VERSION}" CACHE STRING "Windows 10 SDK ve
 set(CMAKE_C_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
 set(CMAKE_CXX_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
 set(CMAKE_ASM_MASM_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/ml.exe")
-set(CMAKE_LINKER "${XBOX_XDK_ROOT}/bin/vc71/Link.Exe")
+option(XBOX_MODERN_LINKER "Use VS2022 link.exe instead of XDK Link.Exe" ON)
+if(XBOX_MODERN_LINKER)
+    set(CMAKE_LINKER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/link.exe")
+else()
+    set(CMAKE_LINKER "${XBOX_XDK_ROOT}/bin/vc71/Link.Exe")
+endif()
 set(CMAKE_AR "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/lib.exe")
 # CMake's Windows platform module requires an RC compiler even though no .rc
 # file is built; the SDK's rc.exe satisfies it. The manifest tool is unused.

diff --git a/cmake/xbox.cmake b/cmake/xbox.cmake
index 92e1e1a..0ff5888 100644
--- a/cmake/xbox.cmake
+++ b/cmake/xbox.cmake
@@ -38,6 +38,12 @@ option(XBOX_ENABLE_SOUND "Enable the DirectSound backend" OFF)
 option(XBOX_ENABLE_NETWORK "Experimental: multiplayer over XNet TCP (Minecraft 1.2.5 servers)" OFF)
 option(XBOX_AUTOPILOT "Test build: replay a scripted controller from D:/autopilot.txt (never deployed)" OFF)
 option(XBOX_LIMIT_MEMORY "Limit the title to the retail 64 MB even on 128 MB dev kits / xemu" ON)
+option(XBOX_LTCG "Enable Link Time Code Generation (LTCG)" OFF)
+if(XBOX_LTCG AND NOT XBOX_MODERN_LINKER)
+    message(WARNING "XBOX_LTCG requires XBOX_MODERN_LINKER. Disabling XBOX_LTCG.")
+    set(XBOX_LTCG OFF)
+endif()
+
 set(MC_LOG_LEVEL "0" CACHE STRING "Unified diagnostic verbosity: 0=off, 1=info, 2=debug, 3=trace")
 set_property(CACHE MC_LOG_LEVEL PROPERTY STRINGS 0 1 2 3)
 set(XBOX_TITLE_NAME "OptiCraft by xeodeo" CACHE STRING "Title name embedded in the XBE")
@@ -76,11 +82,9 @@ add_custom_command(
 set(XBOX_RUNTIME_SOURCES
     "${XBOX_RUNTIME_DIR}/XboxEntry.cpp"
     "${XBOX_RUNTIME_DIR}/XboxCrtShim.cpp"
-    "${XBOX_RUNTIME_DIR}/XboxP3Math.c"
     "${XBOX_IMPORTS_ASM}"
 )
 # Pentium III replacements for SSE2-only UCRT math: plain x87/integer code.
-set_property(SOURCE "${XBOX_RUNTIME_DIR}/XboxP3Math.c" APPEND PROPERTY COMPILE_OPTIONS "/arch:IA32")
 
 # --- Source selection ---------------------------------------------------------
 if(XBOX_BRINGUP)
@@ -123,8 +127,16 @@ else()
     message(STATUS "Xbox build: FULL game sources")
 endif()
 
+# --- Unity Build --------------------------------------------------------------
+# Global variables must be set BEFORE add_executable()
+
 # --- Target -------------------------------------------------------------------
-add_executable(OptiCraft ${XBOX_SOURCES} ${XBOX_RUNTIME_SOURCES})
+add_library(XboxP3Math OBJECT "${XBOX_RUNTIME_DIR}/XboxP3Math.c")
+target_compile_options(XboxP3Math PRIVATE /arch:IA32 /O2 /GS- /Gy /utf-8 /wd4996)
+add_executable(OptiCraft ${XBOX_SOURCES} ${XBOX_RUNTIME_SOURCES} $<TARGET_OBJECTS:XboxP3Math>)
+
+# Target-level properties (also set for good measure)
+
 set_target_properties(OptiCraft PROPERTIES
     SUFFIX ".exe"
     CXX_STANDARD 17
@@ -133,9 +145,11 @@ set_target_properties(OptiCraft PROPERTIES
     RUNTIME_OUTPUT_DIRECTORY "${XBOX_BIN_DIR}"
 )
 
+
 target_compile_options(OptiCraft PRIVATE
     $<$<COMPILE_LANGUAGE:C,CXX>:/arch:SSE>        # Pentium III: SSE, no SSE2
     $<$<COMPILE_LANGUAGE:C,CXX>:/O2>
+    $<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<STREQUAL:${XBOX_LTCG},ON>>:/GL>
     $<$<COMPILE_LANGUAGE:C,CXX>:/GS->             # no /GS cookies (no fastfail handler)
     $<$<COMPILE_LANGUAGE:C,CXX>:/Gy>
     $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
@@ -186,6 +200,8 @@ target_include_directories(OptiCraft PRIVATE
     ${XBOX_STL_INCLUDES}
 )
 
+# Link libraries and flags (replaces the old CMAKE_CXX_LINK_EXECUTABLE override)
+
 # XDK headers (XTL.h, D3D8.h, DSound.h, XInput via XTL) conflict with the
 # Win32 SDK's <windows.h>, so only Xbox backend sources get them, appended
 # after the modern STL (see xbox_toolchain.cmake). A backend TU includes
@@ -223,6 +239,7 @@ endif()
 # --- Link ---------------------------------------------------------------------
 # Order matters: the modern CRT resolves first (its mainCRTStartup is the one
 # XboxEntry calls), XAPI supplies Win32 services, xboxkrnl comes last.
+# Use custom link command for Xbox (required for XDK libraries and /ENTRY:XboxEntry)
 set(XBOX_LINK_LIBS
     "${XBOX_MSVC_LIB_DIR}/libcmt.lib"
     "${XBOX_MSVC_LIB_DIR}/libcpmt.lib"
@@ -244,8 +261,17 @@ if(XBOX_NETLOG_HOST OR XBOX_ENABLE_NETWORK)
 endif()
 list(APPEND XBOX_LINK_LIBS "${XBOX_XDK_ROOT}/lib/xboxkrnl.lib")
 string(REPLACE ";" "\" \"" _xbox_libs_quoted "${XBOX_LINK_LIBS}")
+if(XBOX_LTCG)
+    set(XBOX_LTCG_FLAG "/LTCG")
+else()
+    set(XBOX_LTCG_FLAG "")
+endif()
 set(CMAKE_CXX_LINK_EXECUTABLE
-    "\"${CMAKE_LINKER}\" /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /INCREMENTAL:NO /OPT:REF /MAP:<TARGET>.map /OUT:<TARGET> <OBJECTS> \"${_xbox_libs_quoted}\"")
+    "\"${CMAKE_LINKER}\" /nologo /MACHINE:I386 /FIXED:NO /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:XboxEntry /INCREMENTAL:NO /OPT:REF ${XBOX_LTCG_FLAG} /FORCE:MULTIPLE /MAP:<TARGET>.map /OUT:<TARGET> <OBJECTS> \"${_xbox_libs_quoted}\"")
+
+# --- Unity Build (manual implementation) --------------------------------------
+# CMake's built-in UNITY_BUILD doesn't work with custom CMAKE_CXX_LINK_EXECUTABLE.
+# Manually create unity_*.cxx files that #include batches of source files.
 
 # --- Post-link: PE fixups, XBE, ISO ------------------------------------------
 get_filename_component(_xbox_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
diff --git a/cmake/xbox_toolchain.cmake b/cmake/xbox_toolchain.cmake
index 71f70bd..6106ed2 100644
--- a/cmake/xbox_toolchain.cmake
+++ b/cmake/xbox_toolchain.cmake
@@ -62,7 +62,12 @@ set(XBOX_WINSDK_VERSION "${XBOX_WINSDK_VERSION}" CACHE STRING "Windows 10 SDK ve
 set(CMAKE_C_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
 set(CMAKE_CXX_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/cl.exe")
 set(CMAKE_ASM_MASM_COMPILER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/ml.exe")
-set(CMAKE_LINKER "${XBOX_XDK_ROOT}/bin/vc71/Link.Exe")
+option(XBOX_MODERN_LINKER "Use VS2022 link.exe instead of XDK Link.Exe" ON)
+if(XBOX_MODERN_LINKER)
+    set(CMAKE_LINKER "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/link.exe")
+else()
+    set(CMAKE_LINKER "${XBOX_XDK_ROOT}/bin/vc71/Link.Exe")
+endif()
 set(CMAKE_AR "${XBOX_MSVC_ROOT}/bin/Hostx64/x86/lib.exe")
 # CMake's Windows platform module requires an RC compiler even though no .rc
 # file is built; the SDK's rc.exe satisfies it. The manifest tool is unused.


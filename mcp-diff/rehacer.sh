#!/bin/sh
# Rehace desde cero la fuente Java con nombres MCP de 1.2.5 y 1.5.2 y las listas de diferencias.
# Uso (Git Bash):  sh rehacer.sh
# Guia completa: buzon-xbox/55-GUIA-mcp-descompilar-y-revisar.md
set -e
W="$(cd "$(dirname "$0")" && pwd)"
J=/c/Users/xeodeo/Downloads/xeolauncher/java/java-runtime-delta/bin/java.exe
cd "$W"

# 1. Herramientas (solo si faltan)
mkdir -p tools
[ -f tools/vineflower.jar ] || curl -sL -o tools/vineflower.jar \
  https://repo1.maven.org/maven2/org/vineflower/vineflower/1.10.1/vineflower-1.10.1.jar
[ -f tools/tiny-remapper.jar ] || curl -sL -o tools/tiny-remapper.jar \
  https://maven.fabricmc.net/net/fabricmc/tiny-remapper/0.10.4/tiny-remapper-0.10.4-fat.jar
# RetroMCP-Java-CLI.jar (github.com/MCPHackers/RetroMCP-Java, releases) da jars/minecraft.jar y conf/mappings.tiny.
# Su "setup" termina con error (maven.glass-launcher.net no responde) pero deja lo que hace falta.
for v in 1.2.5 1.5.2; do
  d=v$(echo $v | tr -d .)
  if [ ! -f $d/conf/mappings.tiny ]; then
    mkdir -p $d; (cd $d && $J -jar ../tools/RetroMCP-Java-CLI.jar setup --version $v || true)
  fi
done

# 2. Renombrar (ofuscado -> nombres MCP). 1.2.5 usa el espacio "client", 1.5.2 "official".
L=$(find v125/libraries -name "lwjgl*.jar" -o -name "jinput-2*.jar" | grep -v natives | tr '\n' ' ')
rm -f mc125-named.jar mc152-named.jar
$J -jar tools/tiny-remapper.jar v125/jars/minecraft.jar mc125-named.jar v125/conf/mappings.tiny client named $L
$J -jar tools/tiny-remapper.jar v152/jars/minecraft.jar mc152-named.jar v152/conf/mappings.tiny official named $L --ignoreConflicts

# 3. Descompilar
rm -rf src125 src152; mkdir -p src125 src152
$J -Xmx2G -jar tools/vineflower.jar -log=WARN --only=net/minecraft mc125-named.jar src125 > dec125.log 2>&1
$J -Xmx2G -jar tools/vineflower.jar -log=WARN --only=net/minecraft mc152-named.jar src152 > dec152.log 2>&1

# 4. Listas
(cd src125 && find net -name '*.java' | sort) > l125
(cd src152 && find net -name '*.java' | sort) > l152
comm -23 l125 l152 > solo125      # clases que desaparecen
comm -13 l125 l152 > solo152      # clases nuevas
comm -12 l125 l152 > comunes
: > cambiadas                     # "<lineas distintas> <clase>"
while read f; do
  diff -q -w "src125/$f" "src152/$f" >/dev/null || \
    echo "$(diff -w "src125/$f" "src152/$f" | grep -c '^[<>]') $f" >> cambiadas
done < comunes
sed 's|.*/||;s|\.java||' solo152 > n152
# contenido152 = solo152 sin servidor/red/infra
grep -vE '/(Callable|Command|Dedicated|Crash|GuiScreen|Mco|Exception|Ban|Http|RCon|Snooper|Server|Thread|Integrated|Demo|IAdmin|ICommand|IServer|Tcp|Log|Request|Task|ValueObject|Lan|Score|Packet|Player(Manager|Instance|Selector|NotFound)|EntityPlayerMP|EntityTracker|WorldServer|NetServer|NetLogin|Network|ChunkProviderServer|ItemInWorldManager|Gui(Slot)?Online|Selection|Timer|RunnableTitle|PropertyManager|MinecraftFake|ClientBrand|MemoryConnection|Crypt)' solo152 > contenido152
wc -l solo125 solo152 comunes cambiadas contenido152

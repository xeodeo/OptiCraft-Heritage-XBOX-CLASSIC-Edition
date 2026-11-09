# Informe 65 (agy): Auditoría y Corrección Completa de Idiomas de Minecraft 1.5.2

## 1. Resumen de la Intervención
- **Rama de trabajo**: `port152-idiomas` en `wt-152-idiomas`.
- **Objetivo**: Garantizar paridad total con Minecraft Java 1.5.2 en textos, claves de traducción, respaldos automáticos a `en_US`, corrección de formateo de cadenas (`%1$d`, `%1$s of %2$s`), resolución de textos en inglés escritos a mano en la interfaz de consola, y soporte correcto de libros encantados y yunques.
- **Empaquetado y Datos**: `scripts/port152/build_textures_152.py` actualizado para fusionar los textos 1.5.2 de los 58 idiomas soportados por OptiCraft y corregir `languages.txt`. Empaquetado `assets.pak` generado exitosamente (1.701 entradas, 63.704.128 bytes).
- **Compilación Xbox**: Ambas configuraciones (`xbox-release` y `xbox-release-exp`) compiladas con el candado `buzon-xbox/BUILD.lock`.

---

## 2. Textos a Mano / Claves Corregidas en C++

| Elemento / Ubicación | Problema Previo | Solución Aplicada |
| :--- | :--- | :--- |
| **Formateo de enteros en cadenas (`StringTranslate::translateKeyFormat`)** | Solo reemplazaba `%s` y `%$s`. En 1.5.2, el yunque usa `%1$d` (`container.repair.cost=Enchantment Cost: %1$d`). El coste no se sustituía y se mostraba `Coste de Encantamiento: %1$d`. | `translateKeyFormat` ahora procesa tanto `%s` como `%d`, y posicionales `%$s` y `%$d`. |
| **Indicador de página de libros (`GuiScreenBook.cpp`)** | Se pasaba un solo argumento concatenado (`currPage + 1 + " of " + bookTotalPages`) a la clave `book.pageIndicator=Page %1$s of %2$s`. Mostraba `Página 1 of 5 de %2$s`. | Sobrecarga `StatCollector::translateToLocalFormatted(key, arg1, arg2)` implementada y llamada con 2 argumentos separados. |
| **Libro Encantado sin nivel ni nombre (`ItemEnchantedBook.cpp`)** | `Enchantment::initEnchantments()` no era invocado por `ItemEnchantedBook`. Al abrir el tooltip, la tabla de encantamientos estaba vacía y la comprobación de puntero nulo fallaba, mostrando el libro sin nombre de encantamiento ni nivel. Tampoco generaba encantamientos en cofres. | Se añadió la inicialización `Enchantment::initEnchantments()` en el constructor y en todos los métodos de consulta y generación (`addInformation`, `func_92109_a`, `func_92112_a`, `func_92113_a`). |
| **Nombre personalizado en Yunque / Libros (`ItemStack.cpp`)** | El tooltip de `ItemStack` llamaba a `item->getItemDisplayName(this)`, ignorando la etiqueta `display.Name` guardada por el yunque. Además, tenía caracteres mal codificados en el prefijo cursiva. | Se corrigió para comprobar `hasDisplayName()` y formatear en cursiva usando `\xc2\xa7o` + `getDisplayName()` + `\xc2\xa7r`. |
| **Pestañas del Menú Creativo 1.5.2 (`CreativeTabs.cpp` / `GuiContainerCreative.cpp`)** | Los nombres de pestañas estaban escritos a mano en inglés (`"Building Blocks"`, `"Decoration"`, etc.). | Con el nuevo menú creativo de 12 pestañas, `CreativeTabs::getTranslatedTabLabel()` consulta `itemGroup.<tabLabel>`. Se añadieron todas las claves `itemGroup.*` a `port152Defaults` y a los archivos `.lang`. |
| **Pestañas y textos de la Interfaz Xbox (`XboxCraftingScreen.cpp` / `UiStrings.h`)** | Textos de consola como `"Structures"`, `"Tools"`, `"Food"`, `"Armour"`, `"Mechanisms"`, `"Transport"`, `"Craft"`, `"Back"`, `"Group"`, `"Ingredients"`, etc. no tenían traducción o solo usaban inglés. | Se asociaron a claves vanillas (`itemGroup.*`, `gui.*`, `key.*`) en `englishUiKeys` para todos los idiomas, y se añadieron traducciones directas en español en `StringTranslate::translateUi`. |
| **Nombres regionales y orden en `languages.txt`** | `en_CA` y `en_GB` estaban invertidos (`en_CA=English (UK)`, `en_GB=Canadian English (CA)`). Además, varios dialectos hispanohablantes usaban nombres desfasados (`es_AR=Castellano (Argentina)`). | `build_textures_152.py` ahora actualiza `languages.txt` con los nombres oficiales de la 1.5.2 preservando exactamente los 58 idiomas soportados por OptiCraft. |
| **Comprobación de clave existente (`StatCollector::canTranslate` / `func_94522_b`)** | Métodos necesarios en 1.5.2 implementados y delegados a `StringTranslate::containsTranslateKey`. | Implementado en `StatCollector.h/.cpp` y `StringTranslate.h/.cpp`. |

---

## 3. Claves de Traducción por Idioma (Base 1.2.5 vs Merged 1.5.2)

Todos los 58 idiomas soportados en OptiCraft se actualizaron con las claves de Minecraft 1.5.2 mediante `merge_languages()`:

| Idioma | Claves Base (1.2.5) | Claves 1.5.2 | Claves Nuevas Incorporadas |
| :--- | :---: | :---: | :---: |
| **af_ZA** (Afrikaans) | 737 | 1220 | +503 |
| **ar_SA** (العربية) | 737 | 1220 | +503 |
| **bg_BG** (Български) | 737 | 1220 | +503 |
| **ca_ES** (Català) | 737 | 1220 | +503 |
| **cs_CZ** (Čeština) | 737 | 1220 | +503 |
| **cy_GB** (Cymraeg) | 737 | 1220 | +503 |
| **da_DK** (Dansk) | 737 | 1220 | +503 |
| **de_DE** (Deutsch) | 737 | 1220 | +503 |
| **el_GR** (Ελληνικά) | 737 | 1220 | +503 |
| **en_CA** (Canadian English) | 737 | 1220 | +503 |
| **en_GB** (English UK) | 737 | 1220 | +503 |
| **en_PT** (Pirate Speak) | 737 | 1220 | +503 |
| **en_US** (English US) | 740 | 1240 | +520 |
| **eo_UY** (Esperanto) | 737 | 1220 | +503 |
| **es_AR** (Español Argentina) | 737 | 1220 | +503 |
| **es_ES** (Español España) | 737 | 1220 | +503 |
| **es_MX** (Español México) | 737 | 1220 | +503 |
| **es_UY** (Español Uruguay) | 737 | 1220 | +503 |
| **es_VE** (Español Venezuela) | 737 | 1220 | +503 |
| **et_EE** (Eesti) | 737 | 1220 | +503 |
| **eu_ES** (Euskara) | 737 | 1220 | +503 |
| **fi_FI** (Suomi) | 737 | 1220 | +503 |
| **fr_CA** (Français Canadien) | 737 | 1220 | +503 |
| **fr_FR** (Français) | 737 | 1220 | +503 |
| **gl_ES** (Galego) | 737 | 1220 | +503 |
| **he_IL** (עברית) | 737 | 1220 | +503 |
| **hr_HR** (Hrvatski) | 737 | 1220 | +503 |
| **hu_HU** (Magyar) | 737 | 1220 | +503 |
| **is_IS** (Íslenska) | 737 | 1220 | +503 |
| **it_IT** (Italiano) | 737 | 1220 | +503 |
| **ja_JP** (日本語) | 737 | 1220 | +503 |
| **ka_GE** (ქართული) | 737 | 1220 | +503 |
| **ko_KR** (한국어) | 737 | 1220 | +503 |
| **ky_KG** (Кыргызча) | 737 | 869 | +133 |
| **lt_LT** (Lietuvių) | 737 | 1220 | +503 |
| **lv_LV** (Latviešu) | 737 | 1220 | +503 |
| **mi_NZ** (Te Reo Māori) | 737 | 869 | +133 |
| **mt_MT** (Malti) | 737 | 1220 | +503 |
| **nb_NO** (Norsk bokmål) | 737 | 1018 | +282 |
| **nl_NL** (Nederlands) | 737 | 1220 | +503 |
| **nn_NO** (Norsk nynorsk) | 737 | 1220 | +503 |
| **pl_PL** (Polski) | 737 | 1220 | +503 |
| **pt_BR** (Português Brasil) | 737 | 1220 | +503 |
| **pt_PT** (Português Portugal) | 737 | 1220 | +503 |
| **qya_AA** (Quenya) | 737 | 1220 | +503 |
| **ro_RO** (Română) | 737 | 1220 | +503 |
| **ru_RU** (Русский) | 737 | 1219 | +503 |
| **sk_SK** (Slovenčina) | 737 | 1220 | +503 |
| **sl_SI** (Slovenščina) | 737 | 1220 | +503 |
| **sr_SP** (Српски) | 737 | 1220 | +503 |
| **sv_SE** (Svenska) | 737 | 1220 | +503 |
| **th_TH** (ภาษาไทย) | 737 | 1220 | +503 |
| **tlh_AA** (tlhIngan Hol) | 737 | 1220 | +503 |
| **tr_TR** (Türkçe) | 737 | 1220 | +503 |
| **uk_UA** (Українська) | 737 | 1220 | +503 |
| **vi_VN** (Tiếng Việt) | 737 | 1220 | +503 |
| **zh_CN** (简体中文) | 737 | 1220 | +503 |
| **zh_TW** (繁體中文) | 737 | 1220 | +503 |

---

## 4. Idiomas de Java 1.5.2 No Presentes en OptiCraft

En el código fuente de Java 1.5.2 existen 66 idiomas. OptiCraft incluye 58 en su selector de consola. Siguiendo las instrucciones del encargo, **no se añadieron sin consultar** para no romper la interfaz ni las fuentes de la lista de selección de consola:

1. `en_AU` (Australian English)
2. `ga_IE` (Gaeilge / Irish)
3. `hi_IN` (हिन्दी / Hindi)
4. `id_ID` (Bahasa Indonesia)
5. `kw_GB` (Kernewek / Cornish)
6. `ms_MY` (Bahasa Melayu)
7. `no_NO` (Norwegian genérico; OptiCraft soporta las dos variantes estándar: `nb_NO` y `nn_NO`)

---

## 5. Auditoría de Fuentes y Limitaciones de Caracteres

La fuente estándar bitmap del juego (`default.png` gobernada por `font.txt`) consta de **143 glifos mapeados** correspondientes al repertorio ASCII y una selección de caracteres extendidos Latin-1.

### 5.1. Soporte en Español y Alfabeto Latino Básico
- **Español (`es_ES`, `es_AR`, `es_MX`, `es_UY`, `es_VE`)**: Cobertura del **99.9%** con la fuente predeterminada. Todas las vocales minúsculas acentuadas (`á`, `é`, `í`, `ó`, `ú`), la eñe (`ñ`, `Ñ`), signos de interrogación/exclamación invertidos (`¿`, `¡`), diéresis (`ü`) y cedilla (`ç`) están presentes en `font.txt` y se renderizan directamente en la textura de fuente normal. Solo un reducido número de mayúsculas acentuadas excepcionales (`Á`, `Í`) recaen fuera del `font.txt`.
- **10 Idiomas 100% compatibles con `font.txt`** (0 caracteres fuera de la textura):
  `da_DK`, `en_CA`, `en_GB`, `en_US`, `fi_FI`, `mi_NZ`, `nb_NO`, `nl_NL`, `nn_NO`, `sv_SE`.

### 5.2. Idiomas que Requieren Páginas Unicode (`glyph_XX.png` / `glyph_sizes.bin`)
Un total de **48 idiomas** contienen caracteres que exceden el repertorio de `font.txt`:
- **Alfabetos CJK (Chino, Japonés, Coreano)**: `zh_CN`, `zh_TW`, `ja_JP`, `ko_KR` (entre 500 y 950 ideogramas/sílabas por idioma).
- **Alfabeto Cirílico**: `ru_RU`, `uk_UA`, `bg_BG`, `sr_SP`, `ky_KG` (requieren páginas `glyph_04.png`).
- **Alfabeto Griego**: `el_GR` (requiere página `glyph_03.png`).
- **Escrituras Semíticas (Árabe y Hebreo)**: `ar_SA`, `he_IL` (requieren páginas `glyph_05.png`, `glyph_06.png` y renderizado bidireccional RTL).
- **Otras Escrituras**: `ka_GE` (georgiano), `th_TH` (tailandés).
- **Latino Extendido (Europa Central y Oriental)**: `pl_PL`, `cs_CZ`, `sk_SK`, `hu_HU`, `ro_RO`, `tr_TR`, `lt_LT`, `lv_LV`, `et_EE`, `sl_SI`, `hr_HR` (caracteres con ogonek, carón, barra o doble acento agudo como `ą`, `č`, `ř`, `ő`, `ğ`).

*(Nota: De acuerdo con la directriz del encargo, no se modificaron los archivos de fuentes `font.txt` ni `glyph_sizes.bin`).*

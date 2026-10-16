# TotalEditor

[![Idioma: Español](https://img.shields.io/badge/Idioma-Español-green)](#) [English](README.md)

Un editor integrado en el juego para bonificaciones de razas, atributos de personajes, colores de uniformes de armaduras y fotografía de retratos en Kenshi, implementado como un plugin C++ para RE_Kenshi. Presione **Ctrl+K** para abrirlo.

Este documento contiene la documentación técnica para desarrolladores y creadores de mods. Para la descripción orientada a jugadores, consulte `STEAM_EN.md` y `STEAM_ES.md`.

---

## Anteriormente RaceStatsRework

TotalEditor sustituye a RaceStatsRework; es el mismo mod bajo una arquitectura ampliada y en una nueva carpeta (`mods/TotalEditor`). Al iniciar el juego, si la carpeta de TotalEditor no contiene un archivo `races.json` pero la carpeta antigua de `mods/RaceStatsRework` sí lo tiene, los datos se copian automáticamente una vez (`race/legacy_data.cpp`) para preservar las modificaciones guardadas. El archivo original nunca se elimina ni se mueve. Es necesario desactivar el mod antiguo RaceStatsRework en el lanzador, ya que mantener ambos activos causaría colisiones de atajos de teclado y ganchos de memoria.

---

## Las Seis Pestañas

1. **Race overview (Resumen de raza)** — Comparación directa leída de izquierda a derecha. Muestra la raza de origen a la izquierda y la de destino a la derecha, cada una con un desglose completo de rasgos, bonificaciones y estadísticas corporales para verificar los cambios antes de aplicarlos. El botón Copiar se ubica bajo el origen; Restablecer se ubica bajo el objetivo.
2. **Skill bonuses (Bonificaciones de habilidades)** — A la derecha se listan las bonificaciones de la raza seleccionada y a la izquierda los controles para modificar cada una. Al seleccionar una fila de la lista se carga en los controles de edición.
3. **Body parts (Partes del cuerpo)** — Salud y peso de impacto de cada extremidad y órgano, junto con el rango global de sangre del cuerpo.
4. **Equipment and nature (Equipamiento y naturaleza)** — Dieciséis casillas de selección que controlan permisos de raza (cascos, armaduras corporales, botas, natación, capacidad de ser cargado, condición de robot, vampirismo, entre otros).
5. **Character (Personaje)** — Modifica al personaje individual seleccionado en el mundo:
   - **Habilidades**: Atributos y habilidades de combate en la escala nativa del motor (0 a 200).
   - **Personalidad**: Define el estado de personalidad de la IA (`personality.cpp`), permitiendo elegir entre las 10 opciones del juego (Honorable, Traidor, Inteligente, Tonto, Valiente, Temeroso, Amable, Frío/Cruel, Normal, Maníaco).
   - **Editar apariencia**: Abre el editor nativo de creación de personajes de Kenshi para el personaje jugador seleccionado (`appearance.cpp`).
6. **Uniform & Portrait (Uniforme y retrato)** — Sistema integrado de personalización visual y fotografía:
   - **Colores de armadura**: Personalización de los códigos hexadecimales primario y secundario con previsualización en tiempo real sobre el modelo 3D del personaje en el mundo. Incluye botones de ajuste por canal RGB (±15), selector de ajustes preestablecidos e indicadores de color. Los cambios son exclusivos de previsualización hasta presionar **Guardar**; presionar **Cancelar** o cerrar la ventana revierte todos los cambios de inmediato; **Vanilla** restablece los colores originales de fábrica del equipo.
   - **Cámara y fondo de retrato**: Abre el editor fotográfico para ajustar rotación (yaw), elevación de cámara, encuadre horizontal, zoom (campo de visión) y fondos personalizados (Verde azulado, Negro, Arena, Color RGB personalizado, Burdeos o imágenes del usuario).

---

## Alcance de las Modificaciones

La interfaz opera en múltiples niveles de la arquitectura del juego:

- **Las bonificaciones de raza son globales y compartidas.** `RaceData::AllRaces` contiene una única instancia por cada definición de raza, y `Character::myRace` apunta directamente a dicha instancia. Modificar los multiplicadores de la raza Greenlander afectará a todos los Greenlander del mundo: miembros del escuadrón, guardias de ciudades y bandidos hostiles por igual. Las bonificaciones son **multiplicadores** (`stat *= bonus`), donde `1.0` representa el valor estándar sin cambios y `1.15` un incremento del 15%.
- **Las estadísticas de personaje son individuales.** Modificar habilidades en la pestaña Character escribe directamente en la estructura de memoria del personaje seleccionado en el rango de `0..200` y fuerza el recálculo inmediato de valores dependientes (peso transportable, velocidad de ataque).
- **Los colores de uniformes están vinculados al personaje.** Los esquemas de color se guardan por identificador único de personaje (`CharacterIdentity`), modificando los tintes de tela y metal sin alterar a otros miembros del escuadrón ni a los personajes no jugables de la facción.

---

## Plantilla de Raza vs Personaje Activo

La mayor parte de los datos de raza constituyen una plantilla que el motor consulta únicamente al generar o cargar una entidad. Modificar la plantilla no altera retroactivamente a personajes ya existentes en el mundo a menos que se sincronicen de forma explícita:

| Modificación | Aplicación en el personaje seleccionado en el mundo |
|---|---|
| Ranuras de equipamiento (cascos, camisas, botas) | Las secciones de inventario se habilitan o deshabilitan dinámicamente |
| Rango de sangre | Se recalcula e interpola de acuerdo con la edad del personaje |
| Salud de partes corporales | Se aplica directamente a la anatomía viva (`ApplyAnatomyToCharacter`) |
| Habilidades de personaje | Se escriben directamente en la instancia del personaje |
| Colores de uniforme | Se aplican en vivo mediante un esquema de previsualización temporal y se guardan al confirmar |

---

## Flujo de Uniformes y Retratos

### Motor de Uniformes (`src/uniform/`)
- **Sesión de previsualización**: Al alternar entre personajes o modificar colores, `UniformEngine::BeginPreview` genera un esquema temporal `GameData` en memoria.
- **Actualizaciones en vivo**: Las modificaciones de color se envían inmediatamente a través de `UniformEngine::PreviewColours` y `UniformDresser::RefreshEquipped`, repintando las mallas equipadas.
- **Persistencia**: Las paletas confirmadas se almacenan en `uniform_colours.json` asociadas al UID del personaje (`uniform_store.cpp`). Los cambios no guardados se descartan al cerrar la ventana mediante `CancelPreview`.

### Editor de Retratos (`src/portrait/`)
- **Ganchos de cámara**: Intercepta `Ogre::Camera::getViewMatrix` y `RenderToTextureotron::renderCharacter` para ajustar el ángulo, la altura, la posición y la distancia focal de la cámara sin alterar la geometría del mundo.
- **Composición y transferencia de atlas**: Intercepta `PortraitManager::setImageWidget` y `PortraitMainCellView::update`. Renderiza al personaje en una textura, compone las transparencias sobre el fondo seleccionado y transfiere el resultado directamente a la memoria de texturas de la interfaz.

---

## Arquitectura de Persistencia

TotalEditor mantiene varios archivos de persistencia junto a su archivo DLL:

1. **`races.json`**:
   - `originals` / `originalTraits` / `originalParts`: Registrados la primera vez que se carga cada raza; sirven como base inmutable para la función Restablecer.
   - `overrides` / `overrideTraits` / `overrideParts`: Modificaciones activas del usuario aplicadas al iniciar la partida.
2. **`uniform_colours.json`**:
   - Asocia los identificadores persistentes de personajes (UID) con códigos de color hexadecimal primario y secundario.
3. **Archivo de guardado del juego**:
   - Las habilidades individuales, atributos y personalidades se almacenan directamente en los archivos de guardado `.pl_save` de Kenshi.

---

## Estructura Modular del Código

El código fuente está organizado en módulos desacoplados por dominio de responsabilidad:

```text
TotalEditor/
├── src/
│   ├── TotalEditor.cpp            Puntos de entrada del plugin (startPlugin, shutdown)
│   ├── common.h                   Definiciones comunes, macros y utilidades SEH
│   ├── core/                      Infraestructura e integración con el motor
│   │   ├── config.cpp/.h          Cargador de configuración (config.json)
│   │   ├── frame_tick.cpp/.h      Bucle principal y eventos de inicialización
│   │   ├── hotkey.cpp/.h          Gancho de teclado OIS y mapeo de teclas
│   │   ├── json.cpp/.h            Serialización y deserialización ligera de JSON
│   │   ├── log.cpp/.h             Sistema de registro en tres niveles (LogError, LogInfo, LogDebug)
│   │   └── messages.cpp/.h        Diccionario de localización (messages.json)
│   ├── race/                      Definiciones de razas y estadísticas globales
│   │   ├── anatomy.cpp/.h         Salud de partes corporales, peso de impacto y sangre
│   │   ├── catalog.cpp/.h         Enumeración de razas y habilidades en GameData
│   │   ├── legacy_data.cpp/.h     Migración de datos desde RaceStatsRework
│   │   ├── race_edit.cpp/.h       Lectura y escritura en memoria de bonificaciones
│   │   └── store.cpp/.h           Persistencia de races.json y valores originales
│   ├── character/                 Modificaciones por personaje individual
│   │   ├── appearance.cpp/.h      Integración con el editor de apariencia del juego
│   │   ├── character_identity.cpp/.h Generación y resolución de identificadores persistentes (UID)
│   │   └── personality.cpp/.h     Lectura y escritura del estado de personalidad de IA
│   ├── uniform/                   Gestión de colores de armadura y ropa
│   │   ├── colour_adjust_rules.cpp/.h Ajuste incremental de canales (±15) y preajustes
│   │   ├── uniform_colour.cpp/.h  Conversión y validación de formatos RGB y hexadecimal
│   │   ├── uniform_dresser.cpp/.h Detección de ranuras de equipo y repintado de materiales
│   │   ├── uniform_engine.cpp/.h  Máquina de estados de previsualización, guardado y restauración
│   │   ├── uniform_schemes.cpp/.h Generación de esquemas GameData en memoria
│   │   └── uniform_store.cpp/.h   Persistencia en uniform_colours.json
│   ├── portrait/                  Cámara de retrato y renderizado compuesto
│   │   └── portrait_editor.cpp/.h Ganchos de cámara Ogre, composición de texturas e interfaz
│   └── ui/                        Capa de presentación de MyGUI
│       ├── editor_window.cpp/.h   Ventana principal de 6 pestañas y distribución visual
│       └── ui_widgets.cpp/.h      Fábrica protegida de widgets MyGUI y componentes base
└── TotalEditor/                   Archivos de distribución del mod
    ├── TotalEditor.mod
    ├── TotalEditor.dll
    ├── RE_Kenshi.json
    ├── config.json
    ├── messages.json
    ├── backgrounds/
    └── gui/
```

---

## Referencia de Configuración (`config.json`)

| Parámetro | Valor por defecto | Descripción |
|---|---|---|
| `enabled` | `true` | Habilita o deshabilita los ganchos del plugin |
| `hotkey.key` | `"K"` | Nombre de la tecla o código OIS para abrir el editor |
| `hotkey.needsCtrl` | `true` | Requiere mantener presionada la tecla Ctrl |
| `bonus.min` / `bonus.max` | `0.1` / `5.0` | Rango mínimo y máximo del multiplicador de bonificaciones |
| `bonus.steps` | `98` | Granularidad de pasos en la barra deslizadora de multiplicadores |
| `stat.min` / `stat.max` | `0.0` / `200.0` | Rango permitido para habilidades de personajes |
| `partHealth.min` / `partHealth.max` | `1.0` / `500.0` | Rango de puntos de salud para partes corporales |
| `window.width` / `window.height` | `1180` / `920` | Dimensiones en píxeles de la ventana del editor |
| `store.saveOnClose` | `true` | Guarda automáticamente los cambios al cerrar la ventana |
| `store.applyOnStart` | `true` | Aplica las modificaciones guardadas al cargar el mundo |
| `followSelection` | `true` | Inspecciona automáticamente al personaje seleccionado en el juego |
| `debugLog` | `false` | Activa el registro detallado de diagnóstico paso a paso |

---

## Compilación e Instalación

### Requisitos
- Visual Studio (MSVC) con **Conjunto de herramientas de la plataforma v100** (compatibilidad con Visual Studio 2010)
- SDK de KenshiLib (cabeceras y bibliotecas) configurado en la variable de entorno `KENSHILIB_DIR`
- Cabeceras de Boost 1.60+ configuradas en `BOOST_INCLUDE_PATH`

### Comando de Compilación
Ejecute MSBuild seleccionando la configuración Release de 64 bits:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TotalEditor.vcxproj /p:Configuration=Release /p:Platform=x64
```

El evento posterior a la compilación copiará automáticamente `TotalEditor.dll` a `D:\SteamLibrary\steamapps\common\Kenshi\mods\TotalEditor\TotalEditor.dll` si la ruta existe.

> [!WARNING]
> Kenshi bloquea el archivo `TotalEditor.dll` en memoria mientras se encuentra en ejecución. Asegúrese de cerrar el juego antes de compilar para evitar errores de permisos de escritura.

---

## Notas Técnicas y Lecciones del SDK

- **Jerarquía de WhiteSkin en MyGUI**: No asigne un widget con `WhiteSkin` como elemento hijo directo de otro widget con `WhiteSkin`. En la versión de MyGUI utilizada por Kenshi, los SubSkins del contenedor padre pueden ocultar a los hijos. Cree siempre el marco y el relleno como widgets hermanos (*siblings*) dentro del panel contenedor.
- **Ausencia de símbolos para `MyGUI::Slider`**: Los métodos de `MyGUI::Slider` no se encuentran exportados en los binarios del motor de Kenshi. Los deslizadores de valor se construyen utilizando `MyGUI::ScrollBar` con `setMoveToClick(true)`.
- **Funciones virtuales**: Nunca declare funciones miembro `virtual` coincidentes contra clases del motor de Kenshi en el código del plugin, ya que MSVC creará una tabla virtual independiente. Utilice las funciones no virtuales equivalentes (`_NV_`) provistas por KenshiLib.
- **Almacenamiento de anatomía**: La salud y pesos de impacto de combate residen en la lista de GameData de la raza (`TripleInt`), no directamente en la estructura `RaceData`.
- **Disciplina en bloques de captura de excepciones**: Evite escribir bloques de captura vacíos (`TE_CATCH { }`) en rutinas de inicialización o enlace de eventos. Una excepción no controlada dentro de `BindEvents` interrumpe la función por completo, dejando los controles restantes desvinculados y simulando problemas de captura de eventos del mouse.

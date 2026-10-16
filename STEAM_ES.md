# Total Editor

Edita desde el juego los bonos de cualquier raza, la salud de sus partes, su
sangre y sus rasgos — y las habilidades propias de cualquier personaje — sin
abrir el FCS.

Pulsa **Ctrl + K**.

**Antes se llamaba Race Stats Rework.** Total Editor lo reemplaza. Tus cambios
de razas guardados se toman automáticamente de la carpeta del mod anterior la
primera vez que inicias el juego (se copian; el archivo original no se toca).
Desactiva el mod Race Stats Rework: si ambos están activos, habrá dos editores
en la misma tecla.

---

## Léelo antes de usarlo

**Los bonos de raza los comparten todos los personajes de esa raza.** Si le das
a los Shek +20% de Fuerza, se lo has dado a tu escuadra, a los guardias de las
ciudades y a todos los bandidos Shek que te ataquen. Kenshi guarda una sola
tabla de bonos por raza y cada personaje apunta a ella; no existe una versión
por personaje.

**Los bonos son multiplicadores, no puntos que se suman.** `1.15` significa un
quince por ciento más, `1.0` significa sin cambios.

La única página que edita a una sola persona — **Character** — está señalada
como tal, y esas habilidades son los valores 0–200 del propio motor.

---

## Las seis páginas

**Race overview** — dos razas una al lado de la otra. Elige a la izquierda la
raza de la que copiar y a la derecha la que quieres cambiar; ambas columnas
listan todo lo que lleva cada raza, para que veas el intercambio antes de
hacerlo. El botón de copiar está bajo la fuente, y el de revertir bajo el
destino.

**Skill bonuses** — los bonos de la raza seleccionada, a la derecha. Pulsa uno
para cargarlo en los controles de la izquierda y cambiarlo o eliminarlo.

**Body parts** — cada parte con su salud y su peso de impacto, más el rango de
sangre del cuerpo entero. El peso de impacto es relativo, no un porcentaje:
subir una parte baja la proporción de todas las demás.

**Equipment and nature** — dieciséis interruptores, cada uno leído como un permiso.
Marcado significa que la raza SÍ puede.

**Character** — las habilidades propias del personaje seleccionado, listadas a
la derecha. Pulsa una para editarla a la izquierda. Debajo está la
**personalidad** del personaje (Honorable, Traidor, Inteligente, Torpe, Valiente,
Miedoso, Cálido / Amable, Frío / Cruel, Normal, Maníaco): elige una en
la lista y pulsa APPLY. **Edit Appearance** abre el editor de personaje del
propio juego sobre el seleccionado (solo personajes de tu escuadrón).

**Uniform & Portrait** — estilo de equipamiento y fotografía del personaje seleccionado.
Ajusta los colores primario y secundario de las armaduras con previsualización 3D en vivo
sobre el personaje, recorre preajustes, modifica canales RGB (±15) o restaura los colores
originales de fábrica. El botón "Edit portrait camera" abre el estudio fotográfico con
rotación de cámara, elevación, zoom y fondos personalizados (verde azulado, negro, arena,
RGB personalizado, burdeos o imágenes propias).

---

## Qué alcanza a los personajes que ya existen

Casi todo lo que editas es la plantilla de la raza, que Kenshi lee cuando
CONSTRUYE un personaje. Si la cambias, nadie que ya esté en el mundo se ve
afectado.

Esto sí se aplica al personaje seleccionado de inmediato:

- Las habilidades del personaje (no son una propiedad de la raza)
- La personalidad del personaje
- El rango de sangre
- Las ranuras de equipo
- La salud de las partes del cuerpo, tras una copia o una edición manual

Todo lo demás se aplica a los personajes que nazcan después del cambio.

---

## Cómo deshacer los cambios

- **Revert race** devuelve una raza a sus valores originales, incluidas sus
  partes del cuerpo.
- **Borrar `races.json`** (junto al DLL del mod) elimina todas las ediciones.
- **Cancelar la suscripción** restaura las razas originales tanto en partidas
  nuevas como en las existentes: nada de esto se escribe en tu partida guardada.

La excepción son las habilidades y la personalidad del personaje. Esas viven en
tu partida, como
cualquier otra habilidad que tu personaje haya ganado.

---

## Configuración

`config.json`, junto al DLL del mod.

**Cambiar la tecla** — escríbela por su nombre:

```json
"hotkey": { "key": "F5", "needsCtrl": false }
```

Aceptadas: `A`–`Z`, `0`–`9`, `F1`–`F12`, `HOME`, `END`, `INSERT`, `DELETE`,
`PGUP`, `PGDOWN`, `SPACE`, `ENTER`, `TAB`, `BACKSPACE`, `ESCAPE`, `GRAVE`,
`MINUS`, `EQUALS`, `LBRACKET`, `RBRACKET`, `SEMICOLON`, `APOSTROPHE`,
`BACKSLASH`, `COMMA`, `PERIOD`, `SLASH`.

Otras claves útiles:

| Clave | Qué hace |
|---|---|
| `window.width` / `window.height` | Tamaño de la ventana en píxeles |
| `stat.max` | Tope de habilidad en la página Character, por defecto 200 |
| `partHealth.max` | Tope de salud por parte del cuerpo, por defecto 500 |
| `store.applyOnStart` | Reaplica las ediciones guardadas al cargar. `false` para jugar sin ellas sin borrarlas |
| `followSelection` | La ventana sigue al personaje que seleccionas. `false` si sospechas de ello |
| `debugLog` | Registro detallado para informes de fallos, ver más abajo |

Un `config.json` ausente o mal formado no es un problema: se usan los valores
por defecto.

---

## Requisitos

- Kenshi **1.0.65**
- [RE_Kenshi](https://www.nexusmods.com/kenshi/mods/906)

---

## ¿Algo ha fallado?

Incluye el registro, por favor. Sin él, un informe es una suposición.

1. Abre `config.json` y pon `"debugLog": true`
2. Inicia el juego y reproduce el fallo
3. Adjunta `RE_Kenshi_log.txt` de tu carpeta de Kenshi

El registro indica la versión del mod y la tecla que abre la ventana, lo que ya
resuelve la mayoría de los informes. Vuelve a ponerlo en `false` después: ese
archivo lo comparten todos los plugins que tengas cargados.

**La ventana no se abre.** Puede que otro mod se haya quedado con Ctrl+K.
Cambia la tecla como se explica arriba.

---

## Limitaciones conocidas

- Una raza añadida por un mod que después desactives conserva su entrada en
  `races.json` en lugar de perder tu edición: simplemente no se aplica mientras
  ese mod esté desactivado, y vuelve cuando lo reactivas.
- La salud de las partes del cuerpo se aplica al personaje que tengas
  seleccionado. El resto de personajes de esa raza que ya estén en el mundo
  conservan la suya hasta que se construyan de nuevo.

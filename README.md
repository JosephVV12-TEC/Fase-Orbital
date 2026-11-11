# Fase Orbital

Videojuego de defensa orbital desarrollado en C++20 con Allegro 5 para el curso **Estructuras de Datos y Algoritmos**. El proyecto combina una interfaz gráfica con partidas multijugador por turnos, bots, simulación por lotes, pruebas automatizadas y registro CSV de resultados.

## Descripción

El jugador controla una nave que orbita alrededor de un núcleo y debe disparar a enemigos que avanzan desde el borde del campo. Los disparos tienen una de tres fases de color (cian, magenta o ámbar): acertar el color del enemigo permite destruirlo y sumar puntos; los impactos incorrectos penalizan el combo. La partida puede incluir hasta seis jugadores humanos y/o bots, que se alternan por oleadas en sentidos horarios y antihorarios.

El código separa el motor de reglas de la vista Allegro: las mismas reglas se utilizan en el modo gráfico, la simulación sin ventana y las pruebas. Las oleadas son reproducibles mediante una semilla y el generador congruencial lineal implementado en `Oleada.cpp`.

## Características

- Interfaz gráfica con menú, registro, anuncio de turno, juego, pausa, resumen de oleada, resultado final, estadísticas históricas y ranking.
- Partidas por turnos para entre uno y seis jugadores. Los alias que comienzan con `BOT` identifican a jugadores controlados por computadora.
- Seis tipos de enemigos: Dron, Girador, Divisor, Mini, Tanque y Camaleón; incluyen movimiento, resistencia y/o comportamientos específicos.
- Disparos automáticos, selección de fase de color, combos, puntuación, daño al núcleo y pulso de área recargable.
- Bot con una heurística fija: selecciona un enemigo objetivo cercano al núcleo, adopta su color y activa el pulso bajo condiciones de carga y cantidad de enemigos.
- Generación determinista de oleadas a partir de la semilla y el número de oleada.
- Modo por lotes sin ventana, con control mediante bots o archivo de acciones para jugadores humanos.
- Reporte de posiciones, métricas individuales y línea de tiempo de eventos por oleada.
- Historial CSV, estadísticas agregadas por alias y ranking Top 10.
- Catorce pruebas integradas para estructuras enlazadas, reglas del motor, reproducibilidad, estadísticas, persistencia y un escenario de memoria.
- Efectos visuales de partículas y sonidos para bajas, impactos erróneos, daño y pulso.

## Tecnologías utilizadas

| Categoría | Tecnología / dependencia |
|---|---|
| Lenguaje | C++20 (configurado en el proyecto de Visual Studio) |
| Framework multimedia | Allegro 5.2.11.3 |
| Paquete de dependencias | NuGet `AllegroDeps` 1.16.0 |
| Add-ons Allegro usados por la interfaz | Primitivas, font, TTF, imagen, audio y acodec |
| IDE / sistema de construcción incluido | Microsoft Visual Studio, proyecto MSBuild `.vcxproj` y solución `.slnx` |
| Plataforma configurada | Windows 10 SDK; configuraciones Win32 y x64 |

El proyecto no incluye `CMakeLists.txt` ni un sistema de construcción CMake. El `.vcxproj` también marca add-ons adicionales de Allegro como dependencias de configuración; la vista fuente utiliza los add-ons indicados arriba.

## Arquitectura del proyecto

### Módulos principales

| Archivos | Responsabilidad |
|---|---|
| `Fase-Orbital.cpp` | Punto de entrada. Selecciona la interfaz gráfica, lotes, reportes, pruebas o generación de partidas de bots según los argumentos. |
| `VistaAllegro.cpp` / `VistaAllegro.h` | Ventana, ciclo de eventos a 30 ticks por segundo, dibujo, pantallas, entradas y audio. Traduce las entradas del usuario o del bot a acciones del motor. |
| `Motor.cpp` / `Motor.h` | Simulación por tick: movimiento y colisiones, disparos, daño, puntuación, pulso, bajas y registro de eventos. No depende de la interfaz gráfica. |
| `Partida.cpp` / `Partida.h` | Registro de jugadores, flujo de turnos y oleadas, eliminación, clasificación, informes e historial/estadísticas/ranking. |
| `Oleada.cpp` / `Oleada.h` | Construcción determinista de la cola de apariciones con el generador congruencial lineal. |
| `Bot.cpp` / `Bot.h` | Elección de objetivo, color y activación de pulso para el jugador automático. |
| `ModoLotes.cpp` / `ModoLotes.h` | Simulación completa sin ventana, parser de scripts, salida de reportes y persistencia CSV. |
| `Pruebas.cpp` / `Pruebas.h` | Suite de pruebas P01–P14, sin dependencia de Allegro. |
| `Jugador.cpp` / `Jugador.h` | Estado y métricas de los jugadores, además de cálculos de precisión y eficiencia. |
| `ColaSpawn.cpp` / `ColaSpawn.h` | Cola FIFO de apariciones de enemigos, incluyendo copia profunda para reutilizar la oleada en los turnos de cada jugador. |
| `RondaJugadores.cpp` / `RondaJugadores.h` | Ronda circular doblemente enlazada de jugadores activos. |
| `ListaEnemigos.h` y `ListaBalas.h` | Definiciones de listas dinámicas para enemigos y balas; las operaciones de lista también se implementan en `FuncionesJuego.h`. |

### Flujo y decisiones de diseño

`Partida` genera una oleada y conserva su cola original. Al iniciar cada turno, `Motor` recibe una copia profunda para que cada jugador enfrente la misma secuencia. El motor procesa el tick y devuelve el resultado; la vista consulta ese resultado y se ocupa de mostrarlo, sin decidir las reglas de juego. Por eso el motor, el modo por lotes y las pruebas pueden ejecutarse sin abrir una ventana Allegro.

Se emplean estructuras enlazadas propias en lugar de contenedores de biblioteca: la lista doble de enemigos facilita inserciones y eliminaciones durante colisiones y divisiones; la cola de spawn conserva orden FIFO; la lista circular de jugadores permite alternar sentido y retirar jugadores eliminados. Para los arreglos pequeños y acotados del juego se utilizan arreglos de tamaño fijo, por ejemplo el máximo de seis jugadores.

## Estructuras de datos implementadas

| Estructura | Tipo | Uso |
|---|---|---|
| `ListaEnemigos` / `NodoEnemigo` | Lista doblemente enlazada con cabeza, cola y tamaño | Mantiene enemigos activos; permite recorrer en ambos sentidos, insertar los dos Mini generados por un Divisor y retirar enemigos durante la simulación. |
| `ListaBalas` / `NodoBala` | Lista simplemente enlazada | Guarda balas activas. La inserción ocurre al inicio y `eliminarSi` recorre mediante puntero a puntero para retirar las balas impactadas o fuera del campo, incluidas las consecutivas y la cabeza. |
| `ColaSpawn` / `NodoSpawn` | Cola FIFO enlazada con frente, final y tamaño | Almacena las apariciones programadas. Implementa copia profunda para que el motor consuma una copia por turno sin alterar la oleada compartida. |
| `RondaJugadores` / `nodoRonda` | Lista circular doblemente enlazada de punteros a jugadores | Administra el jugador actual, la rotación horaria/antihoraria y la eliminación de participantes sin destruir los objetos `Jugador` que se conservan para el reporte. |
| `BitacoraEventos` / `NodoEvento` | Lista simplemente enlazada con cabeza y cola | Registra eventos cronológicos de la partida por tick, oleada y jugador; se usa para la línea de tiempo de reportes y las métricas. |
| `HistorialPartidas` / `NodoFila` | Lista simplemente enlazada con cabeza, cola y tamaño | Mantiene en memoria las filas cargadas o creadas desde el CSV. La inserción al final es O(1). |
| `NodoNum` | Lista simplemente enlazada de valores numéricos | Almacena puntajes temporalmente; `mergeSortLista` la ordena para calcular la mediana y se usa también en el cálculo de dispersión. |
| `Ranking` / `NodoRanking` | Lista simplemente enlazada ordenada por inserción | Construye el Top 10 por mejor puntaje, desempata por precisión promedio y luego por alias alfabético. |
| Arreglos de tamaño fijo | Arreglos C++ | Guardan jugadores (máximo 6), métricas por tipo/color, resumen de hasta 50 alias, efectos de partículas (máximo 300) y resultados temporales. |

Las estructuras que poseen nodos dinámicos liberan esos nodos mediante destructores o funciones de vaciado. Las listas de enemigos y balas, el historial y la ronda deshabilitan la copia superficial; `ColaSpawn` implementa copia profunda.

## Requisitos

- Windows con Microsoft Visual Studio y componentes de desarrollo de C++ instalados. El proyecto configura el SDK de Windows 10 y el toolset `v145`.
- Soporte del compilador para C++20.
- Restaurar los paquetes NuGet declarados en `Fase Orbital/packages.config`: `Allegro` 5.2.11.3 y `AllegroDeps` 1.16.0.
- Mantener la carpeta de trabajo en `Fase Orbital` al ejecutar, ya que las rutas de imágenes, sonidos e historial son relativas a ella.
- Para la interfaz gráfica, Allegro debe inicializarse correctamente. Los archivos WAV se cargan si el audio está disponible; si no hay dispositivo de audio, el juego avisa y continúa en silencio.

La compilación y ejecución en sistemas operativos distintos de Windows no están configuradas ni documentadas en los archivos del repositorio.

## Instalación

1. Clona o descarga este repositorio.
2. Abre `Fase Orbital.slnx` con una versión de Visual Studio que soporte el formato de solución incluido y el toolset `v145`.
3. Restaura los paquetes NuGet de la solución. El proyecto comprueba que estén disponibles `AllegroDeps.1.16.0` y `Allegro.5.2.11.3`.
4. Selecciona la plataforma `x64` (o `x86`) y la configuración `Debug` o `Release`.
5. Compila el proyecto y configura `Fase Orbital` como directorio de trabajo para ejecutar la aplicación con sus rutas de recursos relativas.

## Compilación

La forma de compilación proporcionada por el repositorio es mediante la solución y el proyecto de Visual Studio; no hay instrucciones de CMake ni un `CMakeLists.txt`.

1. Abre `Fase Orbital.slnx`.
2. Selecciona `Release` y `x64` en las configuraciones de Visual Studio.
3. Usa **Compilar > Compilar solución**.
4. Ejecuta el programa desde Visual Studio con el directorio de trabajo establecido en `Fase Orbital` para que se resuelvan `assets/...` e `historial.csv`.

El proyecto incluye configuraciones `Debug` y `Release` para `Win32` y `x64`, usa C++20 y declara Allegro mediante NuGet. La ruta final del ejecutable depende de la configuración local de MSBuild.

## Ejecución

Sin argumentos, el programa abre la interfaz gráfica. Los ejemplos siguientes muestran la sintaxis disponible en el punto de entrada; sustituye `<ejecutable>` por la ruta del `.exe` compilado y ejecútalo con `Fase Orbital` como directorio de trabajo:

```text
<ejecutable>
<ejecutable> --lotes --jugadores Ana,Luis,BOT1 --semilla 42 --oleadas 10 --historial historial.csv
<ejecutable> --estadisticas historial.csv
<ejecutable> --ranking historial.csv
<ejecutable> --pruebas
<ejecutable> --generar-bots 30 historial.csv
<ejecutable> --ayuda
```

La interfaz gráfica carga `historial.csv` al inicio y guarda las partidas gráficas completadas allí. Si no existe todavía, el código trata el historial como vacío. Las rutas de recursos y CSV se interpretan con respecto al directorio de trabajo actual.

### Script del modo por lotes

`--script` acepta una acción por línea con formato `jugador oleada tick ACCION [parametro]`. Las líneas vacías y las que comienzan con `#` se ignoran. Acciones reconocidas:

| Acción | Parámetro |
|---|---|
| `COLOR` | `C`, `M` o `A` (también `0`, `1` o `2`) |
| `GIRAR_A` | Ángulo numérico en grados |
| `PULSO` | Sin parámetro requerido |

Ejemplo de formato tomado de los casos de prueba:

```text
# jugador oleada tick ACCION [parametro]
Ana 1 0 COLOR M
Ana 1 75 GIRAR_A 135
Ana 2 300 PULSO
```

Las acciones se aplican al jugador, oleada y tick indicados. Las líneas mal formadas se notifican con su número y se omiten.

## Controles

### Menú

| Tecla | Acción |
|---|---|
| `↑` / `↓` | Cambiar opción seleccionada |
| `Enter` | Confirmar |
| `1` / `2` / `3` / `4` | Jugar / estadísticas / ranking / salir |
| `Esc` | Salir desde la opción 4 |

### Registro y partida

| Tecla o entrada | Acción |
|---|---|
| Escribir letras, números o `_` | Escribir alias (máximo 12 caracteres) |
| `Backspace` | Borrar el último carácter del alias |
| `Enter` | Registrar alias; si está vacío se genera `JugadorN` |
| `Tab` | Agregar un bot (`BOTN`) |
| `←` / `→` en registro | Reducir/aumentar el límite de oleadas (mínimo 1, máximo 50) |
| `F2` | Iniciar la partida si hay al menos un jugador registrado |
| Mouse | Orientar la nave hacia la posición del cursor |
| `←` / `→` durante el juego | Girar la nave |
| `1` / `2` / `3` durante el juego | Seleccionar fase cian / magenta / ámbar |
| `Espacio` | Solicitar pulso (cuando la carga está completa) |
| `P` o `Esc` durante el juego | Pausar; las mismas teclas reanudan |
| `Q` en pausa | Abandonar la partida y volver al menú |
| `Enter` | Empezar o continuar desde anuncios/resúmenes; volver desde el resultado final, ranking o estadísticas |
| `↑` / `↓` en estadísticas | Seleccionar jugador histórico |

Al perder el foco de la ventana durante el juego, la aplicación activa la pausa automáticamente. En el registro también se acepta `Esc` para volver al menú.

## Parámetros de línea de comandos

El uso impreso por el ejecutable es:

| Opción | Argumentos | Descripción y restricciones |
|---|---|---|
| *(sin argumentos)* | — | Abre la interfaz gráfica. |
| `--lotes` | `--jugadores lista [--semilla n] [--oleadas n] [--script archivo] [--historial archivo]` | Ejecuta una partida completa sin ventana. `--jugadores` es obligatorio; admite de 1 a 6 alias distintos separados por comas. Valores por defecto del código: semilla `42`, `10` oleadas e historial `historial.csv`. `--oleadas` debe estar entre 1 y 200; el historial puede desactivarse en la API con ruta vacía. |
| `--estadisticas` | `archivo.csv` | Carga el historial indicado e imprime estadísticas agregadas. |
| `--ranking` | `archivo.csv` | Carga el historial indicado e imprime el Top 10. |
| `--pruebas` | — | Ejecuta las pruebas P01–P14; devuelve código cero si pasan todas. |
| `--generar-bots` | `cantidad archivo.csv` | Genera partidas de cuatro bots y las agrega al CSV. El punto de entrada usa semilla inicial `1000` y `10` oleadas por partida. |
| `--ayuda` / `-h` | — | Muestra la ayuda. |

Para `--lotes`, se rechazan argumentos desconocidos o sin valor. El número de oleadas del modo gráfico tiene por defecto 10 y puede ajustarse entre 1 y 50 en la pantalla de registro. El límite del modo por lotes admite hasta 200 oleadas.

## Persistencia de datos

El archivo `Fase Orbital/historial.csv` contiene una fila por jugador y partida. El encabezado actual define 33 columnas: identificador y semilla de partida, modo, alias, condición de bot, posición, puntaje y oleada, disparos/impactos, combo, pulsos, daño y ticks, bajas por cada uno de los seis tipos de enemigo, impactos y aciertos por fase, y daño por tipo de enemigo. El lector también acepta filas antiguas de 21 columnas.

Al completar una partida gráfica se agregan filas con modo `grafico`; el modo por lotes usa `lotes`. Las filas inválidas se notifican con archivo y número de línea y se omiten. Las estadísticas calculadas incluyen partidas, victorias, puntajes mejor/peor, media, mediana, desviación estándar poblacional, precisión promedio, fase con mejor precisión, enemigo que más daño causó e histograma de diez intervalos. El ranking usa el mejor puntaje de cada alias y desempata por precisión y orden alfabético.

`historial.csv` está versionado y ya contiene registros de ejemplo. Las pruebas de persistencia escriben archivos CSV temporales y los eliminan al finalizar.

## Pruebas

No se incluye un framework externo de pruebas. La suite propia se ejecuta con:

```text
<ejecutable> --pruebas
```

`Pruebas.cpp` ejecuta P01–P14 y muestra el resultado individual, el total aprobado y un código de salida distinto de cero si falla alguna. Cubre:

- P01–P03: recorrido, eliminación e inserción de `ListaEnemigos`.
- P04: eliminación de balas consecutivas en `ListaBalas`.
- P05–P06: FIFO, vaciado y copia profunda de `ColaSpawn`.
- P07: rotación, eliminación y validez de enlaces de `RondaJugadores`.
- P08: reproducibilidad con semilla y script.
- P09–P11: impacto incorrecto, división de un Divisor y reglas del pulso en el motor.
- P12: merge sort enlazado, mediana, media, desviación e histograma.
- P13: escritura/carga CSV, persistencia, estadísticas y ranking.
- P14: partida de cuatro bots y diez oleadas; el comentario de la prueba indica que la verificación de fugas se realiza externamente con Valgrind, AddressSanitizer o CRT.

## Estructura del proyecto

```text
.
├── .github/
│   └── copilot-instructions.md
├── Fase Orbital.slnx
├── Fase Orbital/
│   ├── Fase Orbital.vcxproj
│   ├── Fase Orbital.vcxproj.filters
│   ├── Fase-Orbital.cpp
│   ├── Bot.cpp / Bot.h
│   ├── ColaSpawn.cpp / ColaSpawn.h
│   ├── FuncionesJuego.h
│   ├── Jugador.cpp / Jugador.h
│   ├── Juego.h
│   ├── ListaBalas.h / ListaEnemigos.h
│   ├── ModoLotes.cpp / ModoLotes.h
│   ├── Motor.cpp / Motor.h
│   ├── Oleada.cpp / Oleada.h
│   ├── Partida.cpp / Partida.h
│   ├── Pruebas.cpp / Pruebas.h
│   ├── RondaJugadores.cpp / RondaJugadores.h
│   ├── VistaAllegro.cpp / VistaAllegro.h
│   ├── packages.config
│   ├── historial.csv
│   └── assets/
│       ├── fonts/
│       │   └── PON_AQUI_fuente.ttf.txt
│       ├── images/
│       │   └── fondo.png
│       └── sounds/
│           ├── baja.wav
│           ├── dano.wav
│           ├── erroneo.wav
│           └── pulso.wav
├── photos/
│   └── fanclub.jpg
└── README.md
```

## Capturas de pantalla

No se identificaron capturas de la interfaz del juego en los recursos del proyecto. Agrega imágenes propias en `photos/` y actualiza estos marcadores:

- **Menú principal:** `![Menú principal](./photos/menu.png)` — pendiente de agregar.
- **Juego en curso:** `![Juego en curso](./photos/juego.png)` — pendiente de agregar.
- **Estadísticas o ranking:** `![Estadísticas y ranking](./photos/estadisticas-ranking.png)` — pendiente de agregar.

## Integrantes

Integrantes que aparecen en el README previo del repositorio:

| Integrante | Carné |
|---|---|
| Joseph Vargas Villalta | 2026010975 |
| Luis Josué Rosales Castillo | 2026016613 |
| María Luisa Lalyre Castillo | 2025068971 |

## Créditos

- **Allegro 5** y `AllegroDeps`, declarados como paquetes NuGet del proyecto.
- El repositorio incluye `assets/images/fondo.png` y los sonidos `baja.wav`, `dano.wav`, `erroneo.wav` y `pulso.wav`. No se encontró información de autoría o licencia de estos recursos en los archivos revisados: **Pendiente de completar**.
- La carpeta `assets/fonts` contiene un archivo marcador vacío llamado `PON_AQUI_fuente.ttf.txt`, no un archivo TTF. La aplicación intenta cargar `assets/fonts/fuente.ttf`, después Arial en `C:\Windows\Fonts\arial.ttf` y, si no encuentra ninguna, usa la fuente interna de Allegro.

## Licencia

**Pendiente de completar.** No se encontró un archivo de licencia en el repositorio.
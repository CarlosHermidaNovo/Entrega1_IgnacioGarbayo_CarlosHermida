# Grúa

**Autores:** Ignacio Garbayo y Carlos Hermida (Entrega 1)

---

## Características Implementadas

* **Renderizado Moderno:** Uso de *Programmable Pipeline* (Vertex y Fragment shaders). Renderizado de color plano (*Flat Shading*) mediante color uniforme por pieza.
* **Gestión de Memoria GPU:** Los vértices del cubo y la baldosa se envían una sola vez a la VRAM mediante VBOs y VAOs, reutilizándolos para todas las piezas de la grúa y todas las baldosas del suelo.
* **5 Estructuras Jerárquicas:** El sistema mecánico se modela con las estructuras `Base`, `Cabina`, `Articulacion`, `Brazo` y `GruaCamion` (que agrupa las cuatro anteriores). Cada una almacena sus propios parámetros de transformación (posición, ángulos, velocidades, extensión).
* **Transformaciones Jerárquicas:** Las matrices de modelo se construyen de forma acumulativa. La cabina hereda la transformación de la base, la articulación hereda la de la cabina, y el brazo hereda la de la articulación, formando una cadena padre-hijo completa.
* **Física de Conducción:** El movimiento del vehículo es progresivo. La aceleración (W) y el freno/marcha atrás (X) modifican la velocidad gradualmente. La fricción detiene el vehículo de forma natural cuando no se pulsa ninguna tecla. La dirección (A/D) solo actúa si el vehículo está en movimiento y su sentido se invierte al ir en marcha atrás.
* **Controles de la Grúa:** La cabina giratoria (Q/E), el ángulo de elevación del brazo (R/F) y la extensión del brazo (T/G) se controlan de forma independiente. La articulación (naranja) tiene una colisión dura contra la base/chasis (amarillo): si toca el volumen del chasis, el movimiento se detiene (sin rebotes), evitando penetraciones. La extensión está limitada entre 2.0 y 8.0 unidades.
* **Suelo Modular:** El escenario se renderiza como un mosaico de 41×41 baldosas generado mediante bucles, con alternancia de colores gris claro y gris oscuro para crear un efecto de tablero de ajedrez.
* **Límite de Área de Conducción:** El vehículo no puede salirse del área del suelo. Al alcanzar el borde, la posición se restringe y la velocidad se anula para evitar que el vehículo se quede empujando contra el límite.
* **3 Modos de Cámara:** Cámara en tercera persona que sigue al vehículo (1), cámara en primera persona desde la cabina del conductor (2) y cámara exterior cenital con ligera inclinación para ver todo el escenario (3).
* **Independencia de FPS (`deltaTime`):** Todos los movimientos (conducción, giro de cabina, elevación, extensión) se calculan en base al tiempo real transcurrido entre fotogramas, garantizando fluidez independiente de los FPS.

---

## Controles

| Tecla | Acción |
| :--- | :--- |
| `W` | **Acelerar** hacia delante (aceleración progresiva). |
| `X` | **Frenar / Marcha atrás** (deceleración progresiva). |
| `A` | **Girar a la izquierda** (solo si el vehículo está en movimiento). |
| `D` | **Girar a la derecha** (solo si el vehículo está en movimiento). |
| `Q` | **Rotar cabina** en sentido antihorario. |
| `E` | **Rotar cabina** en sentido horario. |
| `R` | **Elevar brazo** (aumentar ángulo de articulación). |
| `F` | **Bajar brazo** (reducir ángulo de articulación). |
| `T` | **Extender brazo**. |
| `G` | **Recoger brazo**. |
| `1` | **Cámara Tercera Persona:** Sigue al vehículo desde atrás. |
| `2` | **Cámara Primera Persona:** Vista desde la cabina del conductor. |
| `3` | **Cámara Cenital:** Vista exterior desde arriba con inclinación. |
| `ESC` | Cierra la aplicación. |

---

## Estructura del Proyecto

* `mainGrua.cpp`: Contiene el bucle principal, la inicialización de dependencias, las estructuras de datos de la grúa, la lógica de física y actualización de matrices, y la gestión de entradas y cámaras.
* `Grua.vert`: Código fuente del *Vertex Shader*, calcula la posición final de cada vértice aplicando las matrices Modelo, Vista y Proyección.
* `Grua.frag`: Código fuente del *Fragment Shader*, aplica el color sólido uniforme (`colorObjeto`) a cada fragmento.

---

## Tabla de dependencias incluidas en Include_Lib

| Librería | Uso |
| :--- | :--- |
| GLFW | Creación de ventana y contexto OpenGL, gestión de eventos de teclado. |
| GLAD | Carga de punteros a funciones de OpenGL en tiempo de ejecución. |
| GLM | Matemáticas de matrices y vectores (transformaciones, cámara). |
| BibliotecasCurso (`lecturaShader_0_9.h`) | Compilación y enlazado de shaders GLSL desde archivo. |

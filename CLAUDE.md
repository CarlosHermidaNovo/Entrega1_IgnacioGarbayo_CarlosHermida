# Entrega I - Simulación de Sistemas Jerárquicos en OpenGL 3.3

## Condiciones de Entrega y Formato

- Presentación: La práctica debe entregarse en el Campus Virtual y ser presentada y defendida en clase. Las entregas no defendidas se evaluarán como "No Presentadas".
- Grupos: Se permite el trabajo por parejas.
- Contenido del Archivo: El archivo comprimido debe incluir:
- Código fuente completo (.cpp, .h, shaders).
- Proyecto/Makefiles necesarios para la compilación.
- Ejecutable funcional junto con sus dependencias (.dll, carpetas de recursos).

## Sistema Solar

El objetivo es representar la mecánica celeste mediante transformaciones acumulativas.

1. Estructura de Datos: Cada cuerpo celeste debe definirse mediante una estructura en C que incluya posición (px, py, pz), ángulos de rotación y traslación, velocidades, escala, color y el puntero a su VAO.
2. Lógica Temporal: El movimiento debe gestionarse mediante una función que reciba la estructura y actualice sus ángulos en base al tiempo transcurrido (lapsoTime), garantizando fluidez independiente de los FPS.
3. Contenido Mínimo: Todos los planetas del sistema solar, la Luna y la Estación Espacial Internacional (ISS).
4. Órbitas: Se debe dibujar la trayectoria de cada planeta mediante líneas (GL_LINE_LOOP) para visualizar su camino.
5. Interactividad y Modo Telescopio:
6. Re-escalado: La aplicación debe ajustar el viewport correctamente al cambiar el tamaño de la ventana.
7. Cámaras Dinámicas: Implementar un menú numérico para seleccionar objetivos. Ejemplo: Tecla 1 (Enfocar Marte), Tecla 2 (Enfocar Tierra desde la Luna), Tecla 3 (Enfocar la ISS desde la Tierra).

## Grúa

Representación de un sistema mecánico móvil sobre un terreno modular.

1. Estructura Mecánica: Definir una estructura para cada parte (Base, Cabina, Articulación, Brazo) que almacene sus estados de transformación.
2. Suelo Modular: El escenario debe ser un suelo diseñado mediante un mosaico de cuadrados o cubos utilizando bucles de renderizado.
3. Física de Conducción: El vehículo debe controlarse con las teclas: W (Acelerador) / X (Frenar-Marcha atrás): Deben modificar la velocidad de forma progresiva. A (Izquierda) / D (Derecha): Deben modificar el ángulo de dirección.
4. Modos de Cámara: Primera Persona: Cámara situada sobre la cabina del conductor. Tercera Persona: Cámara que sigue al vehículo desde una distancia fija. Exterior/Cenital: Cámara libre o fija para ver todo el escenario.

## Criterios de Evaluación

Los criterios de evaluación se centrarán en:

1. Alcance de los objetivos: Realización de los puntos antes presentados.
2. Calidad Visual: Uso correcto de colores, proporciones y coherencia visual fluidez
3. Funcionamiento: Estabilidad del programa y suavidad de las animaciones.
4. Buenas Prácticas: Código modular, uso eficiente de funciones de OpenGL y gestión de memoria (borrado de VAOs/VBOs).
5. Limpieza de código.
6. Defensa Oral: Claridad en la explicación del código y resolución de dudas planteadas por el profesor.
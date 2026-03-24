# Sistema Solar

**Autores:** Ignacio Garbayo y Carlos Hermida (Entrega 1)

---

## Características Implementadas

* **Renderizado Moderno:** Uso de *Programmable Pipeline* (Vertex y Fragment shaders). Actualmente utilizando renderizado de color plano (*Flat Shading*).
* **Gestión de Memoria GPU:** Los vértices del modelo 3D (esfera) se envían una sola vez a la VRAM mediante VBOs y VAOs, optimizando el rendimiento.
* **Transformaciones Jerárquicas:** Sistema de padres e hijos. La Luna calcula su posición basándose en la posición espacial de la Tierra, y la ISS basándose en la de la Tierra.
* **Independencia de FPS (`deltaTime`):** La velocidad de la simulación física (traslación y rotación) está desvinculada de los fotogramas por segundo del monitor, garantizando que el paso del tiempo sea consistente en cualquier ordenador.
* **Órbitas Visibles:** Trazado en tiempo real de la ruta orbital de cada cuerpo celeste usando `GL_LINE_LOOP`.
* **Cámaras Dinámicas:** Sistema de cámara espectador interactivo que permite "saltar" entre diferentes cuerpos celestes en tiempo real recalculando la matriz de Vista (`View Matrix`).
* **Re-escalado Dinámico:** El *viewport* se ajusta automáticamente al cambiar el tamaño de la ventana de la aplicación sin deformar la relación de aspecto de los planetas.
* **Proporciones Ajustadas:** Los valores de escalado de todos los cuerpos celestes han sido revisados para aproximarse mejor a las proporciones relativas reales (Júpiter y Saturno más grandes, planetas interiores más pequeños). Afecta únicamente a los parámetros `escalado` de cada `Objeto` en `main()`.
* **ISS con Modelo 3D OBJ:** La Estación Espacial Internacional se renderiza usando un modelo `.obj` externo cargado en tiempo de ejecución (`iss.obj`), en lugar de la esfera genérica. Se implementó la función `cargarOBJ()` que parsea el archivo, extrae posiciones XYZ y sube la geometría a la GPU en un VAO/VBO propio. El sistema de dibujo se generalizó para que cada `Objeto` use su propio VAO y `numVertices` en lugar de los globales de la esfera, sin romper el resto de planetas.
* **Anillos de Saturno:** Saturno incluye 4 anillos concéntricos diferenciados (C, B, A y F), aproximando los anillos reales del planeta. Cada anillo es un disco plano (`GL_TRIANGLE_STRIP`) generado proceduralmente con radio interno y externo propios, y dibujado con su color característico. Se aplica la inclinación axial real de Saturno (~27°) respecto al plano orbital. El backface culling se desactiva temporalmente durante su renderizado al tratarse de superficies planas.
* **Modo Telescopio desde la Tierra:** Cámara posicionada en la Tierra que permite observar cualquier cuerpo celeste del sistema. La tecla `5` activa el modo y, pulsándola repetidamente, cicla entre los 10 objetivos disponibles (Sol, Mercurio, Venus, Marte, Júpiter, Saturno, Urano, Neptuno, Luna, ISS).
* **Rotación Manual de la Cámara:** Las flechas del teclado permiten rotar la vista en cualquier modo de cámara, aplicando rotaciones incrementales sobre los ejes X (arriba/abajo) e Y (izquierda/derecha) a la matriz de vista.

---

## Controles de Cámara

El programa incluye un modo "Telescopio" controlado numéricamente:

| Tecla | Acción |
| :--- | :--- |
| `0` | **Vista General:** Cámara cenital enfocando al Sol. |
| `1` | **Seguimiento:** Cámara enfocada y siguiendo a Marte. |
| `2` | **Vista Satélite:** Cámara posicionada en la Luna enfocando a la Tierra. |
| `3` | **Vista Orbital:** Cámara posicionada en la Tierra enfocando a la ISS. |
| `4` | **Saturno:** Cámara lateral elevada para apreciar los anillos con su inclinación axial. |
| `5` | **Telescopio desde la Tierra:** Primera pulsación activa el modo telescopio; pulsaciones sucesivas ciclan el objetivo entre todos los cuerpos celestes (Sol, Mercurio, Venus, Marte, Júpiter, Saturno, Urano, Neptuno, Luna, ISS). |
| `←↑↓→` | **Rotación manual de la cámara:** Las flechas rotan la vista en cualquier modo de cámara. |
| `ESC` | Cierra la aplicación. |


## Estructura del Proyecto

* `mainSistemaSolar.cpp`: Contiene el bucle principal de la aplicación, la inicialización de dependencias, la lógica de actualización física (movimiento orbital) y la gestión de inputs.
* `esfera.h`: Contiene el array estático de vértices, normales y coordenadas de textura pre-calculadas del modelo 3D de la esfera (1080 vértices).
* `SistemaSolar.vert`: Código fuente del *Vertex Shader*, encargado de calcular las posiciones finales de los vértices multiplicándolos por las matrices de transformación.
* `SistemaSolar.frag`: Código fuente del *Fragment Shader*, encargado de aplicar el color sólido a los píxeles de los planetas y las líneas de sus órbitas.
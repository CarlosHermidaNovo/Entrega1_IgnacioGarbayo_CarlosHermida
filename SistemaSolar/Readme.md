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

---

## Controles de Cámara

El programa incluye un modo "Telescopio" controlado numéricamente:

| Tecla | Acción |
| :--- | :--- |
| `0` | **Vista General:** Cámara cenital enfocando al Sol. |
| `1` | **Seguimiento:** Cámara enfocada y siguiendo a Marte. |
| `2` | **Vista Satélite:** Cámara posicionada en la Luna enfocando a la Tierra. |
| `3` | **Vista Orbital:** Cámara posicionada en la Tierra enfocando a la ISS. |
| `ESC` | Cierra la aplicación. |


## Estructura del Proyecto

* `mainSistemaSolar.cpp`: Contiene el bucle principal de la aplicación, la inicialización de dependencias, la lógica de actualización física (movimiento orbital) y la gestión de inputs.
* `esfera.h`: Contiene el array estático de vértices, normales y coordenadas de textura pre-calculadas del modelo 3D de la esfera (1080 vértices).
* `SistemaSolar.vert`: Código fuente del *Vertex Shader*, encargado de calcular las posiciones finales de los vértices multiplicándolos por las matrices de transformación.
* `SistemaSolar.frag`: Código fuente del *Fragment Shader*, encargado de aplicar el color sólido a los píxeles de los planetas y las líneas de sus órbitas.
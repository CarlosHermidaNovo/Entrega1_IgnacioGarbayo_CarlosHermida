# Entrega I — Simulación de Sistemas Jerárquicos en OpenGL 3.3

**Autores:** Ignacio Garbayo y Carlos Hermida
**Asignatura:** Computación Gráfica — USC 2026

---

## Descripción

Práctica de representación de sistemas jerárquicos mediante transformaciones acumulativas en OpenGL 3.3 (programmable pipeline). Incluye dos proyectos independientes:

- **Sistema Solar:** simulación de la mecánica celeste con todos los planetas, la Luna y la ISS.
- **Grúa:** sistema mecánico móvil controlable sobre un terreno modular. *(en desarrollo)*

---

## Estructura del Repositorio

```
Entrega 1/
├── Include_Lib/          # Dependencias compartidas (GLFW, GLAD, GLM, headers del curso)
├── SistemaSolar/         # Proyecto Sistema Solar
│   ├── Makefile          # Makefile para compilar desde terminal
│   └── SistemaSolar/
│       └── SistemaSolar/ # Código fuente y shaders
├── Grua/                 # Proyecto Grúa (en desarrollo)
└── docs/                 # Documentación adicional
```

---

## Compilación y Ejecución

### Opción A — PowerShell / terminal (GNU Make + MinGW)

Requiere tener instalados **GNU Make** y **MinGW (g++)** y ambos disponibles en el PATH.

**Sistema Solar:**
```powershell
cd SistemaSolar
make run
```

La primera vez, `make` genera automáticamente la librería de importación de GLFW compatible con MinGW (`libglfw3.a`) usando `dlltool`. A partir de ahí la compilación es directa.

Comandos disponibles:

| Comando      | Acción                                      |
| :----------- | :------------------------------------------ |
| `make`       | Compila el proyecto                         |
| `make run`   | Compila y ejecuta                           |
| `make clean` | Elimina el ejecutable y archivos generados  |

> Los shaders y la DLL de GLFW se copian automáticamente junto al ejecutable al compilar.

### Opción B — Visual Studio 2022

Abrir el archivo de solución y compilar con F5 o Ctrl+F5:

```
SistemaSolar/SistemaSolar/SistemaSolar.slnx
```

La configuración del proyecto ya incluye las rutas a `Include_Lib` y enlaza con `glfw3.lib` automáticamente.

---

## Objetivos de la Práctica

### Sistema Solar

1. Estructura de datos por cuerpo celeste (posición, rotación, traslación, velocidad, escala, color, VAO).
2. Movimiento basado en `deltaTime` para fluidez independiente de los FPS.
3. Contenido mínimo: todos los planetas, la Luna y la ISS.
4. Órbitas visibles mediante `GL_LINE_LOOP`.
5. Re-escalado correcto del viewport al cambiar el tamaño de la ventana.
6. Cámaras dinámicas seleccionables con teclado numérico.

### Grúa

1. Estructura mecánica con Base, Cabina, Articulación y Brazo.
2. Suelo modular mediante mosaico de cuadrados/cubos.
3. Control progresivo de velocidad y dirección (W/X/A/D).
4. Modos de cámara: primera persona, tercera persona y vista cenital.

---

## Criterios de Evaluación

1. Alcance de los objetivos implementados.
2. Calidad visual: colores, proporciones y fluidez.
3. Estabilidad y suavidad de las animaciones.
4. Buenas prácticas: código modular, uso eficiente de VAOs/VBOs.
5. Limpieza de código.
6. Defensa oral.

---

## Dependencias

Incluidas en `Include_Lib/`:

| Librería | Uso |
| :--- | :--- |
| GLFW 3 | Gestión de ventana y contexto OpenGL |
| GLAD | Carga de funciones OpenGL |
| GLM | Matemáticas (matrices, vectores) |
| `esfera.h` | Geometría de esfera pre-calculada |
| `lecturaShader_0_9.h` | Utilidad de carga de shaders |

---

## Créditos de Assets

| Asset | Autor | Licencia |
| :--- | :--- | :--- |
| [International Space Station](https://skfb.ly/onMDA) | calebcram (Sketchfab) | [CC Attribution 4.0](http://creativecommons.org/licenses/by/4.0/) |

> "International Space Station" (https://skfb.ly/onMDA) by calebcram is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).

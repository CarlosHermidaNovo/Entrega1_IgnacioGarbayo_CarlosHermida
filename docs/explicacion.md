# Explicación técnica completa: Grúa y Sistema Solar (OpenGL 3.3)

## 1. Objetivo de este documento

Este documento explica, de forma integral y justificando cada decisión relevante, cómo funciona el código de los dos programas de la entrega:

- Sistema Solar
- Grúa (camión grúa sobre suelo modular)

La explicación cubre:

1. Flujo completo de renderizado OpenGL: desde datos en CPU y buffers de entrada hasta salida en framebuffer y pantalla.
2. Estructuras de datos de cada simulación.
3. Función de cada bloque principal del código.
4. Lógica física y temporal en ambos programas (cinemática orbital y cinemática vehicular/articulada).
5. Justificación de decisiones de implementación y limitaciones actuales.

## 2. Arquitectura global del proyecto

Ambos ejecutables comparten el mismo patrón de arquitectura gráfica:

1. Crear contexto OpenGL con GLFW.
2. Cargar funciones OpenGL con GLAD.
3. Compilar y enlazar shaders con la utilidad compartida setShaders.
4. Crear VAO/VBO para geometría.
5. En cada frame:
   - actualizar estado físico con deltaTime,
   - calcular matrices,
   - enviar uniforms,
   - lanzar draw calls.
6. Presentar imagen final con glfwSwapBuffers.
7. Liberar recursos de GPU al final.

Componentes comunes relevantes:

- GLFW: ventana, eventos de teclado, contexto OpenGL, swap buffers.
- GLAD: resolución de punteros de funciones OpenGL.
- GLM: matemáticas (vectores, matrices, transformaciones).
- lecturaShader_0_9.h: lectura de ficheros shader, compilación y link del programa de shaders.

```cpp
glfwInit();
GLFWwindow* window = glfwCreateWindow(800, 600, "App", nullptr, nullptr);
glfwMakeContextCurrent(window);
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
shaderProgram = setShaders("*.vert", "*.frag");
```

---

## 3. Flujo gráfico completo (buffers de entrada a salida)

### 3.1 Datos en CPU

La geometría se define en CPU de dos maneras:

- Arrays estáticos de floats (por ejemplo, cubo y baldosa en la grúa).
- Vectores dinámicos (órbitas, anillos, modelo OBJ de la ISS en sistema solar).

En sistema solar, la esfera se toma de esfera.h, con layout de 8 floats por vértice.

```cpp
// Grúa: arrays estáticos
float vertices_baldosa[] = { /* ... */ };
float vertices_cubo[] = { /* ... */ };

// Sistema Solar: esfera precalculada y vectores dinámicos
int numFloats = sizeof(vertices_esfera) / sizeof(float);
numVerticesEsfera = numFloats / 8;
std::vector<float> verticesOrbita;
```

### 3.2 Subida de datos a GPU

Patrón usado en ambos programas:

1. glGenVertexArrays y glGenBuffers.
2. glBindVertexArray y glBindBuffer(GL_ARRAY_BUFFER, ...).
3. glBufferData para copiar vértices a memoria GPU.
4. glVertexAttribPointer para describir formato del atributo.
5. glEnableVertexAttribArray(0).

No se usa EBO en esta entrega: todo se dibuja con glDrawArrays.

```cpp
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeBytes, dataPtr, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
glEnableVertexAttribArray(0);
```

### 3.3 Entrada al vertex shader

Ambos vertex shaders usan el mismo contrato de entrada:

- location 0: vec3 aPos

El shader aplica transformación MVP:

gl_Position = projection * view * modelo * vec4(aPos, 1.0)

Interpretación:

- modelo: coloca y orienta el objeto en el mundo.
- view: transforma mundo a cámara.
- projection: proyecta a clip space para rasterización.

```glsl
layout (location = 0) in vec3 aPos;
uniform mat4 modelo;
uniform mat4 view;
uniform mat4 projection;

void main() {
   gl_Position = projection * view * modelo * vec4(aPos, 1.0);
}
```

### 3.4 Rasterización y fragment shader

El hardware rasteriza triángulos/líneas y genera fragmentos.

Ambos fragment shaders son de color sólido:

- uniform vec3 colorObjeto
- salida FragColor = vec4(colorObjeto, 1.0)

No hay iluminación ni texturizado en esta versión, lo cual simplifica mucho la explicación y la estabilidad de la entrega.

```glsl
out vec4 FragColor;
uniform vec3 colorObjeto;

void main() {
   FragColor = vec4(colorObjeto, 1.0);
}
```

### 3.5 Buffers de salida

Se utiliza el framebuffer por defecto de la ventana GLFW:

- Color buffer
- Depth buffer (activo)

En cada frame se limpia con glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT).

Después se presenta con doble buffer:

- glfwSwapBuffers muestra el back buffer.
- glfwPollEvents procesa entradas.

No hay FBO custom, postprocesado ni multipass en esta fase.

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// ... draw calls ...
glfwSwapBuffers(window);
glfwPollEvents();
```

---

## 4. Sistema Solar: explicación completa del código

Archivo principal: mainSistemaSolar.cpp

### 4.1 Estructura de datos Objeto

Cada cuerpo celeste se modela con una estructura Objeto que contiene:

- distancia orbital al padre,
- ángulo y velocidad de traslación,
- ángulo y velocidad de rotación propia,
- escala,
- color,
- puntero al padre (jerarquía),
- VAO/VBO y número de vértices,
- modelMatrix por frame,
- método getPosicionGlobal para extraer posición desde la matriz.

Justificación:

Esta estructura encapsula todo lo necesario para simulación + render en una sola unidad lógica. Permite iterar sobre un vector de punteros y tratar planetas, luna e ISS de forma uniforme.

```cpp
struct Objeto {
   float distancia;
   float angulo_translacion;
   float velocidad_translacion;
   float angulo_rotacion;
   float velocidad_rotacion;
   float escalado;
   glm::vec3 color;
   Objeto* padre;
   unsigned int VAO, VBO;
   int numVertices;
   glm::mat4 modelMatrix;
   glm::vec3 getPosicionGlobal() { return glm::vec3(modelMatrix[3]); }
};
```

### 4.2 Inicialización OpenGL y callbacks

- openGlInit:
  - activa depth test,
  - activa culling de caras traseras,
  - define clear color.

- framebuffer_size_callback:
   - actualiza glViewport al redimensionar ventana y guarda las nuevas dimensiones para recalcular el aspect ratio de la proyección; tras cada resize la matriz `projection` usa el aspect actualizado (ver mainSistemaSolar.cpp:15-17, 88-93, 327-329).

- key_callback:
  - gestiona cámaras con teclas 0..5 y ESC.
  - la tecla 5 activa/cicla el modo telescopio (cada pulsación avanza al siguiente objetivo).

- processInput (llamada cada frame):
  - lee flechas del teclado (←↑↓→) de forma continua y acumula rotaciones en `rotationX`/`rotationY`.

Justificación:

Separar callbacks del bucle principal mantiene el flujo limpio y permite explicar interacción y render por separado en la defensa. La entrada continua de flechas se procesa aparte con `processInput` porque GLFW_PRESS en key_callback solo detecta el evento puntual, no la pulsación sostenida.

```cpp
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (action == GLFW_PRESS) {
      if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
      if (key == GLFW_KEY_0) camaraActual = CAM_SOL;
      if (key == GLFW_KEY_1) camaraActual = CAM_MARTE;
      if (key == GLFW_KEY_2) camaraActual = CAM_TIERRA_DESDE_LUNA;
      if (key == GLFW_KEY_3) camaraActual = CAM_ISS_DESDE_TIERRA;
      if (key == GLFW_KEY_4) camaraActual = CAM_SATURNO;
      if (key == GLFW_KEY_5) { // Telescopio: activa o cicla objetivo
         if (camaraActual == CAM_TELESCOPIO) telescopioIdx++;
         else camaraActual = CAM_TELESCOPIO;
      }
   }
}

void processInput(GLFWwindow* window) {
   float speed = 0.003f;
   if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    rotationX -= speed;
   if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  rotationX += speed;
   if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  rotationY -= speed;
   if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rotationY += speed;
}
```

### 4.3 Preparación de geometría

Funciones clave:

- prepararModelos
- prepararAnillo
- cargarOBJ

Detalle:

1. Esfera:
   - VBO cargado con vertices_esfera.
   - stride de 8 floats.
   - se toma solo posición con offset de 5 floats (porque antes hay 3 normales + 2 UV).

2. Órbita:
   - círculo unitario procedural (NUM_SEGMENTOS_ORBITA).
   - se dibuja con GL_LINE_LOOP.

3. Anillos de Saturno:
   - 4 anillos, cada uno con VAO/VBO propio.
   - malla en tira triangulada GL_TRIANGLE_STRIP.
   - radios interno/externo y color por anillo.

4. ISS desde OBJ:
   - parser simple que lee vértices y caras.
   - soporta caras tri y quad (quad se triangula en dos triángulos).
   - sube resultado a VAO/VBO.

Justificación:

- Reutilizar la misma esfera para planetas reduce coste y complejidad.
- Órbitas como línea separan visualización de trayectoria de geometría del planeta.
- Anillos con triangle strip son eficientes para bandas planas.
- Parser OBJ manual evita dependencias externas extra para este hito.

```cpp
// Esfera (position en location 0 con offset 5 floats)
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_esfera), vertices_esfera, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

// Órbita
glDrawArrays(GL_LINE_LOOP, 0, NUM_SEGMENTOS_ORBITA);

// Anillos
glDrawArrays(GL_TRIANGLE_STRIP, 0, numVerticesAnillo[r]);

// ISS desde OBJ
cargarOBJ("iss.obj", issVAO, issVBO, numVerticesISS);
```

### 4.4 Actualización temporal y cinemática orbital

Función clave: actualizarObjeto(Objeto& obj, float dt)

Por frame:

1. angulo_translacion += velocidad_translacion * dt
2. angulo_rotacion += velocidad_rotacion * dt
3. normaliza ángulos al superar 360
4. construye modelMatrix:
   - identidad
   - si tiene padre: translate a posición global del padre
   - rotate por ángulo orbital en Y
   - translate por radio orbital (distancia)

Justificación física:

- dt desacopla la simulación de FPS, manteniendo velocidad aparente estable.
- jerarquía padre-hijo implementa órbitas relativas (luna/ISS respecto a tierra).
- separar traslación orbital y rotación axial permite controlar ambos fenómenos de forma independiente.

```cpp
obj.angulo_translacion += obj.velocidad_translacion * dt;
obj.angulo_rotacion += obj.velocidad_rotacion * dt;

glm::mat4 modelo = glm::mat4(1.0f);
if (obj.padre != nullptr) {
   modelo = glm::translate(modelo, obj.padre->getPosicionGlobal());
}
modelo = glm::rotate(modelo, glm::radians(obj.angulo_translacion), glm::vec3(0.0f, 1.0f, 0.0f));
modelo = glm::translate(modelo, glm::vec3(obj.distancia, 0.0f, 0.0f));
obj.modelMatrix = modelo;
```

### 4.5 Render de objetos y órbitas

Función clave: dibujarObjeto

- Si el objeto tiene padre:
  - dibuja órbita en gris con orbitaVAO y GL_LINE_LOOP.
  - centra órbita en el padre y la escala por distancia.

- Después dibuja el cuerpo:
  - colorObjeto por uniform,
  - modeloRender = modelMatrix,
  - aplica rotación propia,
  - aplica escala,
  - glDrawArrays(GL_TRIANGLES, 0, numVertices).

Función clave: dibujarAnillosSaturno

- parte de la modelMatrix de Saturno,
- aplica inclinación axial de 27 grados,
- escala por tamaño de Saturno,
- desactiva temporalmente culling para ver ambos lados del disco,
- dibuja cada anillo con su color.

```cpp
// Órbita
glm::mat4 modeloOrbita = glm::mat4(1.0f);
modeloOrbita = glm::translate(modeloOrbita, obj.padre->getPosicionGlobal());
modeloOrbita = glm::scale(modeloOrbita, glm::vec3(obj.distancia));

// Cuerpo
glm::mat4 modeloRender = obj.modelMatrix;
modeloRender = glm::rotate(modeloRender, glm::radians(obj.angulo_rotacion), glm::vec3(0.0f, 1.0f, 0.0f));
modeloRender = glm::scale(modeloRender, glm::vec3(obj.escalado));

// Anillos de Saturno
glm::mat4 m = saturno.modelMatrix;
m = glm::rotate(m, glm::radians(27.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

### 4.6 Cámaras del sistema solar

Modo de cámara seleccionado por enum CameraMode:

- CAM_SOL (tecla 0): vista general del sistema.
- CAM_MARTE (tecla 1): seguimiento de Marte.
- CAM_TIERRA_DESDE_LUNA (tecla 2): vista desde la Luna enfocando la Tierra.
- CAM_ISS_DESDE_TIERRA (tecla 3): vista desde la Tierra enfocando la ISS.
- CAM_SATURNO (tecla 4): vista lateral elevada para apreciar los anillos con su inclinación axial.
- CAM_TELESCOPIO (tecla 5): cámara posicionada en la Tierra que apunta al objetivo actual. Cada pulsación sucesiva de la tecla 5 cicla entre los 10 objetivos disponibles (Sol, Mercurio, Venus, Marte, Júpiter, Saturno, Urano, Neptuno, Luna, ISS) usando un índice `telescopioIdx` y un vector `objetivosTelescopio`.

Rotación manual con flechas:

- En cualquier modo de cámara, las flechas del teclado (←↑↓→) permiten rotar la vista.
- Se acumulan dos ángulos globales (`rotationX`, `rotationY`) incrementados en `processInput()` cada frame.
- Después de calcular `view` con `glm::lookAt`, se aplican rotaciones adicionales sobre los ejes X e Y de la cámara.

En el loop, según el modo:

- se define eye y center,
- se calcula view con glm::lookAt,
- se aplican rotaciones manuales sobre view,
- projection con glm::perspective (aspect ratio recalculado tras resize).

Justificación:

Las cámaras temáticas facilitan demostrar jerarquía espacial y relación entre cuerpos, que es parte central de la práctica. El modo telescopio permite explorar todo el sistema desde un punto fijo. La rotación manual con flechas añade interactividad sin depender de un modo de cámara concreto.

```cpp
switch (camaraActual) {
case CAM_SOL:
   eye = glm::vec3(0.0f, 100.0f, 120.0f);
   center = ptrSol->getPosicionGlobal();
   break;
case CAM_TELESCOPIO:
   telescopioIdx = telescopioIdx % (int)objetivosTelescopio.size();
   eye = ptrTierra->getPosicionGlobal();
   center = objetivosTelescopio[telescopioIdx]->getPosicionGlobal();
   break;
}
view = glm::lookAt(eye, center, up);
// Rotación manual con flechas
view = glm::rotate(view, rotationX, glm::vec3(1.0f, 0.0f, 0.0f));
view = glm::rotate(view, rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
float aspect = (SCR_HEIGHT != 0) ? (float)SCR_WIDTH / (float)SCR_HEIGHT : 1.0f;
projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
```

### 4.7 Bucle principal

Secuencia de cada frame:

1. calcular deltaTime,
2. procesar entrada de flechas (processInput),
3. limpiar color+depth,
4. glUseProgram,
5. actualizar todos los objetos,
6. calcular view/projection según cámara + rotación manual,
7. enviar uniforms view/projection,
8. dibujar objetos,
9. dibujar anillos de Saturno,
10. swap y poll events.

Al terminar:

- glDelete* de VAO/VBO,
- glfwTerminate.

```cpp
while (!glfwWindowShouldClose(window)) {
   float currentFrame = glfwGetTime();
   deltaTime = currentFrame - lastFrame;
   lastFrame = currentFrame;
   processInput(window);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glUseProgram(shaderProgram);
   for (auto obj : objetos) actualizarObjeto(*obj, deltaTime);
   for (auto obj : objetos) dibujarObjeto(*obj, shaderProgram);
   dibujarAnillosSaturno(saturno, shaderProgram);
   glfwSwapBuffers(window);
   glfwPollEvents();
}
```

---

## 5. Grúa: explicación completa del código

Archivo principal: mainGrua.cpp

### 5.1 Geometría base

- vertices_baldosa: 2 triángulos (suelo modular por instancia).
- vertices_cubo: 36 vértices (6 caras).
- vertices_borde_rueda: 8 vértices (contorno cuadrado de las caras frontal y trasera de cada rueda, dibujado con GL_LINE_LOOP).

Buffers:

- sueloVAO/sueloVBO
- cuboVAO/cuboVBO
- bordeVAO/bordeVBO (contorno de las ruedas)

Funciones:

- prepararSuelo
- prepararCubo
- prepararBorde

Todas usan layout location 0 con posición vec3.

Justificación:

Con una sola malla de cubo, escalada por matriz, se construyen todas las piezas de la grúa y las 4 ruedas de forma jerárquica y eficiente. El borde de las ruedas permite visualizar la rodadura.

```cpp
void prepararCubo() {
   glGenVertexArrays(1, &cuboVAO);
   glGenBuffers(1, &cuboVBO);
   glBindVertexArray(cuboVAO);
   glBindBuffer(GL_ARRAY_BUFFER, cuboVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_cubo), vertices_cubo, GL_STATIC_DRAW);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);
}
```

### 5.2 Estructuras mecánicas

Modelo jerárquico por structs:

- Base:
  - posición, orientación, velocidad,
  - parámetros de aceleración, frenado, fricción, límite velocidad, giro,
  - anguloRodadura (ángulo acumulado de giro de las ruedas por rodadura),
  - anguloVolante (ángulo de dirección visual de las ruedas delanteras),
  - modelMatrix.

- Cabina:
  - giro sobre Y,
  - offset relativo a base,
  - modelMatrix.

- Articulacion:
  - elevación sobre eje X,
  - offset relativo a cabina,
  - modelMatrix.

- Brazo:
  - extensión variable,
  - offset relativo,
  - modelMatrix.

- GruaCamion: agrupa las cuatro piezas.

Justificación:

La descomposición por piezas replica el sistema mecánico real y simplifica aplicar transformaciones acumuladas padre-hijo.

```cpp
struct GruaCamion {
   Base base;
   Cabina cabina;
   Articulacion articulacion;
   Brazo brazo;
};
```

### 5.3 Dibujo del suelo y de piezas

Función dibujarSuelo:

- recorre una cuadrícula de baldosas con dos bucles x-z,
- para cada baldosa:
  - calcula modelo por traslación,
  - alterna color gris claro/oscuro tipo ajedrez,
  - dibuja 6 vértices.

Función dibujarPieza:

- recibe matriz jerárquica + escala + color,
- compone modelo final con glm::scale,
- envía uniforms,
- dibuja el cubo base.

Función dibujarRueda:

- recibe matriz de la base, posición de la rueda, ángulo de rodadura, ángulo de dirección y color,
- aplica transformaciones: traslación a posición, giro de dirección (Y), orientación lateral (90° en Y), rodadura (Z) y escala (1.2 en XY, 0.35 en Z),
- dibuja el cubo reutilizando cuboVAO,
- después dibuja el contorno cuadrado (bordeVAO) en negro con GL_LINE_LOOP para visualizar la rodadura.

Función dibujarGrua:

- llama 4 veces a dibujarPieza con modelMatrix de cada pieza,
- cada llamada usa escala distinta para forma final,
- llama 4 veces a dibujarRueda para las ruedas en las esquinas del chasis: las delanteras reciben el ángulo de dirección (anguloVolante), las traseras reciben 0.

```cpp
void dibujarGrua(GruaCamion& grua, GLuint shader) {
   dibujarPieza(grua.base.modelMatrix, glm::vec3(2.5f, 1.0f, 5.0f), glm::vec3(0.9f, 0.8f, 0.1f), shader);
   dibujarPieza(grua.cabina.modelMatrix, glm::vec3(1.5f), glm::vec3(0.3f), shader);
   dibujarPieza(grua.articulacion.modelMatrix, glm::vec3(0.5f, 2.0f, 0.5f), glm::vec3(0.9f, 0.5f, 0.1f), shader);
   dibujarPieza(grua.brazo.modelMatrix, glm::vec3(0.3f, grua.brazo.extension, 0.3f), glm::vec3(0.7f), shader);
   // 4 ruedas: delanteras con dirección, traseras solo rodadura
   dibujarRueda(grua.base.modelMatrix, glm::vec3(-1.3f, -0.5f, 1.8f), grua.base.anguloRodadura, grua.base.anguloVolante, colorRueda, shader);
   dibujarRueda(grua.base.modelMatrix, glm::vec3( 1.3f, -0.5f, 1.8f), grua.base.anguloRodadura, grua.base.anguloVolante, colorRueda, shader);
   dibujarRueda(grua.base.modelMatrix, glm::vec3(-1.3f, -0.5f,-1.8f), grua.base.anguloRodadura, 0.0f, colorRueda, shader);
   dibujarRueda(grua.base.modelMatrix, glm::vec3( 1.3f, -0.5f,-1.8f), grua.base.anguloRodadura, 0.0f, colorRueda, shader);
}
```

### 5.4 Física y control del vehículo grúa

Función clave: actualizarFisicas(GruaCamion& grua, float dt, GLFWwindow* window)

Controles y comportamiento:

1. W (acelerar): incrementa velocidad con aceleracion * dt.
2. X (frenar / atrás): reduce velocidad con frenado * dt.
3. Sin W/X: aplica fricción hacia cero de forma progresiva.
4. Clamp de velocidad:
   - máximo hacia delante = velMaxima,
   - máximo hacia atrás = -velMaxima/2.
5. A/D (giro): solo actúa sobre la orientación del vehículo si |velocidad| > umbral; invierte sentido de giro si marcha atrás.
6. Rodadura de las ruedas: anguloRodadura se incrementa proporcionalmente a velocidad/radio (radio visual = 0.6). Convierte desplazamiento lineal a rotación angular.
7. Dirección visual de las ruedas delanteras: anguloVolante interpola suavemente hacia ±25° cuando se pulsa A/D (independientemente de si el vehículo se mueve), y vuelve a 0° al soltar.
8. Avance real en mundo:
   - usa orientación en radianes,
   - x += sin(yaw) * v * dt,
   - z += cos(yaw) * v * dt.
9. Colisión con límites de suelo:
   - clamp en rango seguro,
   - fuerza velocidad a cero al tocar borde.
10. Controles de mecanismo:
   - Q/E: giro de cabina,
   - R/F: elevación articulación,
   - T/G: extensión/retracción brazo.
11. Colisión dura articulación-base:
   - la pieza naranja (articulación) se modela como un segmento con radio de seguridad,
   - se proyecta al espacio del chasis amarillo y se testea contra su AABB,
   - si toca, se detiene el movimiento (sin rebote) tanto al subir/bajar como al extender/recoger.
12. Clamp de extensión del brazo: [2.0, 8.0].

Justificación física:

- Modelo cinemático simple, estable y defendible.
- No pretende física rígida real (sin masas ni fuerzas completas), pero sí un control progresivo creíble y desacoplado de FPS.

```cpp
// Tracción y frenado equivalentes
if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) grua.base.velocidadActual += grua.base.aceleracion * dt;
else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) grua.base.velocidadActual -= grua.base.frenado * dt;

// Fricción equivalente
if (grua.base.velocidadActual > 0.0f) grua.base.velocidadActual -= grua.base.friccion * dt;

// Avance con vector forward
float radianes = glm::radians(grua.base.orientacion);
grua.base.posicion.x += sin(radianes) * grua.base.velocidadActual * dt;
grua.base.posicion.z += cos(radianes) * grua.base.velocidadActual * dt;

// Rodadura de las ruedas (proporcional a velocidad / radio)
grua.base.anguloRodadura += (grua.base.velocidadActual * dt / 0.6f) * (180.0f / 3.14159f);

// Dirección visual de las ruedas delanteras (interpola hacia ±25° o 0°)
if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) objetivo = 25.0f;
if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) objetivo = -25.0f;
```

### 5.5 Transformaciones jerárquicas

Función clave: actualizarMatricesGrua

Orden de propagación:

1. Base:
   - translate a posición global,
   - elevación de 1.1 unidades para que la grúa se apoye sobre las ruedas (las ruedas, a -0.5 de la base, quedan con su centro a y=0.6 y su borde inferior en y=0, tocando el suelo),
   - rotación por orientación.

2. Cabina:
   - hereda base.modelMatrix,
   - translate por offset de enganche,
   - rotate sobre Y (torreta).

3. Articulación:
   - hereda cabina.modelMatrix,
   - translate offset,
   - rotate en X (eleva/baja),
   - translate para situar pivote visual.

4. Brazo:
   - hereda articulación.modelMatrix,
   - translate offset,
   - translate adicional dependiente de extensión.

Justificación:

Este encadenamiento garantiza que cualquier movimiento del padre arrastra correctamente al hijo, que es el núcleo de la simulación jerárquica.

```cpp
glm::mat4 modeloBase = glm::translate(glm::mat4(1.0f), grua.base.posicion);
modeloBase = glm::rotate(modeloBase, glm::radians(grua.base.orientacion), glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 modeloCabina = glm::translate(modeloBase, grua.cabina.offset);
modeloCabina = glm::rotate(modeloCabina, glm::radians(grua.cabina.anguloGiroY), glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 modeloArtic = glm::translate(modeloCabina, grua.articulacion.offset);
modeloArtic = glm::rotate(modeloArtic, glm::radians(grua.articulacion.anguloElevacionZ), glm::vec3(1.0f, 0.0f, 0.0f));
```

### 5.6 Cámaras de la grúa

Enum CameraMode:

- CAM_TERCERA_PERSONA
- CAM_PRIMERA_PERSONA
- CAM_CENITAL

Selección por teclado en key_callback (1,2,3).

Cálculo en loop:

- Tercera persona: detrás y encima de la base según orientación.
- Primera persona: posición de cabina, mirada hacia delante según rotación total base+cabina.
- Cenital: vista externa elevada para ver escenario completo.

Nota de resize: se añadió `framebuffer_size_callback` para actualizar viewport y dimensiones de ventana; la proyección usa el aspect ratio recalculado en cada frame tras un resize (ver mainGrua.cpp:15-17, 344-351, 461-467, 541-543).

```cpp
if (camaraActual == CAM_TERCERA_PERSONA) {
   glm::vec3 forward(sin(rad), 0.0f, cos(rad));
   eye = miGrua.base.posicion - forward * 10.0f + glm::vec3(0.0f, 5.0f, 0.0f);
   center = miGrua.base.posicion + glm::vec3(0.0f, 2.0f, 0.0f);
} else if (camaraActual == CAM_CENITAL) {
   eye = glm::vec3(0.0f, 45.0f, 25.0f);
   center = glm::vec3(0.0f, 0.0f, 0.0f);
}
```

### 5.7 Bucle principal de la grúa

Secuencia por frame:

1. deltaTime,
2. clear color+depth,
3. glUseProgram,
4. actualizarFisicas,
5. actualizarMatricesGrua,
6. calcular view/projection según cámara,
7. enviar uniforms view/projection,
8. dibujar suelo,
9. dibujar grúa,
10. swap y poll events.

Liberación final de buffers y glfwTerminate.

```cpp
while (!glfwWindowShouldClose(window)) {
   deltaTime = glfwGetTime() - lastFrame;
   lastFrame = glfwGetTime();
   actualizarFisicas(miGrua, deltaTime, window);
   actualizarMatricesGrua(miGrua);
   dibujarSuelo(shaderProgram);
   dibujarGrua(miGrua, shaderProgram);
   glfwSwapBuffers(window);
   glfwPollEvents();
}
```

---

## 6. Utilidad compartida de shaders: lecturaShader_0_9.h

Funciones:

- textFileRead:
  - abre fichero shader,
  - lee contenido a memoria dinámica,
  - retorna char* con código fuente.

- printShaderInfoLog / printProgramInfoLog:
  - muestran logs de compilación/link si existen errores.

- setShaders:
  - crea vertex y fragment shader,
  - carga fuentes,
  - compila,
  - crea programa,
  - adjunta shaders,
  - linka programa,
  - devuelve identificador de programa.

Justificación:

Centralizar esta lógica evita duplicación en ambos ejecutables y simplifica mantenimiento.

```cpp
GLuint setShaders(const char *nVertx, const char *nFrag) {
   vertexShader = glCreateShader(GL_VERTEX_SHADER);
   fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(vertexShader, 1, &codigovs, NULL);
   glShaderSource(fragmentShader, 1, &codigofs, NULL);
   glCompileShader(vertexShader);
   glCompileShader(fragmentShader);
   progShader = glCreateProgram();
   glAttachShader(progShader, vertexShader);
   glAttachShader(progShader, fragmentShader);
   glLinkProgram(progShader);
   return progShader;
}
```

---

## 7. Tabla función por función: entradas, salidas, efectos y física

En esta sección, “fuerza” se interpreta en sentido de modelo numérico simplificado. En el código no hay integración de Newton completa con masa/segunda ley explícita, pero sí hay términos equivalentes (aceleración impuesta, frenado, fricción y restricciones) que producen el movimiento observado.

### 7.1 Sistema Solar (mainSistemaSolar.cpp)

| Función | Entradas | Salida | Efectos secundarios | Física implementada (y “fuerzas” equivalentes) | Matrices y vectores implicados |
|---|---|---|---|---|---|
| openGlInit | Ninguna | void | Cambia estado global OpenGL (depth/cull/clear) | No implementa física; prepara el “medio” de simulación visual | No aplica |
| framebuffer_size_callback | window, width, height | void | Actualiza viewport | No física; ajuste geométrico de salida | No aplica |
| key_callback | window, key, scancode, action, mods | void | Cambia modo de cámara y puede cerrar ventana | No física directa; cambia observación del sistema | Vectores de cámara eye/center se recalculan luego en main |
| prepararAnillo | Ninguna | void | Crea VAO/VBO de 4 anillos y rellena numVerticesAnillo | No dinámica física; construcción geométrica estática de discos | Vértices generados con cos(theta), sin(theta) |
| cargarOBJ | ruta, referencias VAO/VBO/numVerts | void | Carga modelo ISS y crea buffers GPU | No física; sólo carga de malla | Vectores de posiciones XYZ leídos del OBJ |
| prepararModelos | Ninguna | void | Crea buffers de esfera, órbita, anillos e ISS | No física temporal; deja listo el soporte geométrico | Layout de vértice esfera (stride 8, offset posición 5) |
| actualizarObjeto | Objeto& obj, float dt | void | Modifica angulo_translacion, angulo_rotacion y modelMatrix | Cinemática orbital y axial. “Fuerza” equivalente: velocidad angular impuesta constante. No hay gravedad newtoniana explícita; la órbita es prescrita por ángulo y radio. | modelo = I; translate(posPadre); rotate(Y, angulo_translacion); translate(distancia,0,0). Rotación propia se aplica después en dibujado. |
| dibujarAnillosSaturno | Objeto& saturno, shader | void | Envía uniforms, draw de 4 anillos, cambia temporalmente cull face | No añade nueva dinámica; representa plano anular inclinado (cinemática visual del cuerpo) | m = saturno.modelMatrix; rotate(Z, 27 grados); scale(escalado) |
| dibujarObjeto | Objeto& obj, shader | void | Dibuja órbita y cuerpo; actualiza uniforms modelo/color | Renderiza estado físico calculado. Para órbita: trayectoria circular prescrita (equivalente a movimiento centrípeto ideal). | modeloOrbita = translate(posPadre) * scale(distancia); modeloRender = obj.modelMatrix * rotate(Y, angulo_rotacion) * scale(escalado) |
| main | Ninguna | int | Control total del ciclo: init, loop, cleanup | Integra la evolución temporal frame a frame usando dt. “Fuerzas” indirectas: las velocidades angulares impuestas en cada objeto gobiernan el movimiento. | view = lookAt(eye, center, up); projection = perspective(...); envío de uniforms MVP |

### 7.2 Grúa (mainGrua.cpp)

| Función | Entradas | Salida | Efectos secundarios | Física implementada (y fuerzas equivalentes) | Matrices y vectores implicados |
|---|---|---|---|---|---|
| prepararSuelo | Ninguna | void | Crea VAO/VBO del tile de suelo | No física; geometría base del escenario | Vértices estáticos de baldosa |
| prepararCubo | Ninguna | void | Crea VAO/VBO del cubo base | No física; geometría reutilizable para piezas | Vértices estáticos de cubo |
| dibujarSuelo | shader | void | Emite draw calls de la cuadrícula y uniforms | No física dinámica; referencia espacial para cinemática del vehículo | modelo = translate(x,0,z) por baldosa |
| dibujarPieza | matrizJerarquica, escala, color, shader | void | Envía modelo/color y dibuja cubo | Representa estado mecánico ya calculado; no integra ecuaciones | modeloFinal = scale(matrizJerarquica, escala) |
| dibujarGrua | GruaCamion&, shader | void | Dibuja base, cabina, articulación y brazo | No añade física; visualiza configuración articulada actual | Usa modelMatrix de cada eslabón + escala por pieza |
| actualizarFisicas | GruaCamion&, dt, window | void | Modifica velocidad, orientación, posición y grados de articulación/extensión | Sí, aquí está la física principal: (1) fuerza tractora equivalente con W: +aceleracion*dt, (2) fuerza de frenado con X: -frenado*dt, (3) fricción equivalente (disipación) cuando no hay entrada, (4) límite de velocidad (restricción), (5) giro proporcional al estado cinemático y sentido de marcha, (6) colisión con borde como restricción dura con anulación de velocidad, (7) accionamiento de actuadores Q/E/R/F/T/G para cinemática de cabina/brazo. | Avance con vector forward = (sin(yaw), 0, cos(yaw)); posicion += forward * velocidad * dt. yaw en grados convertido con radians(). |
| actualizarMatricesGrua | GruaCamion& | void | Recalcula modelMatrix de base, cabina, articulación y brazo | Cinemática directa jerárquica de eslabones (cadena articulada). “Fuerzas” de actuadores ya convertidas en variables angulares/lineales en actualizarFisicas. | Base: T(pos)*T(0,0.5,0)*R_y(orientacion). Cabina: Base*T(offset)*R_y(anguloCabina). Articulación: Cabina*T(offset)*R_x(elevacion)*T(0,1,0). Brazo: Articulación*T(offset)*T(0, extension/2+1,0). |
| key_callback | window, key, scancode, action, mods | void | Cambia cámara o cierra | No física; cambia punto de observación | Afecta qué eye/center se usará en el loop |
| openGlInit | Ninguna | void | Configura depth/cull/clear | No física; estado global gráfico | No aplica |
| main | Ninguna | int | Orquesta init, loop, actualización física y cleanup | Integrador temporal global del modelo cinemático de conducción y articulación con dt por frame | Cálculo de cámara con lookAt y proyección con perspective; envío de view/projection |

### 7.3 Utilidad compartida (lecturaShader_0_9.h)

| Función | Entradas | Salida | Efectos secundarios | Física implementada | Matrices y vectores implicados |
|---|---|---|---|---|---|
| textFileRead | const char* fn | char* | Reserva memoria dinámica y lee fichero | Ninguna | No aplica |
| printShaderInfoLog | GLuint obj | void | Imprime logs de shader | Ninguna | No aplica |
| printProgramInfoLog | GLuint obj | void | Imprime logs de programa | Ninguna | No aplica |
| setShaders | nVertx, nFrag | GLuint | Compila, linka y devuelve program shader | Ninguna | No aplica |

---

## 8. Diagrama por frame (texto) del pipeline exacto de ejecución

### 8.1 Sistema Solar: orden exacto en cada frame

1. currentFrame = glfwGetTime()
2. deltaTime = currentFrame - lastFrame
3. lastFrame = currentFrame
4. glClearColor(...)
5. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
6. glUseProgram(shaderProgram)
7. Para cada objeto en objetos: actualizarObjeto(*obj, deltaTime)
8. Selección de cámara por camaraActual y cálculo de eye/center/up
9. view = glm::lookAt(eye, center, up)
10. projection = glm::perspective(...)
11. glUniformMatrix4fv(view)
12. glUniformMatrix4fv(projection)
13. Para cada objeto en objetos: dibujarObjeto(*obj, shaderProgram)
14. dibujarAnillosSaturno(saturno, shaderProgram)
15. glfwSwapBuffers(window)
16. glfwPollEvents()

Pipeline GPU por draw call (pasos 13 y 14):

1. VAO activo define flujo de atributos (location 0 -> aPos)
2. Vertex shader: gl_Position = projection * view * modelo * vec4(aPos,1)
3. Clipping + rasterización
4. Fragment shader: FragColor = vec4(colorObjeto,1)
5. Depth test + cull face (salvo anillos, cull temporalmente desactivado)
6. Escritura en framebuffer

### 8.2 Grúa: orden exacto en cada frame

1. currentFrame = glfwGetTime()
2. deltaTime = currentFrame - lastFrame
3. lastFrame = currentFrame
4. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
5. glUseProgram(shaderProgram)
6. actualizarFisicas(miGrua, deltaTime, window)
7. actualizarMatricesGrua(miGrua)
8. Según camaraActual, calcular eye/center/up
9. view = glm::lookAt(eye, center, up)
10. projection = glm::perspective(...)
11. glUniformMatrix4fv(view)
12. glUniformMatrix4fv(projection)
13. dibujarSuelo(shaderProgram)
14. dibujarGrua(miGrua, shaderProgram)
15. glfwSwapBuffers(window)
16. glfwPollEvents()

Pipeline GPU por draw call (pasos 13 y 14):

1. Se bindea VAO de suelo o cubo
2. Se envían uniforms modelo y colorObjeto
3. Vertex shader aplica MVP
4. Rasterización de triángulos
5. Fragment shader aplica color plano
6. Depth test/culling y escritura en framebuffer

---

## 9. Resumen para defensa oral

Idea central de ambos programas:

- La geometría está en GPU una vez (VBO/VAO).
- Lo que cambia en tiempo real son matrices y parámetros físicos.
- La simulación se actualiza por deltaTime.
- El shader transforma vértices con MVP y pinta con color uniforme.
- El depth buffer ordena correctamente la visibilidad.

Diferencia conceptual principal:

- Sistema Solar: jerarquía orbital (padre-hijo astronómico).
- Grúa: jerarquía mecánica articulada con conducción en suelo.

Con esto se cumple el objetivo de representar sistemas jerárquicos en OpenGL 3.3 con control interactivo y comportamiento temporal estable.

```text
CPU (estado + matrices) -> Uniforms/VAO -> Vertex Shader (MVP) -> Raster -> Fragment Shader (color) -> Depth/Cull -> Framebuffer -> SwapBuffers
```


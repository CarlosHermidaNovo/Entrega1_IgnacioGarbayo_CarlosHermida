/*
* COMPUTACION GRAFICA 2026
* GRUA. PRIMERA ENTREGA
* AUTORES: Ignacio Garbayo y Carlos Hermida.
*
*/

// INCLUDES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <BibliotecasCurso/lecturaShader_0_9.h>

// Librerias matematicas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// tamaño ventana (mutable para recalcular aspect tras resize)
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

//Variables globales
int shaderProgram;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ==========================================
// CAMARAS
// ==========================================
enum CameraMode { CAM_TERCERA_PERSONA, CAM_PRIMERA_PERSONA, CAM_CENITAL };
CameraMode camaraActual = CAM_TERCERA_PERSONA;


// ==========================================
// GEOMETRiA 
// ==========================================

// Vértices de un plano cuadrado centrado en el origen, que se repetirá para formar el suelo
float vertices_baldosa[] = {
	-0.5f, 0.0f, -0.5f,
	-0.5f, 0.0f,  0.5f,
	 0.5f, 0.0f,  0.5f,
	 0.5f, 0.0f,  0.5f,
	 0.5f, 0.0f, -0.5f,
	 -0.5f, 0.0f, -0.5f
};
unsigned int sueloVAO, sueloVBO;
int numVerticesSuelo = 6;

// Vértices de un cubo unitario centrado en el origen, que se usará para dibujar las piezas de la grúa
float vertices_cubo[] = {
	// Cara Trasera (Z = -0.5)
	 0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,

	// Cara Frontal (Z = 0.5)
	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,

	// Cara Izquierda (X = -0.5)
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,

	// Cara Derecha (X = 0.5)
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,

	 // Cara Inferior (Y = -0.5)
	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f, -0.5f,

	 // Cara Superior (Y = 0.5)
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f,  0.5f
};

unsigned int cuboVAO, cuboVBO;
int numVerticesCubo = 36;

// Vértices para el borde (contorno) de las caras frontal y trasera del cubo-rueda, dibujados con GL_LINE_LOOP
float vertices_borde_rueda[] = {
	// Cara frontal (z = 0.5) — 4 vértices
	-0.5f,-0.5f, 0.5f,
	 0.5f,-0.5f, 0.5f,
	 0.5f, 0.5f, 0.5f,
	-0.5f, 0.5f, 0.5f,
	// Cara trasera (z = -0.5) — 4 vértices
	-0.5f,-0.5f,-0.5f,
	 0.5f,-0.5f,-0.5f,
	 0.5f, 0.5f,-0.5f,
	-0.5f, 0.5f,-0.5f,
};
unsigned int bordeVAO, bordeVBO;

// ==========================================
// ESTRUCTURAS DE LAS PIEZAS
// ==========================================

// Cada pieza de la grúa tiene su propia estructura con los parámetros necesarios para controlar su movimiento, y una matriz de modelo que se actualizará cada frame con su posición y orientación en el mundo.
struct Base {
	glm::vec3 posicion; // Posicion en el mundo (X,Y,Z)
	float orientacion;	// Hacia donde mira el chasis
	float velocidadActual;
	float aceleracion = 10.0f;
	float frenado = 15.0f;
	float friccion = 4.0f;  // Rozamiento para detenerse solo si no se acelera
	float velMaxima = 25.0f;
	float velGiro = 60.0f;
	float anguloRodadura = 0.0f;  // Ángulo acumulado de giro de las ruedas (rodadura)
	float anguloVolante = 0.0f;   // Ángulo de dirección de las ruedas delanteras
	glm::mat4 modelMatrix;
};

// La cabina rota sobre el chasis, por lo que su posición de enganche es relativa a la base. El ángulo de giro se controla con la variable anguloGiroY, y su matriz de modelo se calcula cada frame aplicando esta rotación sobre la posición de la base.
struct Cabina {
	float anguloGiroY;  // Rotacion de la cabina de la grua sobre si misma
	glm::vec3 offset;	// Posicion de enganche relativa respecto al centro del camion
	glm::mat4 modelMatrix;
};

// La articulación es la bisagra que levanta el brazo. Su posición de enganche es relativa a la cabina, y su ángulo de elevación se controla con la variable anguloElevacionZ. Su matriz de modelo se calcula cada frame aplicando esta rotación sobre la posición de la cabina.
struct Articulacion {
	float anguloElevacionZ; // Angulo para subir o bajar el brazo de la grua
	glm::vec3 offset;		// Posicion de enganche relativa respecto a la cabina
	glm::mat4 modelMatrix;
};

// El brazo es el tramo final que se estira. Su posición de enganche es relativa a la articulación, y su extensión se controla con la variable extension. Su matriz de modelo se calcula cada frame aplicando esta extensión sobre la posición de la articulación.
struct Brazo {
	float extension;	// Distancia de estiramiento del brazo
	glm::vec3 offset;	// Posicion de enganche relativa respecto a la articulacion
	glm::mat4 modelMatrix;
};

// Se actualizará cada frame calculando las matrices de modelo de cada pieza aplicando las transformaciones jerárquicas correspondientes (la cabina rota sobre la base, la articulación rota sobre la cabina, y el brazo se estira sobre la articulación).
struct GruaCamion {
	Base base;     // Camion
	Cabina cabina; // La plataforma que rota sobre el camion
	Articulacion articulacion; // La bisagra que levanta el brazo
	Brazo brazo;   // El tramo final que se estira
};

GruaCamion miGrua;

// ==========================================
// FUNCIONES DE PREPARACION
// ==========================================

// Prepara los buffers de OpenGL para dibujar el suelo, el cubo y las piezas de la grúa.
void prepararSuelo() {
	glGenVertexArrays(1, &sueloVAO);
	glGenBuffers(1, &sueloVBO);
	glBindVertexArray(sueloVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sueloVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_baldosa), vertices_baldosa, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

// Prepara los buffers de OpenGL para dibujar un cubo unitario, que se usará para representar las piezas de la grúa.
void prepararCubo() {
	glGenVertexArrays(1, &cuboVAO);
	glGenBuffers(1, &cuboVBO);
	glBindVertexArray(cuboVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cuboVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_cubo), vertices_cubo, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

// Prepara los buffers para el contorno (borde) de las caras del cubo-rueda.
void prepararBorde() {
	glGenVertexArrays(1, &bordeVAO);
	glGenBuffers(1, &bordeVBO);
	glBindVertexArray(bordeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, bordeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_borde_rueda), vertices_borde_rueda, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

// ==========================================
// FUNCIONES DE DIBUJO
// ==========================================

// Dibuja el suelo formando una cuadrícula de baldosas, aplicando un color alternado para crear un efecto de tablero de ajedrez.
void dibujarSuelo(GLuint shader) {
	glBindVertexArray(sueloVAO);
	int tamanhoCuadricula = 20;
	float tamanhoBaldosa = 1.0f;

	for (int x = -tamanhoCuadricula; x <= tamanhoCuadricula; x++) {
		for (int z = -tamanhoCuadricula; z <= tamanhoCuadricula; z++) {
			glm::mat4 modelo = glm::mat4(1.0f);
			modelo = glm::translate(modelo, glm::vec3(x * tamanhoBaldosa, 0.0f, z * tamanhoBaldosa));
			glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modelo));

			//Dependiendo de si es par o impar la xuma de x y z la baldosa ser� gris oscuro o gris claro. Buscamos el efecto tablero de ajedrez.
			glm::vec3 colorBaldosa = ((x + z) % 2 == 0) ? glm::vec3(0.3f) : glm::vec3(0.5f); 
			glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(colorBaldosa));

			glDrawArrays(GL_TRIANGLES, 0, numVerticesSuelo);
		}
	}
	glBindVertexArray(0);
}

// Dibuja una pieza de la grúa aplicando la matriz jerárquica correspondiente para posicionarla correctamente en el mundo, y usando un color específico para cada tipo de pieza (base, cabina, articulación o brazo).
void dibujarPieza(glm::mat4 matrizJerarquica, glm::vec3 escala, glm::vec3 color, GLuint shader) {
	glm::mat4 modeloFinal = glm::scale(matrizJerarquica, escala);
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloFinal));
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(color));
	glBindVertexArray(cuboVAO);
	glDrawArrays(GL_TRIANGLES, 0, numVerticesCubo);
}

// Dibuja una rueda (cubo con borde) en la posición indicada relativa a la base, aplicando la rodadura y opcionalmente el giro de dirección.
void dibujarRueda(glm::mat4 matrizBase, glm::vec3 posRueda, float anguloRodadura, float anguloDireccion, glm::vec3 color, GLuint shader) {
	glm::mat4 modelo = matrizBase;
	modelo = glm::translate(modelo, posRueda);
	// Giro de dirección (solo afecta a las ruedas delanteras, las traseras reciben 0)
	modelo = glm::rotate(modelo, glm::radians(anguloDireccion), glm::vec3(0.0f, 1.0f, 0.0f));
	// Orientar el cubo para que quede vertical y perpendicular al eje del vehículo (eje Z pasa a ser el eje X del camión)
	modelo = glm::rotate(modelo, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	// Rodadura: giro sobre su propio eje (Z del cubo tras la rotación)
	modelo = glm::rotate(modelo, glm::radians(anguloRodadura), glm::vec3(0.0f, 0.0f, 1.0f));
	// Escala: 1.2 en XY (lado visible), 0.35 en Z (grosor fino)
	modelo = glm::scale(modelo, glm::vec3(1.2f, 1.2f, 0.35f));

	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modelo));
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(color));
	// Reutilizamos el cubo existente como geometría de la rueda
	glBindVertexArray(cuboVAO);
	glDrawArrays(GL_TRIANGLES, 0, numVerticesCubo);
	glBindVertexArray(0);

	// Dibujar el contorno de las caras frontal y trasera en negro para ver la rodadura
	glm::vec3 colorBorde(0.0f, 0.0f, 0.0f); // Negro
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(colorBorde));
	glLineWidth(3.0f);
	glBindVertexArray(bordeVAO);
	glDrawArrays(GL_LINE_LOOP, 0, 4);  // Cara frontal
	glDrawArrays(GL_LINE_LOOP, 4, 4);  // Cara trasera
	glBindVertexArray(0);
	glLineWidth(1.0f);
}

// Dibuja la grúa completa llamando a dibujarPieza para cada una de sus partes, aplicando las transformaciones jerárquicas correspondientes para posicionarlas correctamente en el mundo.
void dibujarGrua(GruaCamion& grua, GLuint shader) {
	dibujarPieza(grua.base.modelMatrix, glm::vec3(2.5f, 1.0f, 5.0f), glm::vec3(0.15f, 0.15f, 0.15f), shader);
	dibujarPieza(grua.cabina.modelMatrix, glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.9f, 0.7f, 0.1f), shader);
	dibujarPieza(grua.articulacion.modelMatrix, glm::vec3(0.5f, 2.0f, 0.5f), glm::vec3(0.9f, 0.7f, 0.1f), shader);
	dibujarPieza(grua.brazo.modelMatrix, glm::vec3(0.3f, grua.brazo.extension, 0.3f), glm::vec3(0.8f, 0.8f, 0.8f), shader);

	// Dibujar las 4 ruedas (cubos con borde) en las esquinas de la base
	glm::vec3 colorRueda(0.9f, 0.1f, 0.1f); // Rojo llamativo
	// Posiciones de las ruedas en el espacio local de la base (esquinas inferiores del chasis)
	// Delanteras (Z positivo = frente del camión): con ángulo de dirección
	dibujarRueda(grua.base.modelMatrix, glm::vec3(-1.3f, -0.5f, 1.8f), grua.base.anguloRodadura, grua.base.anguloVolante, colorRueda, shader);
	dibujarRueda(grua.base.modelMatrix, glm::vec3( 1.3f, -0.5f, 1.8f), grua.base.anguloRodadura, grua.base.anguloVolante, colorRueda, shader);
	// Traseras (Z negativo = parte trasera): sin ángulo de dirección
	dibujarRueda(grua.base.modelMatrix, glm::vec3(-1.3f, -0.5f, -1.8f), grua.base.anguloRodadura, 0.0f, colorRueda, shader);
	dibujarRueda(grua.base.modelMatrix, glm::vec3( 1.3f, -0.5f, -1.8f), grua.base.anguloRodadura, 0.0f, colorRueda, shader);
}


// ==========================================
// FUNCIONES DE ACTUALIZACION Y FISICA
// ==========================================

// Función de colisión entre el brazo de la grúa y la cabina. Se modela el brazo como un segmento 3D con un radio (para darle volumen), y la cabina como una caja AABB. Se comprueba si el segmento del brazo intersecta la caja de la cabina, lo que indicaría una colisión.
bool brazoColisionaConCabina(const GruaCamion& grua, float anguloElevacionZ, float extension) {
	const auto segmentoIntersecaAABB = [](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& bmin, const glm::vec3& bmax) -> bool {
		float tmin = 0.0f;
		float tmax = 1.0f;
		glm::vec3 d = p1 - p0;

		// Para cada eje (X, Y, Z), comprobamos la intersección del segmento con los planos de la caja
		for (int i = 0; i < 3; ++i) {
			// Si el segmento es paralelo a los planos de este eje, comprobamos si está dentro del rango de la caja en este eje
			if (fabs(d[i]) < 1e-6f) {
				if (p0[i] < bmin[i] || p0[i] > bmax[i]) return false;
			}
			// El segmento no es paralelo, así que calculamos los valores de t para las intersecciones con los planos de la caja
			else {
				float invD = 1.0f / d[i];
				// Calculamos t1 y t2 para las intersecciones con los planos de la caja en este eje
				float t1 = (bmin[i] - p0[i]) * invD;
				float t2 = (bmax[i] - p0[i]) * invD;
				// Aseguramos que t1 sea el menor y t2 el mayor
				if (t1 > t2) {
					float tmp = t1;
					t1 = t2;
					t2 = tmp;
				}
				// Actualizamos tmin y tmax para el rango de intersección del segmento con la caja
				if (t1 > tmin) tmin = t1;
				if (t2 < tmax) tmax = t2;
				if (tmin > tmax) return false;
			}
		}

		return true;
	};

	const float radioArticulacion = 0.6f;

	// Matrices de modelo como en el render (sin escala) para construir segmentos en mundo.
	glm::mat4 baseModel = glm::mat4(1.0f);
	baseModel = glm::translate(baseModel, grua.base.posicion);
	baseModel = glm::translate(baseModel, glm::vec3(0.0f, 1.1f, 0.0f));
	baseModel = glm::rotate(baseModel, glm::radians(grua.base.orientacion), glm::vec3(0.0f, 1.0f, 0.0f));

	// La cabina rota sobre la base, y la articulacion rota sobre la cabina. Para calcular la posición del brazo en el mundo, aplicamos estas transformaciones jerárquicas.
	glm::mat4 cabModel = baseModel;
	cabModel = glm::translate(cabModel, grua.cabina.offset);
	cabModel = glm::rotate(cabModel, glm::radians(grua.cabina.anguloGiroY), glm::vec3(0.0f, 1.0f, 0.0f));

	// El brazo se eleva con un angulo sobre la articulacion, y se estira con su extension. Para calcular la posición del brazo en el mundo, aplicamos estas transformaciones jerárquicas sobre la posición de la cabina.
	glm::mat4 articModel = cabModel;
	articModel = glm::translate(articModel, grua.articulacion.offset);
	articModel = glm::rotate(articModel, glm::radians(anguloElevacionZ), glm::vec3(1.0f, 0.0f, 0.0f));
	articModel = glm::translate(articModel, glm::vec3(0.0f, 1.0f, 0.0f));

	// Segmento de la articulación (naranja) en espacio mundo (recortado en la base para no contar el anclaje propio).
	glm::vec3 p0ArticW = glm::vec3(articModel * glm::vec4(0.0f, -1.0f, 0.0f, 1.0f));
	glm::vec3 p1ArticW = glm::vec3(articModel * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

	// Solo bloqueamos contra la base/chasis (amarillo).
	const glm::vec3 medioBase(1.25f, 0.5f, 2.5f);
	const glm::vec3 bminBaseArtic = -medioBase - glm::vec3(radioArticulacion);
	const glm::vec3 bmaxBaseArtic = medioBase + glm::vec3(radioArticulacion);

	// Para comprobar la colisión, transformamos el segmento del brazo al espacio local de la base, y comprobamos si intersecta la caja AABB de la base. Si hay intersección, significa que el brazo colisiona con la cabina.
	glm::mat4 invBase = glm::inverse(baseModel);
	glm::vec3 p0ArticBase = glm::vec3(invBase * glm::vec4(p0ArticW, 1.0f));
	glm::vec3 p1ArticBase = glm::vec3(invBase * glm::vec4(p1ArticW, 1.0f));

	return segmentoIntersecaAABB(p0ArticBase, p1ArticBase, bminBaseArtic, bmaxBaseArtic);
}

// Actualiza la posición y orientación de cada pieza de la grúa según las entradas del usuario
void actualizarFisicas(GruaCamion& grua, float dt, GLFWwindow* window) {

	// 1. Acelerador y freno (W/X)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) grua.base.velocidadActual += grua.base.aceleracion * dt;
	else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) grua.base.velocidadActual -= grua.base.frenado * dt;
	else {
		// Friccion: detiene la grua poco a poco si no se pulsa nada
		if (grua.base.velocidadActual > 0.0f) {
			grua.base.velocidadActual -= grua.base.friccion * dt;
			if (grua.base.velocidadActual < 0.0f) grua.base.velocidadActual = 0.0f;
		}
		// Si la velocidad es negativa, el freno la reduce hasta detenerse, pero no la invierte
		else if (grua.base.velocidadActual < 0.0f) {
			grua.base.velocidadActual += grua.base.friccion * dt;
			if (grua.base.velocidadActual > 0.0f) grua.base.velocidadActual = 0.0f;
		}
	}

	// Limitar velocidad para que no corra infinitamente
	if (grua.base.velocidadActual > grua.base.velMaxima) grua.base.velocidadActual = grua.base.velMaxima;
	if (grua.base.velocidadActual < -grua.base.velMaxima / 2.0f) grua.base.velocidadActual = -grua.base.velMaxima / 2.0f;

	// 2. Direccion (A/D)
	// Para que sea realista solo se permitira girar el vehiculo si esta en movimiento
	if (fabs(grua.base.velocidadActual) > 0.1f) {
		float direccionGiro = (grua.base.velocidadActual > 0.0f) ? 1.0f : -1.0f;
		// Si va hacia adelante, A gira a la izquierda y D a la derecha. Si va hacia atrás, se invierte el sentido de giro para que siga siendo intuitivo (A gira a la derecha y D a la izquierda).
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) grua.base.orientacion += grua.base.velGiro * dt * direccionGiro;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) grua.base.orientacion -= grua.base.velGiro * dt * direccionGiro;
	}

	// 3. Aplicamos el movimiento a la posicion global.
	float radianes = glm::radians(grua.base.orientacion);
	// Para movernos en la dirección que estamos mirando, calculamos el desplazamiento en X y Z usando funciones trigonométricas. La velocidad actual se multiplica por el deltaTime para que el movimiento sea suave e independiente de la tasa de frames.
	grua.base.posicion.x += sin(radianes) * grua.base.velocidadActual * dt;
	grua.base.posicion.z += cos(radianes) * grua.base.velocidadActual * dt;

	// 4. Limitamos la posicion al area del suelo (cuadricula de 41x41 baldosas, de -20 a +20).
	// Se usa un margen de seguridad para que el vehiculo no sobresalga del borde.
	const float LIMITE_SUELO = 18.5f;
	// Si la grua se sale del limite, se corrige su posicion y se detiene para evitar que siga moviendose fuera del area visible.
	if (grua.base.posicion.x > LIMITE_SUELO)  { grua.base.posicion.x = LIMITE_SUELO;  grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.x < -LIMITE_SUELO) { grua.base.posicion.x = -LIMITE_SUELO; grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.z > LIMITE_SUELO)  { grua.base.posicion.z = LIMITE_SUELO;  grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.z < -LIMITE_SUELO) { grua.base.posicion.z = -LIMITE_SUELO; grua.base.velocidadActual = 0.0f; }

	// 5. Rodadura de las ruedas: el ángulo de giro depende de la velocidad y el radio de la rueda
	const float radioRueda = 0.6f; // Radio visual de la rueda (0.5 * escala 1.2)
	grua.base.anguloRodadura += (grua.base.velocidadActual * dt / radioRueda) * (180.0f / 3.14159f);

	// 6. Ángulo de dirección visual de las ruedas delanteras (máximo ±25 grados)
	// Las ruedas giran visualmente siempre que se pulse A/D, aunque el vehículo esté parado
	const float maxVolante = 25.0f;
	const float velVolante = 120.0f; // Velocidad a la que las ruedas giran hacia la posición objetivo
	float objetivo = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) objetivo = maxVolante;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) objetivo = -maxVolante;
	// Interpolar suavemente hacia el ángulo objetivo
	if (grua.base.anguloVolante < objetivo) {
		grua.base.anguloVolante += velVolante * dt;
		if (grua.base.anguloVolante > objetivo) grua.base.anguloVolante = objetivo;
	} else if (grua.base.anguloVolante > objetivo) {
		grua.base.anguloVolante -= velVolante * dt;
		if (grua.base.anguloVolante < objetivo) grua.base.anguloVolante = objetivo;
	}

	// Controles adicionales de la grua

	// 7. Giro de la cabina (Q/E)
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) grua.cabina.anguloGiroY += 45.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) grua.cabina.anguloGiroY -= 45.0f * dt;

	// 8. Elevacion del brazo (R/F) y extension del brazo (T/G)
	float deltaAnguloBrazo = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) deltaAnguloBrazo += 30.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) deltaAnguloBrazo -= 30.0f * dt;
	if (deltaAnguloBrazo != 0.0f) {
		float signo = (deltaAnguloBrazo > 0.0f) ? 1.0f : -1.0f;
		float restante = fabs(deltaAnguloBrazo);
		const float pasoAngulo = 0.5f;
		while (restante > 0.0f) {
			float avance = (restante > pasoAngulo) ? pasoAngulo : restante;
			float anguloCandidato = grua.articulacion.anguloElevacionZ + signo * avance;
			bool colisionCandidata = brazoColisionaConCabina(grua, anguloCandidato, grua.brazo.extension);
			if (colisionCandidata) break;
			grua.articulacion.anguloElevacionZ = anguloCandidato;
			restante -= avance;
		}
	}

	// Para la extension del brazo, se hace un proceso similar al de la elevacion, pero en lugar de limitar el angulo, se limita la extension a un rango determinado (por ejemplo, entre 2.0f y 8.0f).

	// Cuanto queremos extender o recoger el brazo en este frame (en unidades de longitud)
	float deltaExtension = 0.0f;
	// T extiende el brazo: suma velocidad * tiempo transcurrido para que sea independiente de los FPS
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) deltaExtension += 2.0f * dt;
	// G recoge el brazo: resta velocidad * tiempo transcurrido
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) deltaExtension -= 2.0f * dt;

	// Solo procesamos si hay movimiento real (evita trabajo innecesario)
	if (deltaExtension != 0.0f) {
		// Extraemos el signo del delta para saber si extendemos (+1) o recogemos (-1)
		float signo = (deltaExtension > 0.0f) ? 1.0f : -1.0f;
		// Cantidad de longitud que aun queda por aplicar en este frame
		float restante = fabs(deltaExtension);
		// Maxima longitud que avanzamos por iteracion: subdivide el movimiento para deteccion de colision
		const float pasoExtension = 0.05f;
		while (restante > 0.0f) {
			// Este paso es el minimo entre lo que queda y el paso maximo permitido
			float avance = (restante > pasoExtension) ? pasoExtension : restante;
			// Extension candidata: la extension actual mas el incremento en la direccion correcta
			float extensionCandidata = grua.brazo.extension + signo * avance;
			// el brazo no puede ser mas corto que 2.0 unidades
			if (extensionCandidata < 2.0f) extensionCandidata = 2.0f;
			// el brazo no puede ser mas largo que 8.0 unidades
			if (extensionCandidata > 8.0f) extensionCandidata = 8.0f;
			// Comprobamos si con esta extension el brazo chocaria con la cabina
			bool colisionCandidata = brazoColisionaConCabina(grua, grua.articulacion.anguloElevacionZ, extensionCandidata);
			// Si hay colision, paramos el bucle: no aplicamos mas extension
			if (colisionCandidata) break;
			// Si no hay movimiento, salimos para evitar bucle infinito
			if (fabs(extensionCandidata - grua.brazo.extension) < 1e-6f) break;
			// Guardamos la nueva extension valida
			grua.brazo.extension = extensionCandidata;
			// Descontamos lo que acabamos de avanzar de lo que quedaba por aplicar
			restante -= avance;
		}
	}
}

// Funcion para actualizar elementos de la grua
void actualizarMatricesGrua(GruaCamion& grua) {
	// La cabina rota sobre la base
	glm::mat4 modeloBase = glm::mat4(1.0f);
	modeloBase = glm::translate(modeloBase, grua.base.posicion);
	modeloBase = glm::translate(modeloBase, glm::vec3(0.0f, 1.1f, 0.0f));
	modeloBase = glm::rotate(modeloBase, glm::radians(grua.base.orientacion), glm::vec3(0.0f, 1.0f, 0.0f));
	grua.base.modelMatrix = modeloBase;

	// La articulacion rota sobre la cabina
	glm::mat4 modeloCabina = grua.base.modelMatrix;
	modeloCabina = glm::translate(modeloCabina, grua.cabina.offset);
	modeloCabina = glm::rotate(modeloCabina, glm::radians(grua.cabina.anguloGiroY), glm::vec3(0.0f, 1.0f, 0.0f));
	grua.cabina.modelMatrix = modeloCabina;

	// El brazo se eleva con un angulo sobre la articulacion
	glm::mat4 modeloArtic = grua.cabina.modelMatrix;
	modeloArtic = glm::translate(modeloArtic, grua.articulacion.offset);
	modeloArtic = glm::rotate(modeloArtic, glm::radians(grua.articulacion.anguloElevacionZ), glm::vec3(1.0f, 0.0f, 0.0f));
	modeloArtic = glm::translate(modeloArtic, glm::vec3(0.0f, 1.0f, 0.0f));
	grua.articulacion.modelMatrix = modeloArtic;

	// El brazo se estira sobre la articulacion
	glm::mat4 modeloBrazo = grua.articulacion.modelMatrix;
	modeloBrazo = glm::translate(modeloBrazo, grua.brazo.offset);
	modeloBrazo = glm::translate(modeloBrazo, glm::vec3(0.0f, grua.brazo.extension / 2.0f + 1.0f, 0.0f));
	grua.brazo.modelMatrix = modeloBrazo;
}

// ==========================================
// EVENTOS Y MAIN
// ==========================================

// Función de callback para manejar las entradas del teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// Cambiar camara pulsando 1,2 y 3
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
		if (key == GLFW_KEY_1) camaraActual = CAM_TERCERA_PERSONA;
		if (key == GLFW_KEY_2) camaraActual = CAM_PRIMERA_PERSONA;
		if (key == GLFW_KEY_3) camaraActual = CAM_CENITAL;
	}
}

// Ajusta el viewport y guarda el tamaño actual para recalcular el aspect ratio
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
}

// Función para inicializar las configuraciones básicas de OpenGL
void openGlInit() {
	glClearDepth(1.0f);
	glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

//========================================
// FUNCION PRINCIPAL
//========================================
int main() {

	// Inicialización de GLFW y creación de la ventana
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Grua", nullptr, nullptr);
	if (window == nullptr) {
		printf("Error al crear la ventana GLFW");
		glfwTerminate();
		return -1;
	}

	// Hacer el contexto de OpenGL actual y configurar el callback de teclado
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Fallo en la inicializacion de GLAD");
		return -1;
	}

	// Configuraciones básicas de OpenGL
	openGlInit();
	shaderProgram = setShaders("Grua.vert", "Grua.frag");

	prepararSuelo();
	prepararCubo();
	prepararBorde();

	// Inicialización de la grúa con sus parámetros iniciales. La base empieza en el centro del mundo, mirando hacia adelante, sin velocidad. La cabina empieza sin rotación, y la articulación empieza con un ángulo de elevación de 30 grados para que el brazo esté levantado inicialmente. El brazo empieza con una extensión de 3 unidades.
	miGrua.base.posicion = glm::vec3(0.0f, 0.0f, 0.0f);
	miGrua.base.orientacion = 0.0f;
	miGrua.base.velocidadActual = 0.0f;
	miGrua.cabina.anguloGiroY = 0.0f;
	miGrua.cabina.offset = glm::vec3(0.0f, 1.0f, -1.0f);
	miGrua.articulacion.anguloElevacionZ = 30.0f;
	miGrua.articulacion.offset = glm::vec3(0.0f, 0.75f, 0.0f);
	miGrua.brazo.extension = 3.0f;
	miGrua.brazo.offset = glm::vec3(0.0f, 0.0f, 0.0f);

	// Variables para controlar el tiempo entre frames, lo que permite que el movimiento sea suave e independiente de la tasa de frames.
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderProgram);

		actualizarFisicas(miGrua, deltaTime, window);
		actualizarMatricesGrua(miGrua);

		glm::vec3 eye(0.0f), center(0.0f), up(0.0f, 1.0f, 0.0f); // EYE ojo, donde estas posicionado; CENTER centro, a qué objeto se esta mirando; UP controlar la inclinación de la camara. al poner (0.0f, 1.0f, 0.0f) se mantiene la camara recta y nivelada con el usuario (cielo arriba)

		if (camaraActual == CAM_TERCERA_PERSONA) {
			float rad = glm::radians(miGrua.base.orientacion);
			glm::vec3 forward(sin(rad), 0.0f, cos(rad));
			// Pongo el ojo de la camara detras de la base y un poco mas arriba.
			eye = miGrua.base.posicion - forward * 10.0f + glm::vec3(0.0f, 5.0f, 0.0f);
			// Que apunte hacia el centro de la base y un poco mas arriba
			center = miGrua.base.posicion + glm::vec3(0.0f, 2.0f, 0.0f);
		}
		else if (camaraActual == CAM_PRIMERA_PERSONA) {
			// Para la primera persona tomo la posición de la cabina y la subo un poco
			eye = glm::vec3(miGrua.cabina.modelMatrix[3]) + glm::vec3(0.0f, 1.0f, 0.0f);
			float rotacionTotal = miGrua.base.orientacion + miGrua.cabina.anguloGiroY;
			float rad = glm::radians(rotacionTotal);
			glm::vec3 forwardCabina(sin(rad), 0.0f, cos(rad));
			// Que apunte hacia delante de la cabina
			center = eye + forwardCabina;
		}

		// Camara exterior con inclinación (~30 grados desde la vertical) para ver todo el escenario
		else if (camaraActual == CAM_CENITAL) {
			eye = glm::vec3(0.0f, 45.0f, 25.0f);
			center = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		// La función lookAt construye la matriz de vista a partir de la posición del ojo, el punto al que se mira y el vector up
		glm::mat4 view = glm::lookAt(eye, center, up);
		// La función perspective construye la matriz de proyección para una cámara con perspectiva, a partir del campo de visión (45 grados), la relación de aspecto (ancho/alto de la ventana), y los planos de recorte cercano (0.1f) y lejano (1000.0f).
		float aspect = (SCR_HEIGHT != 0) ? (float)SCR_WIDTH / (float)SCR_HEIGHT : 1.0f;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

		// Enviamos las matrices de vista y proyección al shader para que se apliquen a los vértices de los objetos al dibujarlos.
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		dibujarSuelo(shaderProgram);
		dibujarGrua(miGrua, shaderProgram);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Limpieza de recursos
	glDeleteVertexArrays(1, &sueloVAO);
	glDeleteBuffers(1, &sueloVBO);
	glDeleteVertexArrays(1, &cuboVAO);
	glDeleteBuffers(1, &cuboVBO);
	glDeleteVertexArrays(1, &bordeVAO);
	glDeleteBuffers(1, &bordeVBO);
	glfwTerminate();

	return 0;
}
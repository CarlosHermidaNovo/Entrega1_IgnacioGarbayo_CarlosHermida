/*
* COMPUTACION GRAFICA 2026
* GRUA. PRIMERA ENTREGA
* AUTORES: Ignacio Garbayo y Carlos Hermida.
*/

//INCLUDES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <BibliotecasCurso/lecturaShader_0_9.h>

//Librer�as matem�ticas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>

// tama�o ventana
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//Variables globales
int shaderProgram;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ==========================================
// C�MARAS
// ==========================================
enum CameraMode { CAM_TERCERA_PERSONA, CAM_PRIMERA_PERSONA, CAM_CENITAL };
CameraMode camaraActual = CAM_TERCERA_PERSONA;


// ==========================================
// GEOMETR�A 
// ==========================================
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

// ==========================================
// ESTRUCTURAS DE LAS PIEZAS
// ==========================================
struct Base {
	glm::vec3 posicion; //Posici�n en el mundo (X,Y,Z)
	float orientacion;	//Hacia donde mira el chasis
	float velocidadActual;
	float aceleracion = 10.0f;
	float frenado = 15.0f;
	float friccion = 4.0f; //Rozamiento para detenerse solo si no se acelera
	float velMaxima = 25.0f;
	float velGiro = 60.0f;
	glm::mat4 modelMatrix;
};

struct Cabina {
	float anguloGiroY; //Rotaci�n de la cabina de la grua sobre si misma
	glm::vec3 offset;	//Posici�n de enganche relativa respecto al centro del camion
	glm::mat4 modelMatrix;
};

struct Articulacion {
	float anguloElevacionZ; //Angulo para subir o bajar el brazo de la gr�a
	glm::vec3 offset;		//Posici�n de enganche relativa respecto a la cabina
	glm::mat4 modelMatrix;
};

struct Brazo {
	float extension;	//Disstancia de estiramiento del brazo
	glm::vec3 offset;	//Posici�n de enganche relativa respecto a la articulaci�n
	glm::mat4 modelMatrix;
};

struct GruaCamion {
	Base base; //Camion
	Cabina cabina; //La plataforma que rota sobre el camion
	Articulacion articulacion; //La bisagra que levanta el brazo
	Brazo brazo; //El tramo final que se estira
};

GruaCamion miGrua;

// ==========================================
// FUNCIONES DE PREPARACI�N
// ==========================================
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

// ==========================================
// FUNCIONES DE DIBUJO
// ==========================================
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


//Funciones que podr�an cambiarse.
void dibujarPieza(glm::mat4 matrizJerarquica, glm::vec3 escala, glm::vec3 color, GLuint shader) {
	glm::mat4 modeloFinal = glm::scale(matrizJerarquica, escala);
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloFinal));
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(color));
	glBindVertexArray(cuboVAO);
	glDrawArrays(GL_TRIANGLES, 0, numVerticesCubo);
}

void dibujarGrua(GruaCamion& grua, GLuint shader) {
	dibujarPieza(grua.base.modelMatrix, glm::vec3(2.5f, 1.0f, 5.0f), glm::vec3(0.9f, 0.8f, 0.1f), shader);
	dibujarPieza(grua.cabina.modelMatrix, glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.3f, 0.3f, 0.3f), shader);
	dibujarPieza(grua.articulacion.modelMatrix, glm::vec3(0.5f, 2.0f, 0.5f), glm::vec3(0.9f, 0.5f, 0.1f), shader);
	dibujarPieza(grua.brazo.modelMatrix, glm::vec3(0.3f, grua.brazo.extension, 0.3f), glm::vec3(0.7f, 0.7f, 0.7f), shader);
}

// ==========================================
// FUNCIONES DE ACTUALIZACI�N Y F�SICA
// ==========================================
void actualizarFisicas(GruaCamion& grua, float dt, GLFWwindow* window) {

	//1. Acelerador y freno (W/X)
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) grua.base.velocidadActual += grua.base.aceleracion * dt;
	else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) grua.base.velocidadActual -= grua.base.frenado * dt;
	else {
		//Fricci�n: detiene la grua poco a poco si no se pulsa nada
		if (grua.base.velocidadActual > 0.0f) {
			grua.base.velocidadActual -= grua.base.friccion * dt;
			if (grua.base.velocidadActual < 0.0f) grua.base.velocidadActual = 0.0f;
		}
		else if (grua.base.velocidadActual < 0.0f) {
			grua.base.velocidadActual += grua.base.friccion * dt;
			if (grua.base.velocidadActual > 0.0f) grua.base.velocidadActual = 0.0f;
		}
	}

	//Limitar velocidad para que no corra infinitamente
	if (grua.base.velocidadActual > grua.base.velMaxima) grua.base.velocidadActual = grua.base.velMaxima;
	if (grua.base.velocidadActual < -grua.base.velMaxima / 2.0f) grua.base.velocidadActual = -grua.base.velMaxima / 2.0f;

	//2. Direcci�n (A/D)
	//Para que sea realista solo se permitir� girar el veh�culo si eset� en movimiento
	if (fabs(grua.base.velocidadActual) > 0.1f) {
		float direccionGiro = (grua.base.velocidadActual > 0.0f) ? 1.0f : -1.0f;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) grua.base.orientacion += grua.base.velGiro * dt * direccionGiro;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) grua.base.orientacion -= grua.base.velGiro * dt * direccionGiro;
	}

	//3. Aplicamos el movimiento a la posici�n global.
	float radianes = glm::radians(grua.base.orientacion);
	grua.base.posicion.x += sin(radianes) * grua.base.velocidadActual * dt;
	grua.base.posicion.z += cos(radianes) * grua.base.velocidadActual * dt;

	//4. Limitamos la posici�n al �rea del suelo (cuadr�cula de 41x41 baldosas, de -20 a +20).
	//Se usa un margen de seguridad para que el veh�culo no sobresalga del borde.
	const float LIMITE_SUELO = 18.5f;
	if (grua.base.posicion.x > LIMITE_SUELO)  { grua.base.posicion.x = LIMITE_SUELO;  grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.x < -LIMITE_SUELO) { grua.base.posicion.x = -LIMITE_SUELO; grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.z > LIMITE_SUELO)  { grua.base.posicion.z = LIMITE_SUELO;  grua.base.velocidadActual = 0.0f; }
	if (grua.base.posicion.z < -LIMITE_SUELO) { grua.base.posicion.z = -LIMITE_SUELO; grua.base.velocidadActual = 0.0f; }

	// Controles adicionales de la gr�a (Quizas es mejor darle otros valores para que sea m�s intuitivo (Q,E giro de cabina / R,F angulo de elevaci�n / T,G extensi�n brazo).
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) grua.cabina.anguloGiroY += 45.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) grua.cabina.anguloGiroY -= 45.0f * dt;

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) grua.articulacion.anguloElevacionZ += 30.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) grua.articulacion.anguloElevacionZ -= 30.0f * dt;

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) grua.brazo.extension += 2.0f * dt;
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) grua.brazo.extension -= 2.0f * dt;

	//Limitamos la extenci�n m�xima y m�nima del brazo
	if (grua.brazo.extension < 2.0f) grua.brazo.extension = 2.0f;
	if (grua.brazo.extension > 8.0f) grua.brazo.extension = 8.0f;
}

//Funcion para actualizar elementos de la grua.
void actualizarMatricesGrua(GruaCamion& grua) {
	glm::mat4 modeloBase = glm::mat4(1.0f);
	modeloBase = glm::translate(modeloBase, grua.base.posicion);
	modeloBase = glm::translate(modeloBase, glm::vec3(0.0f, 0.5f, 0.0f));
	modeloBase = glm::rotate(modeloBase, glm::radians(grua.base.orientacion), glm::vec3(0.0f, 1.0f, 0.0f));
	grua.base.modelMatrix = modeloBase;

	glm::mat4 modeloCabina = grua.base.modelMatrix;
	modeloCabina = glm::translate(modeloCabina, grua.cabina.offset);
	modeloCabina = glm::rotate(modeloCabina, glm::radians(grua.cabina.anguloGiroY), glm::vec3(0.0f, 1.0f, 0.0f));
	grua.cabina.modelMatrix = modeloCabina;

	glm::mat4 modeloArtic = grua.cabina.modelMatrix;
	modeloArtic = glm::translate(modeloArtic, grua.articulacion.offset);
	modeloArtic = glm::rotate(modeloArtic, glm::radians(grua.articulacion.anguloElevacionZ), glm::vec3(1.0f, 0.0f, 0.0f));
	modeloArtic = glm::translate(modeloArtic, glm::vec3(0.0f, 1.0f, 0.0f));
	grua.articulacion.modelMatrix = modeloArtic;

	glm::mat4 modeloBrazo = grua.articulacion.modelMatrix;
	modeloBrazo = glm::translate(modeloBrazo, grua.brazo.offset);
	modeloBrazo = glm::translate(modeloBrazo, glm::vec3(0.0f, grua.brazo.extension / 2.0f + 1.0f, 0.0f));
	grua.brazo.modelMatrix = modeloBrazo;
}

// ==========================================
// EVENTOS Y MAIN
// ==========================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	//Cambiar camara pulsando 1,2 y 3
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
		if (key == GLFW_KEY_1) camaraActual = CAM_TERCERA_PERSONA;
		if (key == GLFW_KEY_2) camaraActual = CAM_PRIMERA_PERSONA;
		if (key == GLFW_KEY_3) camaraActual = CAM_CENITAL;
	}
}

void openGlInit() {
	glClearDepth(1.0f);
	glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

int main() {
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

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Fallo en la inicializacion de GLAD");
		return -1;
	}

	openGlInit();
	shaderProgram = setShaders("Grua.vert", "Grua.frag");

	prepararSuelo();
	prepararCubo();

	miGrua.base.posicion = glm::vec3(0.0f, 0.0f, 0.0f);
	miGrua.base.orientacion = 0.0f;
	miGrua.base.velocidadActual = 0.0f;
	miGrua.cabina.anguloGiroY = 0.0f;
	miGrua.cabina.offset = glm::vec3(0.0f, 1.0f, -1.0f);
	miGrua.articulacion.anguloElevacionZ = 30.0f;
	miGrua.articulacion.offset = glm::vec3(0.0f, 0.75f, 0.0f);
	miGrua.brazo.extension = 3.0f;
	miGrua.brazo.offset = glm::vec3(0.0f, 0.0f, 0.0f);

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderProgram);

		actualizarFisicas(miGrua, deltaTime, window);
		actualizarMatricesGrua(miGrua);

		glm::vec3 eye(0.0f), center(0.0f), up(0.0f, 1.0f, 0.0f); //EYE ojo, donde estas posicionado; CENTER centro, a qu� objeto se esta mirando; UP controlar la inclinaci�n de la camara. al poner (0.0f,1.0f,0.0f) se mantiene la camara recta y nivelada con el usuario (cielo arriba)

		if (camaraActual == CAM_TERCERA_PERSONA) {
			float rad = glm::radians(miGrua.base.orientacion);
			glm::vec3 forward(sin(rad), 0.0f, cos(rad));
			//Pongo el ojo de la camara detr�s de la base y un poco m�s arriba.
			eye = miGrua.base.posicion - forward * 10.0f + glm::vec3(0.0f, 5.0f, 0.0f);
			//Que apunte hacia el centro de la base y un poco m�s arriba
			center = miGrua.base.posicion + glm::vec3(0.0f, 2.0f, 0.0f);
		}
		else if (camaraActual == CAM_PRIMERA_PERSONA) {
			//Para la primera persona tomo la posici�n de la cabina y la subo un poco
			eye = glm::vec3(miGrua.cabina.modelMatrix[3]) + glm::vec3(0.0f, 1.0f, 0.0f);
			float rotacionTotal = miGrua.base.orientacion + miGrua.cabina.anguloGiroY;
			float rad = glm::radians(rotacionTotal);
			glm::vec3 forwardCabina(sin(rad), 0.0f, cos(rad));
			//Que apunte hacia delante de la cabina
			center = eye + forwardCabina;
		}

		//C�mara exterior con inclinaci�n (~30 grados desde la vertical) para ver todo el escenario
		else if (camaraActual == CAM_CENITAL) {
			eye = glm::vec3(0.0f, 30.0f, 17.0f);
			center = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		glm::mat4 view = glm::lookAt(eye, center, up);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		dibujarSuelo(shaderProgram);
		dibujarGrua(miGrua, shaderProgram);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &sueloVAO);
	glDeleteBuffers(1, &sueloVBO);
	glDeleteVertexArrays(1, &cuboVAO);
	glDeleteBuffers(1, &cuboVBO);
	glfwTerminate();
	return 0;
}
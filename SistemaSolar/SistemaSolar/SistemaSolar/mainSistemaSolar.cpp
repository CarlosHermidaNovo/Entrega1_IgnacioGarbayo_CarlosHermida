/*
* COMPUTACION GRAFICA 2026
* SISTEMA SOLAR. PRIMERA ENTREGA
* AUTORES: Ignacio Garbayo y Carlos Hermida.
*
*/


// INCLUDES
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <BibliotecasCurso/esfera.h>
#include <BibliotecasCurso/lecturaShader_0_9.h>
//librerias matematicas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

//tamanho ventana
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Variables globales por comodidad
int shaderProgram;
unsigned int VAO, VBO, EBO;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint esferaVAO, esferaVBO;
GLuint orbitaVAO, orbitaVBO;
int numVerticesEsfera = 0;
const int NUM_SEGMENTOS_ORBITA = 100;

//========================================
// ESTRUCTURA DE DATOS
//========================================
struct Objeto {
	float distancia; //Radio de la orbita respecto a su padre
	float angulo_translacion; //Angulo de traslacion
	float velocidad_translacion; //Velocidad de traslacion
	float angulo_rotacion; //Angulo de rotacion
	float velocidad_rotacion; //Velocidad de rotacion
	float escalado; //Escalado del objeto
	glm::vec3 color; //Color del objeto
	Objeto* padre; //Puntero al objeto padre (null si es el sol)

	//Variables para OpenGL
	unsigned int VAO, VBO, EBO; //Buffers de OpenGL
	int numVertices; //Numero de vertices para dibujar

	//matriz para saber donde está exactamente en el espacio
	glm::mat4 modelMatrix;

	//Extrae la posicion (x,y,z) del objeto en este frame
	glm::vec3 getPosicionGlobal() {
		return glm::vec3(modelMatrix[3]);

	}
};

enum CameraMode { CAM_SOL, CAM_MARTE, CAM_TIERRA_DESDE_LUNA, CAM_ISS_DESDE_TIERRA };
CameraMode camaraActual = CAM_SOL;

Objeto* ptrSol;
Objeto* ptrTierra;
Objeto* ptrLuna;
Objeto* ptrMarte;
Objeto* ptrISS;


// Funci�n para inicializar OpenGL
void openGlInit()
{
	glClearDepth(1.0f);                      // Valor inicial del z-buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    // Color de limpieza del buffer
	glEnable(GL_DEPTH_TEST);                // Activar z-buffer
	glEnable(GL_CULL_FACE);                 // Ocultar caras traseras
	glCullFace(GL_BACK);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	// (Opcional, pero recomendado si usaste variables globales de ancho y alto)
	// SCR_WIDTH = width;
	// SCR_HEIGHT = height;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
		if (key== GLFW_KEY_0) camaraActual =CAM_SOL; //CAMARA PARA EL SOL
		if (key == GLFW_KEY_1) camaraActual =CAM_MARTE; //CAMARA PARA LA MARTE
		if (key == GLFW_KEY_2) camaraActual =CAM_TIERRA_DESDE_LUNA; //CAMARA PARA TIERRA DESDE LUNA
		if (key == GLFW_KEY_3) camaraActual = CAM_ISS_DESDE_TIERRA; //CAMARA ISS DESDE TIERRA
	}
}




void prepararModelos() {
	// ---- ESFERA ----
	int numFloats = sizeof(vertices_esfera) / sizeof(float); // Cada vertice tiene 8 floats: 3 para la posicion, 3 para la normal y 2 para las coordenadas de textura
	numVerticesEsfera = numFloats / 8; // Dividimos el total de floats por 8 para obtener el numero de vertices

	glGenVertexArrays(1, &esferaVAO);// Generamos un VAO para la esfera
	glGenBuffers(1, &esferaVBO);// Generamos un VBO para la esfera
	glBindVertexArray(esferaVAO);// Bind del VAO para configurar los atributos de la esfera
	glBindBuffer(GL_ARRAY_BUFFER, esferaVBO);// Bind del VBO para cargar los datos de la esfera
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_esfera), vertices_esfera, GL_STATIC_DRAW); // Cargamos los datos de la esfera en el VBO

	// Solo configuramos la Posición (ignoramos normales y texturas)
	// El offset es 5 floats porque primero van 3 normales y 2 texturas
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// ---- ÓRBITA ----
	std::vector<float> verticesOrbita;
	for (int i = 0; i <= NUM_SEGMENTOS_ORBITA; ++i) {
		float theta = 2.0f * 3.1415926f * float(i) / float(NUM_SEGMENTOS_ORBITA);
		verticesOrbita.push_back(cos(theta));
		verticesOrbita.push_back(0.0f);
		verticesOrbita.push_back(sin(theta));
	}

	glGenVertexArrays(1, &orbitaVAO); // Generamos un VAO para la orbita
	glGenBuffers(1, &orbitaVBO); // Generamos un VBO para la orbita
	glBindVertexArray(orbitaVAO); 
	glBindBuffer(GL_ARRAY_BUFFER, orbitaVBO);
	glBufferData(GL_ARRAY_BUFFER, verticesOrbita.size() * sizeof(float), verticesOrbita.data(), GL_STATIC_DRAW);

	// Coincide con el layout = 0 del Vertex Shader
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}



void actualizarObjeto(Objeto& obj, float dt) {
	obj.angulo_translacion += obj.velocidad_translacion * dt;
	if (obj.angulo_translacion > 360.0f) obj.angulo_translacion -= 360.0f;

	obj.angulo_rotacion += obj.velocidad_rotacion * dt;
	if (obj.angulo_rotacion > 360.0f) obj.angulo_rotacion -= 360.0f;

	glm::mat4 modelo = glm::mat4(1.0f);

	if (obj.padre != nullptr) {
		glm::vec3 posPadre = obj.padre->getPosicionGlobal();
		modelo = glm::translate(modelo, posPadre);
	}

	modelo = glm::rotate(modelo, glm::radians(obj.angulo_translacion), glm::vec3(0.0f, 1.0f, 0.0f));
	modelo = glm::translate(modelo, glm::vec3(obj.distancia, 0.0f, 0.0f));

	obj.modelMatrix = modelo;
}


void dibujarObjeto(Objeto& obj, GLuint shader) {
	// ORBITA
	if (obj.padre != nullptr && obj.distancia > 0.0f) {
		glUniform3f(glGetUniformLocation(shader, "colorObjeto"), 0.3f, 0.3f, 0.3f); //linea gris para la orbita
		glm::mat4 modeloOrbita = glm::mat4(1.0f); 
		modeloOrbita = glm::translate(modeloOrbita, obj.padre->getPosicionGlobal()); //trasladamos el centro de la orbita al centro del padre
		modeloOrbita = glm::scale(modeloOrbita, glm::vec3(obj.distancia)); //escalamos la orbita al radio correcto
	
		glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloOrbita)); //enviamos la matriz de modelo para la orbita
		
		glBindVertexArray(orbitaVAO);
		glDrawArrays(GL_LINE_LOOP, 0, NUM_SEGMENTOS_ORBITA); //dibujamos la orbita como un loop de lineas

	}

	// OBJETO
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(obj.color));

	glm::mat4 modeloRender = obj.modelMatrix; //matriz de modelo del objeto
	modeloRender = glm::rotate(modeloRender, glm::radians(obj.angulo_rotacion), glm::vec3(0.0f, 1.0f, 0.0f)); //aplicamos la rotacion del objeto
	modeloRender = glm::scale(modeloRender, glm::vec3(obj.escalado, obj.escalado, obj.escalado)); //escalamos el objeto para que no sea demasiado grande

	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloRender)); //enviamos la matriz de modelo para el objeto
	glBindVertexArray(esferaVAO);
	glDrawArrays(GL_TRIANGLES, 0, numVerticesEsfera); //dibujamos el objeto como un conjunto de triangulos


}




//========================================
// FUNCION PRINCIPAL
//========================================
int main() {

	// Inicializar GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Crearmos la ventana

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sistema Solar", nullptr, nullptr);
	if (window == nullptr)
	{
		printf("Error al crear la ventana GLFW");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Inicializar GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Fallo en la inicializacion de GLAD");
		return -1;
	}

	// Inicializar OpenGL
	openGlInit();

	
	shaderProgram = setShaders("sistSolar.vert","sistSolar.frag");

	prepararModelos();

	//Creo los objetos: Sistema Solar (Distancia, velocidad de transicion, angulo de transicion, velocidad de rotacion, angulo de rotacion, tamanho, color, padre, VAO, VBO, Vertices)
	Objeto sol = { 0.0f,  0.0f, 0.0f,  0.0f, 10.0f, 5.0f, glm::vec3(1.0f, 1.0f, 0.0f), nullptr, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto mercurio = { 8.0f,  0.0f, 47.0f, 0.0f, 10.0f, 0.4f, glm::vec3(0.6f, 0.6f, 0.6f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto venus = { 12.0f, 0.0f, 35.0f, 0.0f, 15.0f, 0.9f, glm::vec3(0.9f, 0.5f, 0.2f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto tierra = { 18.0f, 0.0f, 29.0f, 0.0f, 30.0f, 1.0f, glm::vec3(0.2f, 0.2f, 1.0f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto marte = { 25.0f, 0.0f, 24.0f, 0.0f, 28.0f, 0.5f, glm::vec3(1.0f, 0.0f, 0.0f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto jupiter = { 40.0f, 0.0f, 13.0f, 0.0f, 50.0f, 3.0f, glm::vec3(0.8f, 0.7f, 0.6f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto saturno = { 55.0f, 0.0f, 9.0f,  0.0f, 45.0f, 2.5f, glm::vec3(0.9f, 0.8f, 0.5f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto urano = { 70.0f, 0.0f, 6.0f,  0.0f, 35.0f, 1.8f, glm::vec3(0.5f, 0.8f, 0.9f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto neptuno = { 85.0f, 0.0f, 5.0f,  0.0f, 33.0f, 1.7f, glm::vec3(0.1f, 0.1f, 0.8f), &sol, esferaVAO, esferaVBO, numVerticesEsfera };

	//Satelites
	Objeto luna = { 2.5f, 0.0f, 100.0f, 0.0f, 50.0f, 0.3f, glm::vec3(0.8f, 0.8f, 0.8f), &tierra, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto iss = { 1.2f, 0.0f, 300.0f, 0.0f, 0.0f,  0.1f, glm::vec3(1.0f, 1.0f, 1.0f), &tierra, esferaVAO, esferaVBO, numVerticesEsfera };

	std::vector<Objeto*> objetos={&sol, &mercurio, &venus, &tierra, &marte, &jupiter, &saturno, &urano, &neptuno, &luna, &iss };

	ptrSol=&sol;
	ptrTierra = &tierra;
	ptrLuna = &luna;
	ptrMarte = &marte;
	ptrISS = &iss;

	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClearColor(0.01f, 0.01f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		for (auto obj : objetos) {
			actualizarObjeto(*obj, deltaTime);
		}

		glm::mat4 view = glm::mat4(1.0f);
		glm::vec3 eye(0.0f), center(0.0f), up(0.0f, 1.0f, 0.0f);

		switch (camaraActual) {
		case CAM_SOL:
			eye = glm::vec3(0.0f, 100.0f, 120.0f);
			center = ptrSol->getPosicionGlobal();
			break;
		case CAM_MARTE:
			eye = ptrMarte->getPosicionGlobal() + glm::vec3(0.0f, 3.0f, 5.0f);
			center = ptrMarte->getPosicionGlobal();
			break;
		case CAM_TIERRA_DESDE_LUNA:
			eye = ptrLuna->getPosicionGlobal();
			center = ptrTierra->getPosicionGlobal();
			break;
		case CAM_ISS_DESDE_TIERRA:
			eye = ptrTierra->getPosicionGlobal();
			center = ptrISS->getPosicionGlobal();
			break;
		}

		view = glm::lookAt(eye, center, up);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		for (auto obj : objetos) {
			dibujarObjeto(*obj, shaderProgram);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &esferaVAO);
	glDeleteBuffers(1, &esferaVBO);
	glDeleteVertexArrays(1, &orbitaVAO);
	glDeleteBuffers(1, &orbitaVBO);

	glfwTerminate();
	return 0;
}
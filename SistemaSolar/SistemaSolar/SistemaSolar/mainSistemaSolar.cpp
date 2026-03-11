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

//librerias matematicas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

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





void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
		if (key=GLFW_KEY_0) camaraActual =; //CAMARA PARA EL SOL
		if (key = GLFW_KEY_1) camaraActual =; //CAMARA PARA LA MARTE
		if (key = GLFW_KEY_2) camaraActual =; //CAMARA PARA TIERRA DESDE LUNA
		if (key = GLFW_KEY_3) camaraActual = ; //CAMARA ISS DESDE TIERRA
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

	glGenVertexArrays(1, &orbitaVAO);
	glGenBuffers(1, &orbitaVBO);
	glBindVertexArray(orbitaVAO);
	glBindBuffer(GL_ARRAY_BUFFER, orbitaVBO);
	glBufferData(GL_ARRAY_BUFFER, verticesOrbita.size() * sizeof(float), verticesOrbita.data(), GL_STATIC_DRAW);

	// Coincide con el layout = 0 del Vertex Shader
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
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

}
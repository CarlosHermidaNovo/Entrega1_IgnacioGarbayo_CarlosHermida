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

// librerias matemáticas
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// tamaño ventana (mutable para actualizar aspect ratio tras resize)
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

// Variables globales por comodidad
int shaderProgram;
unsigned int VAO, VBO, EBO;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint esferaVAO, esferaVBO;
GLuint orbitaVAO, orbitaVBO;
const int NUM_ANILLOS = 4;
GLuint anilloVAO[NUM_ANILLOS], anilloVBO[NUM_ANILLOS]; // Anillos C, B, A, F de Saturno
int numVerticesAnillo[NUM_ANILLOS] = {};
// Radios y colores aproximados a los anillos reales de Saturno
const float ANILLO_R_INT[NUM_ANILLOS] = { 1.1f,  1.45f, 2.0f,  2.55f };
const float ANILLO_R_EXT[NUM_ANILLOS] = { 1.45f, 2.0f,  2.5f,  2.62f };
const float ANILLO_COLOR[NUM_ANILLOS][3] = {
    { 0.55f, 0.50f, 0.38f }, // C: oscuro e interior
    { 0.92f, 0.85f, 0.62f }, // B: el más brillante y ancho
    { 0.80f, 0.73f, 0.52f }, // A: exterior, medio
    { 0.65f, 0.60f, 0.45f }  // F: fino y tenue
};
GLuint issVAO, issVBO;         // Modelo 3D de la ISS
int numVerticesEsfera = 0;
int numVerticesISS = 0;
const int NUM_SEGMENTOS_ORBITA = 100;

//========================================
// ESTRUCTURA DE DATOS
//========================================
struct Objeto {
	float distancia; // Radio de la orbita respecto a su padre
	float angulo_translacion; // Angulo de traslacion
	float velocidad_translacion; // Velocidad de traslacion
	float angulo_rotacion; // Angulo de rotacion
	float velocidad_rotacion; // Velocidad de rotacion
	float escalado; // Escalado del objeto
	glm::vec3 color; // Color del objeto
	Objeto* padre; // Puntero al objeto padre (null si es el sol)

	// Variables para OpenGL
	unsigned int VAO, VBO; // Buffers de OpenGL
	int numVertices; // Numero de vertices para dibujar

	// matriz para saber donde está exactamente en el espacio
	glm::mat4 modelMatrix; // Matriz de modelo que se actualiza cada frame con la posicion y rotacion del objeto

	// Extrae la posicion (x,y,z) del objeto en este frame
	glm::vec3 getPosicionGlobal() {
		return glm::vec3(modelMatrix[3]); // La posicion global del objeto se encuentra en la cuarta columna de la matriz de modelo

	}
};

enum CameraMode { CAM_SOL, CAM_MARTE, CAM_TIERRA_DESDE_LUNA, CAM_ISS_DESDE_TIERRA, CAM_SATURNO };
CameraMode camaraActual = CAM_SOL;

Objeto* ptrSol;
Objeto* ptrTierra;
Objeto* ptrLuna;
Objeto* ptrMarte;
Objeto* ptrISS;
Objeto* ptrSaturno;

// Función para inicializar OpenGL
void openGlInit()
{
	glClearDepth(1.0f);                      // Valor inicial del z-buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);    // Color de limpieza del buffer
	glEnable(GL_DEPTH_TEST);                // Activar z-buffer
	glEnable(GL_CULL_FACE);                 // Ocultar caras traseras
	glCullFace(GL_BACK);
}

// Callback para ajustar el viewport al cambiar el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
}

// Callback para manejar la entrada de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
		if (key== GLFW_KEY_0) camaraActual =CAM_SOL; // CAMARA PARA EL SOL
		if (key == GLFW_KEY_1) camaraActual =CAM_MARTE; // CAMARA PARA LA MARTE
		if (key == GLFW_KEY_2) camaraActual =CAM_TIERRA_DESDE_LUNA; // CAMARA PARA TIERRA DESDE LUNA
		if (key == GLFW_KEY_3) camaraActual = CAM_ISS_DESDE_TIERRA; // CAMARA ISS DESDE TIERRA
		if (key == GLFW_KEY_4) camaraActual = CAM_SATURNO; // CAMARA SATURNO (anillos)
	}
}

// Genera los 4 anillos de Saturno (C, B, A, F) cada uno con su radio y VAO propio.
void prepararAnillo() {
	const int SEG = 100;
	// Cada anillo se genera como un strip de triángulos entre su radio interior y exterior, con 100 segmentos para suavidad. Se asigna un VAO/VBO distinto a cada anillo para poder dibujarlos con diferentes colores y radios.
	for (int r = 0; r < NUM_ANILLOS; r++) {
		std::vector<float> verts;
		for (int i = 0; i <= SEG; i++) {
			float theta = 2.0f * 3.1415926f * float(i) / float(SEG);
			verts.push_back(ANILLO_R_INT[r] * cos(theta)); verts.push_back(0.0f); verts.push_back(ANILLO_R_INT[r] * sin(theta));
			verts.push_back(ANILLO_R_EXT[r] * cos(theta)); verts.push_back(0.0f); verts.push_back(ANILLO_R_EXT[r] * sin(theta));
		}
		// Cada segmento del anillo se dibuja como un strip de triángulos entre el borde interior y exterior, por eso se generan 2 vértices por cada paso angular.
		numVerticesAnillo[r] = (int)verts.size() / 3;
		glGenVertexArrays(1, &anilloVAO[r]);
		glGenBuffers(1, &anilloVBO[r]);
		glBindVertexArray(anilloVAO[r]);
		glBindBuffer(GL_ARRAY_BUFFER, anilloVBO[r]);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}
}

// Carga un archivo .obj y sube sus vértices (solo posición XYZ) a la GPU.
// Soporta caras triangulares y quads, y los formatos v, v/vt, v/vt/vn y v//vn.
void cargarOBJ(const char* ruta, unsigned int& VAO, unsigned int& VBO, int& numVerts) {
	std::vector<float> posiciones;
	std::vector<float> vertices;
	FILE* f = fopen(ruta, "r");
	if (!f) { printf("Error: no se pudo abrir %s\n", ruta); numVerts = 0; return; }
	char linea[512];
	// Leemos el archivo linea a linea
	while (fgets(linea, sizeof(linea), f)) {
		// Si la línea define un vértice (comienza con "v "), extraemos sus coordenadas XYZ y las almacenamos en el vector de posiciones.
		if (linea[0] == 'v' && linea[1] == ' ') {
			float x, y, z;
			sscanf(linea + 2, "%f %f %f", &x, &y, &z);
			posiciones.push_back(x); posiciones.push_back(y); posiciones.push_back(z);
		// Si la línea define una cara (comienza con "f "), extraemos los índices de los vértices que forman la cara.
		} else if (linea[0] == 'f' && linea[1] == ' ') {
			int idx[4], count = 0;
			char* ptr = linea + 2;
			while (*ptr && count < 4) {
				while (*ptr == ' ') ptr++;
				if (*ptr == '\0' || *ptr == '\n') break;
				idx[count++] = atoi(ptr) - 1; // OBJ base-1; atoi se detiene en '/'
				while (*ptr && *ptr != ' ') ptr++;
			}
			// Dependiendo de si la cara es un triángulo o un quad, generamos 1 o 2 triángulos respectivamente.
			if (count >= 3) {
				for (int vi : {idx[0], idx[1], idx[2]}) {
					vertices.push_back(posiciones[3*vi]); vertices.push_back(posiciones[3*vi+1]); vertices.push_back(posiciones[3*vi+2]);
				}
				if (count == 4) { // quad -> segundo triángulo
					for (int vi : {idx[0], idx[2], idx[3]}) {
						vertices.push_back(posiciones[3*vi]); vertices.push_back(posiciones[3*vi+1]); vertices.push_back(posiciones[3*vi+2]);
					}
				}
			}
		}
	}
	fclose(f);
	numVerts = (int)vertices.size() / 3;
	glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

// Prepara los modelos de la esfera (planetas/lunas), las órbitas (lineas circulares) y los anillos de Saturno
void prepararModelos() {
	// ---- ESFERA ----
	int numFloats = sizeof(vertices_esfera) / sizeof(float); // Cada vertice tiene 8 floats: 3 para la posicion, 3 para la normal y 2 para las coordenadas de textura
	numVerticesEsfera = numFloats / 8; // Dividimos el total de floats por 8 para obtener el numero de vertices

	glGenVertexArrays(1, &esferaVAO); // Generamos un VAO para la esfera
	glGenBuffers(1, &esferaVBO); // Generamos un VBO para la esfera
	glBindVertexArray(esferaVAO); // Bind del VAO para configurar los atributos de la esfera
	glBindBuffer(GL_ARRAY_BUFFER, esferaVBO); // Bind del VBO para cargar los datos de la esfera
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

	// ---- ANILLO DE SATURNO ----
	prepararAnillo();

	// ---- ISS (modelo OBJ) ----
	cargarOBJ("iss.obj", issVAO, issVBO, numVerticesISS);
}

// Actualiza los ángulos de traslación y rotación del objeto según el tiempo transcurrido (dt), y recalcula su matriz de modelo aplicando las transformaciones jerárquicas respecto al padre.
void actualizarObjeto(Objeto& obj, float dt) {
	// Avanza el ángulo de traslación (órbita) según velocidad y tiempo transcurrido
	obj.angulo_translacion += obj.velocidad_translacion * dt;
	if (obj.angulo_translacion > 360.0f) obj.angulo_translacion -= 360.0f; // Evita desbordamiento

	// Avanza el ángulo de rotación sobre su propio eje
	obj.angulo_rotacion += obj.velocidad_rotacion * dt;
	if (obj.angulo_rotacion > 360.0f) obj.angulo_rotacion -= 360.0f;

	glm::mat4 modelo = glm::mat4(1.0f); // Matriz identidad como punto de partida

	// Si tiene padre, hereda su posición global (transformación jerárquica)
	if (obj.padre != nullptr) {
		glm::vec3 posPadre = obj.padre->getPosicionGlobal();
		modelo = glm::translate(modelo, posPadre);
	}

	// Rota alrededor del eje Y para posicionarse en la órbita, luego se desplaza a su distancia
	modelo = glm::rotate(modelo, glm::radians(obj.angulo_translacion), glm::vec3(0.0f, 1.0f, 0.0f));
	modelo = glm::translate(modelo, glm::vec3(obj.distancia, 0.0f, 0.0f));

	obj.modelMatrix = modelo; // Guarda la matriz final para usarla al dibujar
}

// Dibuja los 4 anillos de Saturno (C, B, A, F) con inclinación axial de ~27°.
void dibujarAnillosSaturno(Objeto& saturno, GLuint shader) {
	glm::mat4 m = saturno.modelMatrix;
	m = glm::rotate(m, glm::radians(27.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // inclinación axial real de Saturno
	m = glm::scale(m, glm::vec3(saturno.escalado));
	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(m));
	glDisable(GL_CULL_FACE); // Los discos son planos: sin esto la cara trasera se descarta
	for (int r = 0; r < NUM_ANILLOS; r++) {
		glUniform3f(glGetUniformLocation(shader, "colorObjeto"), ANILLO_COLOR[r][0], ANILLO_COLOR[r][1], ANILLO_COLOR[r][2]);
		glBindVertexArray(anilloVAO[r]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, numVerticesAnillo[r]);
	}
	glEnable(GL_CULL_FACE);
}

// Dibuja un objeto: primero su órbita (si tiene padre), luego el objeto en sí con su color y transformaciones
// Se asume que el shader ya está activo y que el VAO del objeto está configurado.
void dibujarObjeto(Objeto& obj, GLuint shader) {
	// ORBITA
	if (obj.padre != nullptr && obj.distancia > 0.0f) {
		glUniform3f(glGetUniformLocation(shader, "colorObjeto"), 0.3f, 0.3f, 0.3f); // linea gris para la orbita
		glm::mat4 modeloOrbita = glm::mat4(1.0f); 
		modeloOrbita = glm::translate(modeloOrbita, obj.padre->getPosicionGlobal()); // trasladamos el centro de la orbita al centro del padre
		modeloOrbita = glm::scale(modeloOrbita, glm::vec3(obj.distancia)); //escalamos la orbita al radio correcto
	
		glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloOrbita)); // enviamos la matriz de modelo para la orbita
		
		glBindVertexArray(orbitaVAO);
		glDrawArrays(GL_LINE_LOOP, 0, NUM_SEGMENTOS_ORBITA); // dibujamos la orbita como un loop de lineas
	}

	// OBJETO
	glUniform3fv(glGetUniformLocation(shader, "colorObjeto"), 1, glm::value_ptr(obj.color));

	glm::mat4 modeloRender = obj.modelMatrix; //matriz de modelo del objeto
	modeloRender = glm::rotate(modeloRender, glm::radians(obj.angulo_rotacion), glm::vec3(0.0f, 1.0f, 0.0f)); //aplicamos la rotacion del objeto
	modeloRender = glm::scale(modeloRender, glm::vec3(obj.escalado, obj.escalado, obj.escalado)); //escalamos el objeto para que no sea demasiado grande

	glUniformMatrix4fv(glGetUniformLocation(shader, "modelo"), 1, GL_FALSE, glm::value_ptr(modeloRender)); //enviamos la matriz de modelo para el objeto
	glBindVertexArray(obj.VAO);
	glDrawArrays(GL_TRIANGLES, 0, obj.numVertices); //dibujamos el objeto como un conjunto de triangulos


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

	// Creamos la ventana
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

	// Creo los objetos: Sistema Solar (Distancia, velocidad de transicion, angulo de transicion, velocidad de rotacion, angulo de rotacion, tamanho, color, padre, VAO, VBO, Vertices)
	Objeto sol     = { 0.0f,  0.0f,   0.0f, 0.0f, 10.0f, 5.0f,  glm::vec3(1.0f, 1.0f, 0.0f),  nullptr, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto mercurio= { 8.0f,  0.0f,  47.0f, 0.0f, 10.0f, 0.35f, glm::vec3(0.6f, 0.6f, 0.6f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto venus   = { 12.0f, 0.0f,  35.0f, 0.0f, 15.0f, 0.87f, glm::vec3(0.9f, 0.5f, 0.2f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto tierra  = { 18.0f, 0.0f,  29.0f, 0.0f, 30.0f, 1.0f,  glm::vec3(0.2f, 0.4f, 1.0f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto marte   = { 25.0f, 0.0f,  24.0f, 0.0f, 28.0f, 0.53f, glm::vec3(1.0f, 0.2f, 0.1f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto jupiter = { 40.0f, 0.0f,  13.0f, 0.0f, 50.0f, 3.5f,  glm::vec3(0.8f, 0.7f, 0.6f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto saturno = { 55.0f, 0.0f,   9.0f, 0.0f, 45.0f, 3.0f,  glm::vec3(0.9f, 0.8f, 0.5f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto urano   = { 70.0f, 0.0f,   6.0f, 0.0f, 35.0f, 1.6f,  glm::vec3(0.5f, 0.8f, 0.9f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto neptuno = { 85.0f, 0.0f,   5.0f, 0.0f, 33.0f, 1.5f,  glm::vec3(0.1f, 0.1f, 0.8f),  &sol,    esferaVAO, esferaVBO, numVerticesEsfera };

	// Satélites
	Objeto luna = { 2.5f, 0.0f, 100.0f, 0.0f, 50.0f, 0.27f, glm::vec3(0.8f, 0.8f, 0.8f), &tierra, esferaVAO, esferaVBO, numVerticesEsfera };
	Objeto iss  = { 1.5f, 0.0f, 300.0f, 0.0f,  0.0f, 0.02f, glm::vec3(0.9f, 0.9f, 1.0f), &tierra, issVAO,    issVBO,    numVerticesISS };
	
	// Vector de punteros a objetos para facilitar la iteracion
	std::vector<Objeto*> objetos = { &sol, &mercurio, &venus, &tierra, &marte, &jupiter, &saturno, &urano, &neptuno, &luna, &iss };

	// Asignamos los punteros globales para las camaras
	ptrSol = &sol;
	ptrTierra = &tierra;
	ptrLuna = &luna;
	ptrMarte = &marte;
	ptrISS = &iss;
	ptrSaturno = &saturno;

	// Bucle principal
	while (!glfwWindowShouldClose(window)) {
		// Calculo del deltaTime para animaciones suaves
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Limpieza de buffers
		glClearColor(0.01f, 0.01f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Activamos el shader
		glUseProgram(shaderProgram);

		// Actualizamos la posicion de cada objeto
		for (auto obj : objetos) {
			actualizarObjeto(*obj, deltaTime);
		}

		// Configuramos la camara segun la seleccion del usuario
		glm::mat4 view = glm::mat4(1.0f);// Matriz de vista que se actualizara segun la camara seleccionada
		glm::vec3 eye(0.0f), center(0.0f), up(0.0f, 1.0f, 0.0f); // El vector up siempre apunta hacia arriba en el eje Y

		// Dependiendo de la camara seleccionada, calculamos el vector eye (posicion de la camara) y center (punto al que mira la camara)
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
		case CAM_SATURNO:
			// Cámara lateral y ligeramente elevada para apreciar la inclinación de los anillos
			eye = ptrSaturno->getPosicionGlobal() + glm::vec3(0.0f, 8.0f, 15.0f);
			center = ptrSaturno->getPosicionGlobal();
			break;
		}

		// Calculamos la matriz de vista usando glm::lookAt con los vectores eye, center y up
		view = glm::lookAt(eye, center, up);
		// Calculamos la matriz de proyeccion usando glm::perspective con un FOV de 45 grados, aspect ratio segun el tamaño de la ventana y planos cercano y lejano adecuados para nuestro sistema solar
		float aspect = (SCR_HEIGHT != 0) ? (float)SCR_WIDTH / (float)SCR_HEIGHT : 1.0f;
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

		// Enviamos las matrices de vista y proyeccion al shader
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view)); // Enviamos la matriz de proyeccion al shader
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection)); // Enviamos la matriz de vista al shader

		// Dibujamos cada objeto
		for (auto obj : objetos) {
			dibujarObjeto(*obj, shaderProgram);
		}
		dibujarAnillosSaturno(saturno, shaderProgram); // Los anillos se dibujan aparte, después del planeta

		// Intercambiamos buffers y procesamos eventos
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Limpieza de recursos
	glDeleteVertexArrays(1, &esferaVAO);
	glDeleteBuffers(1, &esferaVBO);
	glDeleteVertexArrays(1, &orbitaVAO);
	glDeleteBuffers(1, &orbitaVBO);
	glDeleteVertexArrays(NUM_ANILLOS, anilloVAO);
	glDeleteBuffers(NUM_ANILLOS, anilloVBO);
	glDeleteVertexArrays(1, &issVAO);
	glDeleteBuffers(1, &issVBO);

	glfwTerminate();
	return 0;
}
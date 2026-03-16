CXX      = g++
TARGET   = SistemaSolar.exe
CXXFLAGS = -std=c++17 -Wall

INCLUDE_LIB     = ../Include_Lib
INCLUDE_LIB_WIN = ..\Include_Lib
SRC_DIR_WIN     = SistemaSolar\SistemaSolar

SRC      = SistemaSolar/SistemaSolar/mainSistemaSolar.cpp $(INCLUDE_LIB)/glad/glad.c
INCLUDES = -I$(INCLUDE_LIB) -I$(INCLUDE_LIB)/GLFW -I$(INCLUDE_LIB)/BibliotecasCurso -I$(INCLUDE_LIB)/glad

GLFW_DEF  = glfw3.def
GLFW_ALIB = $(INCLUDE_LIB)/GLFW/libglfw3.a

LIBS = $(GLFW_ALIB) -lopengl32 -lgdi32 -luser32 -lshell32

all: $(GLFW_ALIB) $(TARGET)

# Genera la import lib compatible con MinGW desde el .def (solo si no existe)
$(GLFW_ALIB): $(GLFW_DEF)
	@echo Generando import lib de GLFW para MinGW...
	dlltool -d $(GLFW_DEF) -D glfw3.dll -l $(GLFW_ALIB)

$(TARGET): $(SRC)
	@echo Compilando $(TARGET)...
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(INCLUDES) $(LIBS)
	@copy $(SRC_DIR_WIN)\sistSolar.vert . >nul
	@copy $(SRC_DIR_WIN)\sistSolar.frag . >nul
	@copy $(SRC_DIR_WIN)\iss.obj . >nul
	@echo Compilacion exitosa!

run: all
	@echo Ejecutando $(TARGET)...
	@set PATH=%CD%\$(INCLUDE_LIB_WIN)\GLFW;%PATH% && .\$(TARGET)

debug: CXXFLAGS += -g -DDEBUG
debug: $(GLFW_ALIB) $(TARGET)

rebuild: clean all

release: CXXFLAGS += -O2 -DNDEBUG
release: $(GLFW_ALIB) $(TARGET)

clean:
	@echo Limpiando archivos...
	-del /f /q $(TARGET) 2>nul
	-del /f /q sistSolar.vert sistSolar.frag iss.obj 2>nul
	-del /f /q $(INCLUDE_LIB_WIN)\GLFW\libglfw3.a 2>nul

help:
	@echo Comandos disponibles:
	@echo   make         - Compila el proyecto
	@echo   make rebuild - Fuerza recompilacion completa
	@echo   make run     - Compila y ejecuta
	@echo   make debug   - Compila con informacion de debug
	@echo   make release - Compila optimizado para release
	@echo   make clean   - Limpia archivos generados
	@echo   make help    - Muestra esta ayuda

.PHONY: all run debug release rebuild clean help

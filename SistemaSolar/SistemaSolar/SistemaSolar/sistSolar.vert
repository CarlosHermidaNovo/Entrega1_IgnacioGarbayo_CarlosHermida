#version 330 core

//recibe la posicion de los vertices (layout 0)
    layout (location = 0) in vec3 aPos;

    //matrices de transformacion
    uniform mat4 modelo;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        //calcula la posicion final del vertice
        gl_Position = projection * view * modelo * vec4(aPos, 1.0);
    }
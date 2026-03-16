#version 330 core

//Salida: Color final del pixel
out vec4 FragColor;

//Variable que nos llega con el color del planeta u orbita
uniform vec3 colorObjeto;

void main() {
    //Pinta el pixel de ese color de forma solida (1.0 = totalmente opaco)
    FragColor = vec4(colorObjeto, 1.0);
}
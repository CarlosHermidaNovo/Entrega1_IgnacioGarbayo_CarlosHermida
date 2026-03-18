#version 330 core
out vec4 FragColor;
uniform vec3 colorObjeto;

void main() {
    FragColor = vec4(colorObjeto, 1.0);
}
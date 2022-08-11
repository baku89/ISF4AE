#version 410

in vec2 pos;

void main() {
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

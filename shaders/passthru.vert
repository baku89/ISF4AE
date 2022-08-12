#version 120

attribute vec2 pos;

void main() {
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

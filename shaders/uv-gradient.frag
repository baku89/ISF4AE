#version 410

uniform vec2  u_resolution;
uniform float u_time;
uniform vec2  u_mouse;

out vec4 outColor;

void main() {
    vec2 st = gl_FragCoord.xy / u_resolution;
    
    float d = length(u_mouse - st);
    
    vec3 color = vec3(st, step(d, u_time));
    
    outColor = vec4(1.0, color);
}

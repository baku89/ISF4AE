#version 410

uniform vec2  u_resolution;
uniform float u_time;
uniform vec2  u_mouse;

out vec4 FragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;
    
    vec3 gradient = vec3(uv, 0.0);
    float cross = step(min(abs(uv.x - uv.y), abs(1. - uv.x - uv.y)), 0.01);
    
    vec3 color = mix(gradient, vec3(1.0), vec3(cross));
    
    FragColor = vec4(color, 1.0);
}

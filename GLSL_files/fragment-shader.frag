#version 330

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

out vec4 FragColor;

void main(){
	
	vec2 st = gl_FragCoord.xy / u_resolution;
	
	vec3 color = vec3(st, abs(sin(u_time)));
	
	FragColor = vec4(1.0, color);
}

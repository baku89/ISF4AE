#version 120

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main(){
	
	vec2 st = gl_FragCoord.xy / u_resolution;
	
	vec3 color = vec3(st, abs(sin(u_time)));
	
	gl_FragColor = vec4(1.0, color);
}

#version 330

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

out vec4 FragColor;

void main(){
	
	vec2 st = gl_FragCoord.xy / u_resolution;

	float prog = u_time / 10.0;

	float filled = step(prog, st.x);

	vec3 color = vec3(filled);

	FragColor = vec4(1.0, color);
}

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main(){
	
	vec2 st = gl_FragCoord.xy / u_resolution;

	float prog = u_time / 10.0;

	float filled = step(prog, st.x);

	vec3 color = vec3(filled);

	gl_FragColor = vec4(color, 1.0);
}

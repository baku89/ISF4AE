uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main(){
	
	vec2 st = gl_FragCoord.xy / u_resolution;
	
	vec3 color = vec3(fract(st.x * 2.0), 0.5, 1.0);
	
	gl_FragColor = vec4(color, 1.0);
}

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;

void main() {
  vec2 st = gl_FragCoord.xy/ u_resolution.xy;
  st.x *= u_resolution.x/ u_resolution.y;
  
  vec2 mouse = u_mouse / u_resolution.xy;
  mouse.x *= u_resolution.x/ u_resolution.y;

  vec3 color = vec3(st, abs(sin(u_time)));        
	color *= 1.0 - length(st - mouse);
    
  gl_FragColor = vec4(color,1.0);
}
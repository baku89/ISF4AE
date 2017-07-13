#version 120

uniform vec2 u_resolution;
uniform sampler2D videoTexture;

void main(){
	vec2 st = gl_FragCoord.xy / u_resolution;
	st.y = 1.0 - st.y;
	gl_FragColor = texture2D(videoTexture, st).argb;
}

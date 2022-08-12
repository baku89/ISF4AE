#version 120

uniform float multiplier16bit;
uniform vec2 u_resolution;
uniform sampler2D inputTexture;
uniform sampler2D glslCanvasOutputTexture;

void main() {
	vec2 uv = gl_FragCoord.xy / u_resolution;

	float inputAlpha = texture2D(inputTexture, uv).r;
	vec4 glslCanvas = texture2D(glslCanvasOutputTexture, uv);

	gl_FragColor = vec4(inputAlpha * glslCanvas.a, glslCanvas.rgb / multiplier16bit);
}

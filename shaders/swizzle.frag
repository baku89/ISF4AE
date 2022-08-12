#version 410

uniform float multiplier16bit;
uniform vec2 u_resolution;
uniform sampler2D inputTexture;
uniform sampler2D glslCanvasOutputTexture;

out vec4 FragColor;

void main() {
	vec2 uv = gl_FragCoord.xy / u_resolution;

	float inputAlpha = texture(inputTexture, uv).r;
	vec4 glslCanvas = texture(glslCanvasOutputTexture, uv);

	FragColor = vec4(inputAlpha * glslCanvas.a, glslCanvas.rgb / multiplier16bit);
}

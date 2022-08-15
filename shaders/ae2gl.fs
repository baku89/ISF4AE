/*{
    "DESCRIPTION": "Converts AE texture's channnel order and luminance range to fit with OpenGL",
    "CREDIT": "Baku Hashimoto",
    "ISFVSN": "2",
    "INPUTS": [
        {
            "NAME": "inputImage",
            "TYPE": "image"
        },
        {
            "NAME": "multiplier16bit",
            "TYPE": "float"
        }
    ]
    
}*/

void main() {
    vec2 flippedUV = vec2(isf_FragNormCoord.x, 1.0 - isf_FragNormCoord.y);
    vec4 aeColor = IMG_NORM_PIXEL(inputImage, flippedUV);
    gl_FragColor = aeColor.gbar * multiplier16bit;
}

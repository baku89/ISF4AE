/*{
    "DESCRIPTION": "Swizzle and scale a OpenGL texture to fit with AE format",
    "CREDIT": "Baku Hashimoto",
    "ISFVSN": "2",
    "INPUTS": [
        {
            "NAME": "inputImage",
            "TYPE": "image"
        },
        {
            "NAME": "multiplier16bit",
            "TYPE": "float",
            "DEFAULT": 1
        }
    ]
    
}*/

void main() {
    vec2 flippedUV = vec2(isf_FragNormCoord.x, 1.0 - isf_FragNormCoord.y);
    vec4 glColor = IMG_NORM_PIXEL(inputImage, flippedUV);
    gl_FragColor = glColor.argb / multiplier16bit;
}

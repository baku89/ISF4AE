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
            "NAME": "isfImage",
            "TYPE": "image"
        },
        {
            "NAME": "multiplier16bit",
            "TYPE": "float"
        }
    ]
    
}*/

void main() {
    float inputAlpha = IMG_THIS_PIXEL(inputImage).r;
    vec4 isfImage = IMG_THIS_PIXEL(isfImage);
    
    gl_FragColor = vec4(inputAlpha, vec3(1.0)) * isfImage.argb / multiplier16bit;
}

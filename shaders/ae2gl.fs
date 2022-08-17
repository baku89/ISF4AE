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
        },
        {
            "NAME": "origin",
            "TYPE": "point2D"
        }
    ]
    
}*/

void main() {
    vec2 coord = vec2(gl_FragCoord.x, RENDERSIZE.y - gl_FragCoord.y) - origin;
    vec4 aeColor = IMG_PIXEL(inputImage, coord);
    gl_FragColor = aeColor.gbar * multiplier16bit;
}

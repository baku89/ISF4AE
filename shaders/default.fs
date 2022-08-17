/*{
 "DESCRIPTION": "Default shader",
 "CREDIT": "Baku Hashimoto",
 "ISFVSN": "2",
 "INPUTS": [
     {
         "NAME": "inputImage",
         "TYPE": "image"
     }
 ]
}*/
void main() {
    vec2 uv = isf_FragNormCoord.xy;
    
    vec3 gradient = vec3(uv, 0.0);
    float cross = step(min(abs(uv.x - uv.y), abs(1. - uv.x - uv.y)), 0.01);

    vec3 color = mix(gradient, vec3(1.0), vec3(cross));
    float alpha = IMG_THIS_PIXEL(inputImage).a;
    
    gl_FragColor = vec4(color, alpha);
}

/*{
 "DESCRIPTION": "Default shader",
 "CREDIT": "Baku Hashimoto",
 "ISFVSN": "2",
 "INPUTS": [
 ]
}*/
void main() {
    vec2 uv = isf_FragNormCoord.xy;
    
    vec3 gradient = vec3(uv, 0.0);
    float cross = step(min(abs(uv.x - uv.y), abs(1. - uv.x - uv.y)), 0.01);

    vec3 color = mix(gradient, vec3(1.0), vec3(cross));
    
    gl_FragColor = vec4(color, 1.0);
}

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec2 vTexCoord)


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iTexCoord.xy;
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    gl_FragColor = texture(sDiffMap, vTexCoord);
}
#endif // def COMPILEFS

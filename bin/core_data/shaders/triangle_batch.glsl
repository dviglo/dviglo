#dv_include "uniforms.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec4 vColor)


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vColor = iColor;
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    gl_FragColor = vColor;
}
#endif // def COMPILEFS

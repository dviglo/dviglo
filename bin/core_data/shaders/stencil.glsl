#dv_include "uniforms.inc"
#dv_include "transform.inc"


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    gl_FragColor = vec4(1.0);
}
#endif // def COMPILEFS

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "postprocess.inc"

VS_OUT_FS_IN(vec2 vScreenPos)


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    vec3 color = texture(sDiffMap, vScreenPos).rgb;
    gl_FragColor = vec4(ToInverseGamma(color), 1.0);
}
#endif // def COMPILEFS

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "lighting.inc"

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
    vec3 rgb = texture(sDiffMap, vScreenPos).rgb;
    float intensity = GetIntensity(rgb);
    gl_FragColor = vec4(intensity, intensity, intensity, 1.0);
}
#endif // def COMPILEFS

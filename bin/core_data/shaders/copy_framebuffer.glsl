#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"

VS_OUT_FS_IN(vec2 vScreenPos)

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

#elif defined COMPILEFS

void main()
{
    gl_FragColor = texture(sDiffMap, vScreenPos);
}

#endif // defined COMPILEVS

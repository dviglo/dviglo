#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"

#ifdef COMPILEFS
uniform vec2 cBlurOffsets;
#endif

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
    vec2 color = vec2(0.0);

    color += 0.015625 * texture(sDiffMap, vScreenPos + vec2(-3.0) * cBlurOffsets).rg;
    color += 0.09375 * texture(sDiffMap, vScreenPos + vec2(-2.0) * cBlurOffsets).rg;
    color += 0.234375 * texture(sDiffMap, vScreenPos + vec2(-1.0) * cBlurOffsets).rg;
    color += 0.3125 * texture(sDiffMap, vScreenPos).rg;
    color += 0.234375 * texture(sDiffMap, vScreenPos + vec2(1.0) * cBlurOffsets).rg;
    color += 0.09375 * texture(sDiffMap, vScreenPos + vec2(2.0) * cBlurOffsets).rg;
    color += 0.015625 * texture(sDiffMap, vScreenPos + vec2(3.0) * cBlurOffsets).rg;

    gl_FragColor = vec4(color, 0.0, 0.0);
}
#endif // def COMPILEFS

#include "uniforms.inc"
#include "samplers.inc"
#include "transform.inc"
#include "screen_pos.inc"

varying vec2 vScreenPos;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

void PS()
{
    gl_FragColor = texture(sDiffMap, vScreenPos);
}


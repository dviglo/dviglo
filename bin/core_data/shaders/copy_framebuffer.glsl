#include "uniforms.glsl"
#include "samplers.glsl"
#include "transform.glsl"
#include "screen_pos.glsl"

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


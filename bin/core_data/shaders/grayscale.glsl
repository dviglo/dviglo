#include "uniforms.inc"
#include "samplers.inc"
#include "transform.inc"
#include "screen_pos.inc"
#include "lighting.inc"

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
    vec3 rgb = texture(sDiffMap, vScreenPos).rgb;
    float intensity = GetIntensity(rgb);
    gl_FragColor = vec4(intensity, intensity, intensity, 1.0);
}

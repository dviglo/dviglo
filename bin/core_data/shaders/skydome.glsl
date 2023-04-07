#include "uniforms.inc"
#include "samplers.inc"
#include "transform.inc"

varying vec2 vTexCoord;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iTexCoord.xy;
}

void PS()
{
    gl_FragColor = texture(sDiffMap, vTexCoord);
}

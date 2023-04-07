#include "uniforms.inc"
#include "transform.inc"

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
}

void PS()
{
    gl_FragColor = vec4(1.0);
}


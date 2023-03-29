#include "uniforms.glsl"
#include "samplers.glsl"
#include "transform.glsl"
#include "ScreenPos.glsl"
#include "postprocess.glsl"

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
    vec3 color = texture2D(sDiffMap, vScreenPos).rgb;
    gl_FragColor = vec4(ToInverseGamma(color), 1.0);
}

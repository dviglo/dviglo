#include "uniforms.glsl"
#include "samplers.glsl"
#include "transform.glsl"
#include "screen_pos.glsl"

varying vec2 vTexCoord;
varying vec2 vScreenPos;

#ifdef COMPILEPS
uniform float cBloomThreshold;
uniform vec2 cBloomMix;
uniform vec2 cBlurHInvSize;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetQuadTexCoord(gl_Position);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

void PS()
{
    #ifdef BRIGHT
    vec3 rgb = texture(sDiffMap, vScreenPos).rgb;
    gl_FragColor = vec4((rgb - vec3(cBloomThreshold, cBloomThreshold, cBloomThreshold)) / (1.0 - cBloomThreshold), 1.0);
    #endif

    #ifdef BLURH
    vec3 rgb = texture(sDiffMap, vTexCoord + vec2(-2.0, 0.0) * cBlurHInvSize).rgb * 0.1;
    rgb += texture(sDiffMap, vTexCoord + vec2(-1.0, 0.0) * cBlurHInvSize).rgb * 0.25;
    rgb += texture(sDiffMap, vTexCoord + vec2(0.0, 0.0) * cBlurHInvSize).rgb * 0.3;
    rgb += texture(sDiffMap, vTexCoord + vec2(1.0, 0.0) * cBlurHInvSize).rgb * 0.25;
    rgb += texture(sDiffMap, vTexCoord + vec2(2.0, 0.0) * cBlurHInvSize).rgb * 0.1;
    gl_FragColor = vec4(rgb, 1.0);
    #endif

    #ifdef BLURV
    vec3 rgb = texture(sDiffMap, vTexCoord + vec2(0.0, -2.0) * cBlurHInvSize).rgb * 0.1;
    rgb += texture(sDiffMap, vTexCoord + vec2(0.0, -1.0) * cBlurHInvSize).rgb * 0.25;
    rgb += texture(sDiffMap, vTexCoord + vec2(0.0, 0.0) * cBlurHInvSize).rgb * 0.3;
    rgb += texture(sDiffMap, vTexCoord + vec2(0.0, 1.0) * cBlurHInvSize).rgb * 0.25;
    rgb += texture(sDiffMap, vTexCoord + vec2(0.0, 2.0) * cBlurHInvSize).rgb * 0.1;
    gl_FragColor = vec4(rgb, 1.0);
    #endif

    #ifdef COMBINE
    vec3 original = texture(sDiffMap, vScreenPos).rgb * cBloomMix.x;
    vec3 bloom = texture(sNormalMap, vTexCoord).rgb  * cBloomMix.y;
    // Prevent oversaturation
    original *= max(vec3(1.0) - bloom, vec3(0.0));
    gl_FragColor = vec4(original + bloom, 1.0);
    #endif
}


#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "postprocess.inc"

varying vec2 vScreenPos;

#ifdef COMPILEFS
uniform float cTonemapExposureBias;
uniform float cTonemapMaxWhite;
#endif

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
    #ifdef REINHARDEQ3
    vec3 color = ReinhardEq3Tonemap(max(texture(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0));
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef REINHARDEQ4
    vec3 color = ReinhardEq4Tonemap(max(texture(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0), cTonemapMaxWhite);
    gl_FragColor = vec4(color, 1.0);
    #endif

    #ifdef UNCHARTED2
    vec3 color = Uncharted2Tonemap(max(texture(sDiffMap, vScreenPos).rgb * cTonemapExposureBias, 0.0)) /
        Uncharted2Tonemap(vec3(cTonemapMaxWhite, cTonemapMaxWhite, cTonemapMaxWhite));
    gl_FragColor = vec4(color, 1.0);
    #endif
}

#endif // defined COMPILEVS

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "postprocess.inc"

VS_OUT_FS_IN(vec2 vTexCoord)
VS_OUT_FS_IN(vec2 vScreenPos)

#ifdef COMPILEFS
uniform float cAutoExposureAdaptRate;
uniform vec2 cAutoExposureLumRange;
uniform float cAutoExposureMiddleGrey;
uniform vec2 cHDR128InvSize;
uniform vec2 cLum64InvSize;
uniform vec2 cLum16InvSize;
uniform vec2 cLum4InvSize;

float GatherAvgLum(sampler2D texSampler, vec2 texCoord, vec2 texelSize)
{
    float lumAvg = 0.0;
    lumAvg += texture(texSampler, texCoord + vec2(-1.0, -1.0) * texelSize).r;
    lumAvg += texture(texSampler, texCoord + vec2(-1.0, 1.0) * texelSize).r;
    lumAvg += texture(texSampler, texCoord + vec2(1.0, 1.0) * texelSize).r;
    lumAvg += texture(texSampler, texCoord + vec2(1.0, -1.0) * texelSize).r;
    return lumAvg / 4.0;
}
#endif

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetQuadTexCoord(gl_Position);
    vScreenPos = GetScreenPosPreDiv(gl_Position);
}

#elif defined COMPILEFS

void main()
{
    #ifdef LUMINANCE64
    float logLumSum = 0.0;
    logLumSum += log(dot(texture(sDiffMap, vTexCoord + vec2(-1.0, -1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture(sDiffMap, vTexCoord + vec2(-1.0, 1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture(sDiffMap, vTexCoord + vec2(1.0, 1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    logLumSum += log(dot(texture(sDiffMap, vTexCoord + vec2(1.0, -1.0) * cHDR128InvSize).rgb, LumWeights) + 1e-5);
    gl_FragColor.r = logLumSum;
    #endif

    #ifdef LUMINANCE16
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum64InvSize);
    #endif

    #ifdef LUMINANCE4
    gl_FragColor.r = GatherAvgLum(sDiffMap, vTexCoord, cLum16InvSize);
    #endif

    #ifdef LUMINANCE1
    gl_FragColor.r = exp(GatherAvgLum(sDiffMap, vTexCoord, cLum4InvSize) / 16.0);
    #endif

    #ifdef ADAPTLUMINANCE
    float adaptedLum = texture(sDiffMap, vTexCoord).r;
    float lum = clamp(texture(sNormalMap, vTexCoord).r, cAutoExposureLumRange.x, cAutoExposureLumRange.y);
    gl_FragColor.r = adaptedLum + (lum - adaptedLum) * (1.0 - exp(-cDeltaTimePS * cAutoExposureAdaptRate));
    #endif

    #ifdef EXPOSE
    vec3 color = texture(sDiffMap, vScreenPos).rgb;
    float adaptedLum = texture(sNormalMap, vTexCoord).r;
    gl_FragColor = vec4(color * (cAutoExposureMiddleGrey / adaptedLum), 1.0);
    #endif
}

#endif // defined COMPILEVS

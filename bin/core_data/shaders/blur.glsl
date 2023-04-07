#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "postprocess.inc"

varying vec2 vTexCoord;
varying vec2 vScreenPos;

#ifdef COMPILEFS
uniform vec2 cBlurDir;
uniform float cBlurRadius;
uniform float cBlurSigma;
uniform vec2 cBlurHInvSize;
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
    #ifdef BLUR3
        gl_FragColor = GaussianBlur(3, cBlurDir, cBlurHInvSize * cBlurRadius, cBlurSigma, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR5
        gl_FragColor = GaussianBlur(5, cBlurDir, cBlurHInvSize * cBlurRadius, cBlurSigma, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR7
        gl_FragColor = GaussianBlur(7, cBlurDir, cBlurHInvSize * cBlurRadius, cBlurSigma, sDiffMap, vTexCoord);
    #endif

    #ifdef BLUR9
        gl_FragColor = GaussianBlur(9, cBlurDir, cBlurHInvSize * cBlurRadius, cBlurSigma, sDiffMap, vTexCoord);
    #endif
}

#endif // defined COMPILEVS

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "fog.inc"

VS_OUT_FS_IN(vec2 vTexCoord)
VS_OUT_FS_IN(vec4 vWorldPos)
#ifdef VERTEXCOLOR
    VS_OUT_FS_IN(vec4 vColor)
#endif
#ifdef SOFTPARTICLES
    VS_OUT_FS_IN(vec4 vScreenPos)
    uniform float cSoftParticleFadeScale;
#endif


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = GetTexCoord(iTexCoord);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));

    #ifdef SOFTPARTICLES
        vScreenPos = GetScreenPos(gl_Position);
    #endif

    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        vec4 diffColor = cMatDiffColor * texture(sDiffMap, vTexCoord);
        #ifdef ALPHAMASK
            if (diffColor.a < 0.5)
                discard;
        #endif
    #else
        vec4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= vColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    // Soft particle fade
    // In expand mode depth test should be off. In that case do manual alpha discard test first to reduce fill rate
    #ifdef SOFTPARTICLES
        #ifdef EXPAND
            if (diffColor.a < 0.01)
                discard;
        #endif

        float particleDepth = vWorldPos.w;
        #ifdef HWDEPTH
            float depth = ReconstructDepth(textureProj(sDepthBuffer, vScreenPos).r);
        #else
            float depth = DecodeDepth(textureProj(sDepthBuffer, vScreenPos).rgb);
        #endif

        #ifdef EXPAND
            float diffZ = max(particleDepth - depth, 0.0) * (cFarClipPS - cNearClipPS);
            float fade = clamp(diffZ * cSoftParticleFadeScale, 0.0, 1.0);
        #else
            float diffZ = (depth - particleDepth) * (cFarClipPS - cNearClipPS);
            float fade = clamp(1.0 - diffZ * cSoftParticleFadeScale, 0.0, 1.0);
        #endif

        #ifndef ADDITIVE
            diffColor.a = max(diffColor.a - fade, 0.0);
        #else
            diffColor.rgb = max(diffColor.rgb - fade, vec3(0.0, 0.0, 0.0));
        #endif
    #endif

    gl_FragColor = vec4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
}
#endif // def COMPILEFS

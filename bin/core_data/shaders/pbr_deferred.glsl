#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "lighting.inc"
#dv_include "constants.inc"
#dv_include "pbr.inc"
#line 40007

#ifdef DIRLIGHT
    VS_OUT_FS_IN(vec2 vScreenPos)
#else
    VS_OUT_FS_IN(vec4 vScreenPos)
#endif
VS_OUT_FS_IN(vec3 vFarRay)
#ifdef ORTHO
    VS_OUT_FS_IN(vec3 vNearRay)
#endif


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    #ifdef DIRLIGHT
        vScreenPos = GetScreenPosPreDiv(gl_Position);
        vFarRay = GetFarRay(gl_Position);
        #ifdef ORTHO
            vNearRay = GetNearRay(gl_Position);
        #endif
    #else
        vScreenPos = GetScreenPos(gl_Position);
        vFarRay = GetFarRay(gl_Position) * gl_Position.w;
        #ifdef ORTHO
            vNearRay = GetNearRay(gl_Position) * gl_Position.w;
        #endif
    #endif
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    // If rendering a directional light quad, optimize out the w divide
    #ifdef DIRLIGHT
        vec4 depthInput = texture(sDepthBuffer, vScreenPos);
        #ifdef HWDEPTH
            float depth = ReconstructDepth(depthInput.r);
        #else
            float depth = DecodeDepth(depthInput.rgb);
        #endif
        #ifdef ORTHO
            vec3 worldPos = mix(vNearRay, vFarRay, depth);
        #else
            vec3 worldPos = vFarRay * depth;
        #endif
        vec4 albedoInput = texture(sAlbedoBuffer, vScreenPos);
        vec4 normalInput = texture(sNormalBuffer, vScreenPos);
        vec4 specularInput = texture(sSpecMap, vScreenPos);
    #else
        vec4 depthInput = textureProj(sDepthBuffer, vScreenPos);
        #ifdef HWDEPTH
            float depth = ReconstructDepth(depthInput.r);
        #else
            float depth = DecodeDepth(depthInput.rgb);
        #endif
        #ifdef ORTHO
            vec3 worldPos = mix(vNearRay, vFarRay, depth) / vScreenPos.w;
        #else
            vec3 worldPos = vFarRay * depth / vScreenPos.w;
        #endif
        vec4 albedoInput = textureProj(sAlbedoBuffer, vScreenPos);
        vec4 normalInput = textureProj(sNormalBuffer, vScreenPos);
        vec4 specularInput = textureProj(sSpecMap, vScreenPos);
    #endif

    // Position acquired via near/far ray is relative to camera. Bring position to world space
    vec3 eyeVec = -worldPos;
    worldPos += cCameraPosPS;

    vec3 normal = normalInput.rgb;
    float roughness = length(normal);
    normal = normalize(normal);

    vec3 specColor = specularInput.rgb;

    vec4 projWorldPos = vec4(worldPos, 1.0);

    vec3 lightDir;

    #if defined(DIRLIGHT)
        float atten = GetAtten(normal, worldPos, lightDir);
    #elif defined(SPOTLIGHT)
        float atten = GetAttenSpot(normal, worldPos, lightDir);
    #else
        float atten = GetAttenPoint(normal, worldPos, lightDir);
    #endif

    float shadow = 1.0;
    #ifdef SHADOW
        shadow *= GetShadowDeferred(projWorldPos, normal, depth);
    #endif

    #if defined(SPOTLIGHT)
        vec4 spotPos = projWorldPos * cLightMatricesPS[0];
        vec3 lightColor = spotPos.w > 0.0 ? textureProj(sLightSpotMap, spotPos).rgb * cLightColor.rgb : vec3(0.0);
    #elif defined(CUBEMASK)
        mat3 lightVecRot = mat3(cLightMatricesPS[0][0].xyz, cLightMatricesPS[0][1].xyz, cLightMatricesPS[0][2].xyz);
        vec3 lightColor = texture(sLightCubeMap, (worldPos - cLightPosPS.xyz) * lightVecRot).rgb * cLightColor.rgb;
    #else
        vec3 lightColor = cLightColor.rgb;
    #endif

    vec3 toCamera = normalize(eyeVec);
    vec3 lightVec = normalize(lightDir);

    float ndl = clamp(abs(dot(normal, lightVec)), M_EPSILON, 1.0);

    vec3 BRDF = GetBRDF(worldPos, lightDir, lightVec, toCamera, normal, roughness, albedoInput.rgb, specColor);

    gl_FragColor.a = 1.0;
    gl_FragColor.rgb = BRDF * lightColor * (atten * shadow) / M_PI;
}
#endif // def COMPILEFS

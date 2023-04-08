#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"
#dv_include "screen_pos.inc"
#dv_include "lighting.inc"
#dv_include "constants.inc"
#dv_include "fog.inc"
#dv_include "pbr.inc"
#dv_include "ibl.inc"
#line 30010

uniform float cWindHeightFactor;
uniform float cWindHeightPivot;
uniform float cWindPeriod;
uniform vec2 cWindWorldSpacing;
#ifdef WINDSTEMAXIS
    uniform vec3 cWindStemAxis;
#endif

#if defined(NORMALMAP)
    VS_OUT_FS_IN(vec4 vTexCoord)
    VS_OUT_FS_IN(vec4 vTangent)
#else
    VS_OUT_FS_IN(vec2 vTexCoord)
#endif
VS_OUT_FS_IN(vec3 vNormal)
VS_OUT_FS_IN(vec4 vWorldPos)
#ifdef VERTEXCOLOR
    VS_OUT_FS_IN(vec4 vColor)
#endif
#ifdef PERPIXEL
    #ifdef SHADOW
        VS_OUT_FS_IN(vec4 vShadowPos[NUMCASCADES])
    #endif
    #ifdef SPOTLIGHT
        VS_OUT_FS_IN(vec4 vSpotPos)
    #endif
    #ifdef POINTLIGHT
        VS_OUT_FS_IN(vec3 vCubeMaskVec)
    #endif
#else
    VS_OUT_FS_IN(vec3 vVertexLight)
    VS_OUT_FS_IN(vec4 vScreenPos)
    #ifdef ENVCUBEMAP
        VS_OUT_FS_IN(vec3 vReflectionVec)
    #endif
    #if defined(LIGHTMAP) || defined(AO)
        VS_OUT_FS_IN(vec2 vTexCoord2)
    #endif
#endif

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);

    #ifdef WINDSTEMAXIS
        float stemDistance = dot(iPos.xyz, cWindStemAxis);
    #else
        float stemDistance = iPos.y;
    #endif
    float windStrength = max(stemDistance - cWindHeightPivot, 0.0) * cWindHeightFactor;
    float windPeriod = cElapsedTime * cWindPeriod + dot(worldPos.xz, cWindWorldSpacing);
    worldPos.x += windStrength * sin(windPeriod);
    worldPos.z -= windStrength * cos(windPeriod);

    gl_Position = GetClipPos(worldPos);
    vNormal = GetWorldNormal(modelMatrix);
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));

    #ifdef VERTEXCOLOR
        vColor = iColor;
    #endif

    #if defined(NORMALMAP) || defined(DIRBILLBOARD)
        vec4 tangent = GetWorldTangent(modelMatrix);
        vec3 bitangent = cross(tangent.xyz, vNormal) * tangent.w;
        vTexCoord = vec4(GetTexCoord(iTexCoord), bitangent.xy);
        vTangent = vec4(tangent.xyz, bitangent.z);
    #else
        vTexCoord = GetTexCoord(iTexCoord);
    #endif

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec4 projWorldPos = vec4(worldPos, 1.0);

        #ifdef SHADOW
            // Shadow projection: transform from world space to shadow space
            for (int i = 0; i < NUMCASCADES; i++)
                vShadowPos[i] = GetShadowPos(i, vNormal, projWorldPos);
        #endif

        #ifdef SPOTLIGHT
            // Spotlight projection: transform from world space to projector texture coordinates
            vSpotPos = projWorldPos * cLightMatrices[0];
        #endif

        #ifdef POINTLIGHT
            vCubeMaskVec = (worldPos - cLightPos.xyz) * mat3(cLightMatrices[0][0].xyz, cLightMatrices[0][1].xyz, cLightMatrices[0][2].xyz);
        #endif
    #else
        // Ambient & per-vertex lighting
        #if defined(LIGHTMAP) || defined(AO)
            // If using lightmap, disregard zone ambient light
            // If using AO, calculate ambient in the FS
            vVertexLight = vec3(0.0, 0.0, 0.0);
            vTexCoord2 = iTexCoord1;
        #else
            vVertexLight = GetAmbient(GetZonePos(worldPos));
        #endif

        #ifdef NUMVERTEXLIGHTS
            for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
                vVertexLight += GetVertexLight(i, worldPos, vNormal) * cVertexLights[i * 3].rgb;
        #endif

        vScreenPos = GetScreenPos(gl_Position);

        #ifdef ENVCUBEMAP
            vReflectionVec = worldPos - cCameraPos;
        #endif
    #endif
}

#endif // defined COMPILEVS

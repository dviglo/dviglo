#dv_include "uniforms.inc"
#dv_include "transform.inc"

uniform float cWindHeightFactor;
uniform float cWindHeightPivot;
uniform float cWindPeriod;
uniform vec2 cWindWorldSpacing;
#ifdef WINDSTEMAXIS
    uniform vec3 cWindStemAxis;
#endif

VS_OUT_FS_IN(vec3 vTexCoord)


#ifdef COMPILEVS
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
    vTexCoord = vec3(GetTexCoord(iTexCoord), GetDepth(gl_Position));
}
#endif // def COMPILEVS

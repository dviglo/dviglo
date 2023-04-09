#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec3 vTexCoord)


#ifdef COMPILEVS
void main()
{
#ifdef IGNORENODETRANSFORM
    mat4 modelMatrix = mat4(
        vec4(1.0, 0.0, 0.0, cViewInv[0].w),
        vec4(0.0, 1.0, 0.0, cViewInv[1].w),
        vec4(0.0, 0.0, 1.0, cViewInv[2].w),
        vec4(0.0, 0.0, 0.0, 1.0));
#else
    mat4 modelMatrix = iModelMatrix;
#endif
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iPos.xyz;
}
#endif // def COMPILEVS


#ifdef COMPILEFS
void main()
{
    vec4 sky = cMatDiffColor * texture(sDiffCubeMap, vTexCoord);
    #ifdef HDRSCALE
        sky = pow(sky + clamp((cAmbientColor.a - 1.0) * 0.1, 0.0, 0.25), max(vec4(cAmbientColor.a), 1.0)) * clamp(cAmbientColor.a, 0.0, 1.0);
    #endif
    gl_FragColor = sky;
}
#endif // def COMPILEFS

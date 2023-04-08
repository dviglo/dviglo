#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec3 vTexCoord)

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = vec3(GetTexCoord(iTexCoord), GetDepth(gl_Position));
}

#elif defined COMPILEFS

void main()
{
    #ifdef ALPHAMASK
        float alpha = texture(sDiffMap, vTexCoord.xy).a;
        if (alpha < 0.5)
            discard;
    #endif

    gl_FragColor = vec4(EncodeDepth(vTexCoord.z), 1.0);
}

#endif // defined COMPILEVS

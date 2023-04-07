#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

varying vec2 vTexCoord;

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    gl_Position.z = gl_Position.w;
    vTexCoord = iTexCoord.xy;
}

#elif defined COMPILEFS

void main()
{
    gl_FragColor = texture(sDiffMap, vTexCoord);
}

#endif // defined COMPILEVS

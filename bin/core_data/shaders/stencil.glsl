#dv_include "uniforms.inc"
#dv_include "transform.inc"

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
}

#elif defined COMPILEFS

void main()
{
    gl_FragColor = vec4(1.0);
}

#endif // defined COMPILEVS

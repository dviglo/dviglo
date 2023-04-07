#dv_include "uniforms.inc"
#dv_include "transform.inc"

varying vec4 vColor;

#if defined COMPILEVS

void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vColor = iColor;
}

#elif defined COMPILEFS

void main()
{
    gl_FragColor = vColor;
}

#endif // defined COMPILEVS

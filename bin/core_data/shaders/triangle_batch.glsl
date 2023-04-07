#dv_include "uniforms.inc"
#dv_include "transform.inc"

varying vec4 vColor;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vColor = iColor;
}

void PS()
{
    gl_FragColor = vColor;
}

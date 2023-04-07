#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

varying vec2 vTexCoord;
varying vec4 vColor;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);

    vTexCoord = iTexCoord;
    vColor = iColor;
}

void PS()
{
    vec4 diffColor = cMatDiffColor * vColor;
    vec4 diffInput = texture(sDiffMap, vTexCoord);
    gl_FragColor = diffColor * diffInput;
}

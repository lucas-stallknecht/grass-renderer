@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<uniform> settings: Settings;
@group(0) @binding(2) var normalTexture: texture_2d<f32>;
@group(0) @binding(3) var textureSampler: sampler;

const ambientStrength = 0.7;
const diffuseStrength = 0.3;
const specularStrength = 0.5;

const specularCol: vec3f = vec3f(1.0, 0.89, 0.88);
const lightCol: vec3f = vec3f(1.0);
const redCol:vec3f = vec3f(0.52, 0.12, 0.1);
const greenCol: vec3f = vec3f(0.21, 0.52, 0.1);

@fragment
fn fragment_main(
    in: VertexOut
    ) -> @location(0) vec4f
{
    var ambientCol = ambientStrength * lightCol;

    var diff = max(dot(settings.lightDirection, in.normal), 0.0);
    var diffuseCol = diffuseStrength * diff * lightCol;

    var halfwayDir = normalize(settings.lightDirection - (in.worldPosition - cam.position));
    var spec = pow(max(dot(in.normal, halfwayDir), 0.0), 32.0);
    var specCol = specularStrength * spec * specularCol;

    var AO = mix(vec3f(0.2), vec3f(1.0), in.height);

    var fogFactor = 1.0 - pow(2.0, -pow(0.09 * distance(cam.position, in.worldPosition), 2.0));

    var col = ((ambientCol * AO) + diffuseCol) * redCol + specCol;
    col = mix(col, vec3f(0.1), fogFactor);

    return vec4f(col, 1.0);
}
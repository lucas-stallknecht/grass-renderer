@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<uniform> settings: Settings;
@group(0) @binding(2) var normalTexture: texture_2d<f32>;
@group(0) @binding(3) var textureSampler: sampler;

const ambientStrength = 0.3;
const diffuseStrength = 0.8;
const specularStrength = 0.3;
const wrapValue = 1.0;
// TODO sky L1 SH to replace constant ambient

// red specularCol : 1.0, 0.88, 0.89
const specularCol: vec3f = vec3f(0.88, 0.88, 1.0);
const lightCol: vec3f = vec3f(1.0);
const redCol:vec3f = vec3f(0.65, 0.21, 0.17);
const greenCol: vec3f = vec3f(0.29, 0.57, 0.24);

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    var uv = in.texCoord;
    if(front_facing) {
        uv.x = 1.0 - uv.x;
    }

    var tangentToWorld = mat3x3f(in.bitangent, in.tangent, in.normal);
    var normal = textureSample(normalTexture, textureSampler, uv).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tangentToWorld * normal);

    var ambientCol = ambientStrength * lightCol;


    var NdotL = dot(settings.lightDirection, mix(normal, vec3f(0.0, 1.0, 0.0), 0.7));
    // var diff = max(dot(settings.lightDirection, normal), 0.0);
    var diff = max(0.0, (NdotL + wrapValue) / (1.0 + wrapValue));
    var diffuseCol = diffuseStrength * diff * lightCol;

    var halfwayDir = normalize(settings.lightDirection - (in.worldPosition - cam.position));
    var spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    var specCol = specularStrength * spec * specularCol;

    var AO = mix(vec3f(0.2), vec3f(1.0), smoothstep(0.0, 1.0, in.height));

    var fogFactor = 1.0 - pow(2.0, -pow(0.09 * distance(cam.position, in.worldPosition), 2.0));

    var col = ((ambientCol + diffuseCol) * AO) * redCol + specCol;
    col = mix(col, vec3f(0.1), fogFactor);

    return vec4f(col, 1.0);
}
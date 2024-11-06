@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(1) @binding(0) var<uniform> settings: BladeSettings;
@group(1) @binding(1) var normalTex: texture_2d<f32>;

// TODO sky L1 SH to replace constant ambient
const greenCol: vec3f = vec3f(0.29, 0.57, 0.24);

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: BladeVertexOut
    ) -> @location(0) vec4f
{
    var uv = in.texCoord;
    if(front_facing) {
        uv.x = 1.0 - uv.x;
    }

    // Convert texture tangent space to world space
    // Maybe overkill (cause we blend normal to up vector anyways) but needed for rounded normals
    var tangentToWorld = mat3x3f(in.tangent, in.bitangent, in.normal);
    var normal = textureSample(normalTex, texSampler, uv).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tangentToWorld * normal);

    var ambientCol = settings.ambientStrength * settings.lightCol;

    // TODO change constant up vector with terrain normal
    // Blend normal towards up vector for homogen diffuse
    var NdotL = dot(settings.lightDirection, mix(normal, vec3f(0.0, 1.0, 0.0), 0.7));
    // Wrapped diffuse
    var diff = max(0.0, (NdotL + settings.wrapValue) / (1.0 + settings.wrapValue));
    var diffuseCol = settings.diffuseStrength * diff * settings.lightCol;

    // Blinn-Phong specular
    var halfwayDir = normalize(settings.lightDirection - normalize((in.worldPosition - global.cam.position)));
    var spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    var specCol = settings.specularStrength * spec * settings.specularCol;

    var AO = mix(vec3f(0.2), vec3f(1.0), smoothstep(0.0, 1.0, in.height));

    var fogFactor = 1.0 - pow(2.0, -pow(0.09 * distance(global.cam.position, in.worldPosition), 2.0));

    var col = ((ambientCol + diffuseCol) * AO) * settings.bladeCol + specCol;
    col = mix(col, vec3f(0.1), fogFactor);
    // col = in.color;
    return vec4f(col, 1.0);
}
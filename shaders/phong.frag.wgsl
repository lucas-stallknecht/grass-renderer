@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(1) @binding(0) var diffuseTex: texture_2d<f32>;

const AMBIENT_STRENGTH = 0.3;
const DIFFUSE_STRENGTH = 0.8;
const SPECULAR_STRENGTH = 1.0;
const WRAP = 0.5;
const SPEC_EXP = 64.0;

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    var albedo = textureSample(diffuseTex, texSampler, in.texCoord).rgb;

    var ambientCol = AMBIENT_STRENGTH * mix(
        global.light.skyGroundCol,
        global.light.skyUpCol,
        in.normal.y * 0.5 + 0.5
    );
    var NdotL = dot(global.light.sunDir, in.normal);
    var diff = max(0.0, (NdotL + WRAP) / (1.0 + WRAP));

    var diffuseCol = DIFFUSE_STRENGTH * diff * global.light.sunCol;

    var halfwayDir = normalize(global.light.sunDir - normalize((in.worldPosition - global.cam.position)));
    var spec = pow(max(dot(in.normal, halfwayDir), 0.0), SPEC_EXP);
    var specCol = SPECULAR_STRENGTH * spec;

    var col = (ambientCol + diffuseCol + specCol) * albedo;
    col = mix(global.light.skyGroundCol, col, exponentialFog(distance(global.cam.position, in.worldPosition)));


    return vec4f(col, 1.0);
}
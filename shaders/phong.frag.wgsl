@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(1) @binding(0) var diffuseTex: texture_2d<f32>;

const ambientStrength = 0.3;
const diffuseStrength = 0.8;
const specularStrength = 1.0;
const wrapValue = 0.5;

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    var albedo = textureSample(diffuseTex, texSampler, in.texCoord).rgb;

    var ambientCol = ambientStrength * mix(
        global.light.skyGroundCol,
        global.light.skyUpCol,
        in.normal.y * 0.5 + 0.5
    );
    var NdotL = dot(global.light.sunDir, in.normal);
    var diff = max(0.0, (NdotL + wrapValue) / (1.0 + wrapValue));

    var diffuseCol = diffuseStrength * diff * global.light.sunCol;


    // Blinn-Phong specular
    var halfwayDir = normalize(global.light.sunDir - normalize((in.worldPosition - global.cam.position)));
    var spec = pow(max(dot(in.normal, halfwayDir), 0.0), 64.0);
    var specCol = specularStrength * spec;

    var fogFactor = 1.0 - pow(4.0, -pow(0.09 * distance(global.cam.position, in.worldPosition), 4.0));
    var col = (ambientCol + diffuseCol + specCol) * albedo;
    col = mix(col, global.light.skyGroundCol, fogFactor);


    return vec4f(col, 1.0);
}
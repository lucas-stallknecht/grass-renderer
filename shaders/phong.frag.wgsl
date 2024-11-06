@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(1) @binding(0) var diffuseTex: texture_2d<f32>;


@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    var col = textureSample(diffuseTex, texSampler, in.texCoord).rgb;
    return vec4f(col, 1.0);
}
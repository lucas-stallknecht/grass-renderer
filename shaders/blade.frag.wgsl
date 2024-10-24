@group(0) @binding(2) var normalTexture: texture_2d<f32>;
@group(0) @binding(3) var textureSampler: sampler;


@fragment
fn fragment_main(in: VertexOut) -> @location(0) vec4f {
    var col = textureSample(normalTexture, textureSampler, in.texCoord).rgb;
    col = in.color;
    return vec4f(vec3f(col), 1.0);
}
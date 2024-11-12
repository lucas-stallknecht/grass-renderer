@group(0) @binding(0) var<uniform> global: Global;
@group(2) @binding(0) var<uniform> model: mat4x4f;


@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
) -> VertexOut {
    let worldPos = model * vec4(pos, 1.0);
    let worldNormal = model * vec4(normal, 0.0);
    var output: VertexOut;
    output.position = global.cam.proj * global.cam.view * worldPos;
    output.worldPosition = worldPos.xyz / worldPos.w;
    output.texCoord = texCoord;
    output.normal = normalize(worldNormal.xyz);
    return output;
}
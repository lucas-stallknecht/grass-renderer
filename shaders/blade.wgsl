struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color: vec3f
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4<f32>;
@group(0) @binding(1) var<storage, read> bladePositions: array<vec4<f32>>;


fn translate(pos: vec3<f32>) -> mat4x4<f32> {
    return mat4x4<f32>(
        vec4<f32>(1.0, 0.0, 0.0, 0.0),
        vec4<f32>(0.0, 1.0, 0.0, 0.0),
        vec4<f32>(0.0, 0.0, 1.0, 0.0),
        vec4<f32>(pos.x, pos.y, pos.z, 1.0)
    );
}


@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) texCoord: vec2<f32>
    ) -> VertexOut
{
    var modelMatrix = translate(bladePositions[instanceIndex].xyz);

    var output : VertexOut;
    output.position = viewProj * modelMatrix * vec4(pos, 1.0);
    output.color = vec3(saturate(dot(normal, vec3(0.6, 1.0, 0.5))));
    return output;
}

@fragment
fn fragment_main(fragData: VertexOut) -> @location(0) vec4f {
    return vec4f(fragData.color, 1.0);
}
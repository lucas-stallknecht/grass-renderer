struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color: vec3f
}


@group(0) @binding(0) var<uniform> viewProj: mat4x4<f32>;

@vertex
fn vertex_main(@location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) texCoord: vec2<f32>
    ) -> VertexOut
{
    var output : VertexOut;
    output.position = viewProj * vec4(pos, 1.0);
    output.color = vec3(saturate(dot(normal, vec3(0.6, 1.0, 0.5))));
    return output;
}

@fragment
fn fragment_main(fragData: VertexOut) -> @location(0) vec4f {
    return vec4f(fragData.color, 1.0);
}
struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color: vec3f
}

@fragment
fn fragment_main(fragData: VertexOut) -> @location(0) vec4f {
    return vec4f(fragData.color, 1.0);
}
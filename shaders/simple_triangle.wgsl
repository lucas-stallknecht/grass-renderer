@vertex
fn vertex_main(@location(0) vPos: vec2<f32>) -> @builtin(position) vec4f
{
    return vec4f(vPos, 0.0, 1.0);
}

@fragment
fn fragment_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
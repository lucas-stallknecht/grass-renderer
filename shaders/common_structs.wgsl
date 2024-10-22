struct Blade {
    position: vec3f,
    size: f32,
    uv: vec2f,
    angle: f32,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color: vec3f
}
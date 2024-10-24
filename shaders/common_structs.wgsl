struct Blade {
    position: vec3f,
    size: f32,
    uv: vec2f,
    angle: f32,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) color: vec3f,
    @location(2) texCoord: vec2f,
}
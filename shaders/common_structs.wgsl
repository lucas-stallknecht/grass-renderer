struct Blade {
    position: vec3f,
    size: f32,
    uv: vec2f,
    angle: f32,
}

struct Camera {
    view: mat4x4f,
    proj: mat4x4f,
    position: vec3f,
    direction: vec3f,
}

struct Settings {
    windDirection: vec3f,
    p1: f32,
    lightDirection: vec3f,
    p2: f32,
    windFrequency: f32,
    windStrength: f32,
    time: f32,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
    @location(3) height: f32,
    @location(4) tangent: vec3f,
    @location(5) bitangent: vec3f
}
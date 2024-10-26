struct Blade {
    position: vec3f,
    height: f32,
    uv: vec2f,
    angle: f32,
}

struct Camera {
    view: mat4x4f,
    proj: mat4x4f,
    position: vec3f,
    direction: vec3f,
}

struct BladeSettings {
    wind: vec4f, // vec3 for direction, w for strength
    lightDirection: vec3f,
    windFrequency: f32,
    lightCol: vec3f,
    wrapValue: f32,
    bladeCol: vec3f,
    ambientStrength: f32,
    specularCol: vec3f,
    diffuseStrength: f32,
    specularStrength: f32
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
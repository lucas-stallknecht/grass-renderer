struct Blade {
    c0: vec3f, // root
    idHash: f32,
    uv: vec2f,
    height: f32,
    padding: f32,
    c1: vec3f, // tip
    c2: vec3f, // bendingControlPoint
    facingDirection: vec3f,
}

struct Camera {
    view: mat4x4f,
    proj: mat4x4f,
    position: vec3f,
    direction: vec3f,
}

struct Global {
    cam: Camera,
    time: f32,
}

struct BladeSettings {
    lightCol: vec3f,
    wrapValue: f32,
    lightDirection: vec3f,
    diffuseStrength: f32,
    bladeCol: vec3f,
    ambientStrength: f32,
    specularCol: vec3f,
    specularStrength: f32
}

struct BladeVertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
    @location(3) height: f32,
    @location(4) tangent: vec3f,
    @location(5) bitangent: vec3f,
    @location(6) color: vec3f,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
}
struct Blade {
    c0: vec3f, // root
    idHash: f32,
    uv: vec2f,
    height: f32,
    relativeHeight: f32,
    c1: vec3f, // tip
    c2: vec3f, // bendingControlPoint
    facingDirection: vec3f,
    collisionStrength: f32,
}

struct Camera {
    view: mat4x4f,
    proj: mat4x4f,
    position: vec3f,
    direction: vec3f,
    invView: mat4x4f,
    invProj: mat4x4f,
}

struct Light {
    skyUpCol: vec3f,
    skyGroundCol: vec3f,
    sunCol: vec3f,
    sunDir: vec3f
};

struct Global {
    cam: Camera,
    light: Light,
    time: f32,
    maxGrassHeight: f32,
}

struct BladeSettings {
    smallerBladeCol: vec3f,
    ambientStrength: f32,
    tallerBladeCol: vec3f,
    wrapValue: f32,
    specularCol: vec3f,
    specularStrength: f32,
    diffuseStrength: f32,
}

struct BladeVertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
    @location(3) relativeHeight: f32,
    @location(4) AOValue: f32,
    @location(5) tangent: vec3f,
    @location(6) bitangent: vec3f,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
}
// TODO change constant up vector with terrain normal
const UP = vec3f(0.0, 1.0, 0.0);
const FOG_EXT = 2.0;
const FOG_DENSITY = 0.09;

fn exponentialFog(distance: f32) -> f32 {
    return pow(2.0, -pow(FOG_DENSITY * distance, FOG_EXT));
}

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
    frameNumber: u32,
}

struct BladeSettings {
    smallerBladeCol: vec3f,
    ambientStrength: f32,
    tallerBladeCol: vec3f,
    wrapValue: f32,
    specularCol: vec3f,
    specularStrength: f32,
    diffuseStrength: f32,
    useShadows: f32,
}

struct BladeVertexOut {
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) screenPosition: vec4f, // used to sample SSS
    @location(2) texCoord: vec2f,
    @location(3) normal: vec3f,
    @location(4) relativeHeight: f32,
    @location(5) AOValue: f32,
    @location(6) tangent: vec3f,
    @location(7) bitangent: vec3f,
}

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) texCoord: vec2f,
    @location(2) normal: vec3f,
}
@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<uniform> time: f32;
@group(0) @binding(2) var<uniform> settings: BladeSettings;
@group(1) @binding(0) var<storage, read> bladePositions: array<Blade>;
const tangent: vec3f = vec3f(0.0, 1.0, 0.0);
const bitangent: vec3f = vec3f(0.0, 0.0, 1.0);

const spherePos = vec3f(0.0, 1.0, 0.0);
const sphereRadius = 1.0;

fn mod289(x: vec2f) -> vec2f {
    return x - floor(x * (1. / 289.)) * 289.;
}

fn mod289_3(x: vec3f) -> vec3f {
    return x - floor(x * (1. / 289.)) * 289.;
}

fn permute3(x: vec3f) -> vec3f {
    return mod289_3(((x * 34.) + 1.) * x);
}

fn simplexNoise2(v: vec2f) -> f32 {
    let C = vec4(
        0.211324865405187, // (3.0-sqrt(3.0))/6.0
        0.366025403784439, // 0.5*(sqrt(3.0)-1.0)
        -0.577350269189626, // -1.0 + 2.0 * C.x
        0.024390243902439 // 1.0 / 41.0
    );
    var i = floor(v + dot(v, C.yy));
    let x0 = v - i + dot(i, C.xx);
    var i1 = select(vec2(0., 1.), vec2(1., 0.), x0.x > x0.y);
    var x12 = x0.xyxy + C.xxzz;
    x12.x = x12.x - i1.x;
    x12.y = x12.y - i1.y;
    i = mod289(i);
    var p = permute3(permute3(i.y + vec3(0., i1.y, 1.)) + i.x + vec3(0., i1.x, 1.));
    var m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), vec3(0.));
    m *= m;
    m *= m;
    let x = 2. * fract(p * C.www) - 1.;
    let h = abs(x) - 0.5;
    let ox = floor(x + 0.5);
    let a0 = x - ox;
    m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);
    let g = vec3(a0.x * x0.x + h.x * x0.y, a0.yz * x12.xz + h.yz * x12.yw);
    return 130. * dot(m, g);
}



fn translate(pos: vec3f) -> mat4x4f {
    return mat4x4f(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        pos.x, pos.y, pos.z, 1.0
    );
}

fn resizeY(h: f32) -> mat4x4f {
    return mat4x4f(
        1.0, 0.0, 0.0, 0.0,
        0.0, h, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

fn rotateY(angle: f32) -> mat4x4f {
    let c = cos(angle);
    let s = sin(angle);
    return mat4x4f(
        c, 0.0, s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        -s, 0.0, c, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

fn ease(x: f32, n: f32)-> f32 {
    return pow(x, n);
}

fn displace(p: ptr<function, vec4f>, vertexLocalY: f32, bladeHeight: f32, idHash: f32) {
    // Wind sway effect
    var f = time * settings.windFrequency * settings.wind.xyz;
    var windNoise = 0.5 + 0.5 * (
        simplexNoise2((*p).xz * 0.2 - f.xz + (vertexLocalY * 0.25)) * 0.7 +
        simplexNoise2((*p).xz * 1.2 - f.xz) * 0.3
    );
    var swayFactor = mix(0.85, 1.0, idHash);
    //                                                                    taller blades will sway more distance
    *p += vec4f(settings.wind.xyz, 0.0) * windNoise * ease(vertexLocalY, 2.0) * (settings.wind.w * bladeHeight) * swayFactor;
}


@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
    ) -> VertexOut
{
    // Modify vertex position according to blade generation
    let bladePos = bladePositions[instanceIndex].position;
    let bladeHiehgt = bladePositions[instanceIndex].height;
    let idHash =bladePositions[instanceIndex].idHash;

    var modelMatrix = translate(bladePos);
    modelMatrix *= rotateY(bladePositions[instanceIndex].angle);
    modelMatrix *= resizeY(bladeHiehgt);

    var worldPos = modelMatrix * vec4f(pos, 1.0);
    displace(&worldPos, pos.y, bladeHiehgt, idHash);

    // -- Recalculating normal
    var posPlusTangent = pos + 0.01 * tangent;
    var worldPosPlusTangent = modelMatrix * vec4f(posPlusTangent, 1.0);
    displace(&worldPosPlusTangent, posPlusTangent.y, bladeHiehgt, idHash);
    // --
    var posPlusBitangent = pos + 0.01 * bitangent;
    var worldPosPlusBitangent = modelMatrix * vec4f(posPlusBitangent, 1.0);
    displace(&worldPosPlusBitangent, posPlusBitangent.y, bladeHiehgt, idHash);
    // --
    var modifiedTangent = normalize(worldPosPlusTangent.xyz - worldPos.xyz);
    var modifiedBitangent = normalize(worldPosPlusBitangent.xyz - worldPos.xyz);
    var modifiedNormal = normalize(cross(modifiedTangent, modifiedBitangent));
    // -------

    let camToVertVector = normalize(worldPos.xyz - cam.position);
    var viewDotNormal = dot(modifiedNormal.xz, camToVertVector.xz);
    var viewSpaceShiftFactor = smoothstep(0.5, 1.0, 1.0 - abs(viewDotNormal));
    // Font and back faces have different vectors
    let inversion = -sign(dot(modifiedNormal, camToVertVector));
    modifiedNormal *= inversion;
    modifiedBitangent *= inversion;

    var mvPos = cam.view * worldPos;
    // mvPos.x += viewSpaceShiftFactor * sign(viewDotNormal) * pos.z * 0.5;


    var output: VertexOut;
    output.position = cam.proj * mvPos;
    output.worldPosition = worldPos.xyz / worldPos.w;
    output.texCoord = texCoord;
    output.normal = modifiedNormal;
    output.height = pos.y;
    output.tangent = modifiedTangent;
    output.bitangent = modifiedBitangent;
    return output;
}
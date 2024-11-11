@group(0) @binding(0) var<uniform> global: Global;
@group(1) @binding(1) var<storage, read> bladePositions: array<Blade>;


fn bezier(t: f32, c0: vec3f,  c1: vec3f,  c2: vec3f) -> vec3f {
    return  c2 +
    pow(1.0 - t, 2.0) * (c0 - c2) +
    pow(t, 2.0) * (c1 - c2);
}

fn dBezier(t: f32, c0: vec3f,  c1: vec3f,  c2: vec3f) -> vec3f {
    return  2.0 * (1.0 - t) * (c0 - c2) - 2.0 * t * (c1 - c2);
}

fn ease(x: f32, n: f32)-> f32 {
    return pow(x, n);
}

const SWAY_Y = 0.2;
const SWAY_FREQ = 2.3;
const VERTEX_SHIFTING_AMOUNT = 0.3;


@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
    ) -> BladeVertexOut
{
    let blade = bladePositions[instanceIndex];

    var c1 = blade.c1;
    let swayAmplitude = SWAY_Y * distance(blade.c0.xz, blade.c1.xz) / blade.height;
    let swayPhase = blade.idHash * pos.y;
    // Bobbing up and down gives a better swaying effect than just tilting uniformally
    c1 += pos.y * swayAmplitude * sin(SWAY_FREQ * (global.time + swayPhase));

    // Conservation of length
    let L0 = distance(blade.c0, c1);
    let L1 = distance(blade.c0, blade.c2) + distance(blade.c2, c1);
    let L = (2.0 * L0 + L1) / 3.0;
    let r = blade.height / L;

    var c2 = blade.c0 + r * (blade.c2 - blade.c0);
    c1 = c2 + r * (c1 - c2);

    var bezierPos = bezier(pos.y, blade.c0, c1, blade.c2);
    // "Extruding along tangent"
    var tangent = normalize(cross(-blade.facingDirection, vec3f(0.0, 1.0, 0.0)));
    bezierPos += pos.z * tangent;
    var worldPos = vec4f(bezierPos, 1.0);

    var bitangent = normalize(dBezier(pos.y, blade.c0, c1, blade.c2));
    var modifiedNormal = normalize(cross(bitangent, tangent));

    // Front and back faces have different vectors
    let vertToCamVector = normalize(global.cam.position - worldPos.xyz);
    let inversion = sign(dot(modifiedNormal, vertToCamVector));
    modifiedNormal *= inversion;
    bitangent *= inversion;

    var viewDotTangent = dot(tangent.xz, vertToCamVector.xz);
    var viewSpaceShiftFactor = smoothstep(0.4, 1.0, abs(viewDotTangent));
    // We used to shift the vert in view space but this caused weird z-fighting. We do it in world space now.
    var thicknessAmount = viewSpaceShiftFactor * sign(-viewDotTangent) * pos.z * VERTEX_SHIFTING_AMOUNT;
     worldPos.x += thicknessAmount * modifiedNormal.x;
     worldPos.z += thicknessAmount * modifiedNormal.z;
    var viewPos = global.cam.view * worldPos;
    var screenPos = global.cam.proj * viewPos;

    var output: BladeVertexOut;
    output.position = screenPos;
    output.worldPosition = worldPos.xyz / worldPos.w;
    output.screenPosition = screenPos;
    output.texCoord = texCoord;
    output.relativeHeight = blade.relativeHeight;
    output.AOValue = smoothstep(0.0, 1.0, pos.y);
    output.normal = modifiedNormal;
    output.tangent = tangent;
    output.bitangent = bitangent;
    return output;
}
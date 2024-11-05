@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<uniform> time: f32;
@group(0) @binding(2) var<uniform> settings: BladeSettings;
@group(1) @binding(0) var<storage, read> bladePositions: array<Blade>;


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

fn resizeY(r: f32) -> mat4x4f {
    return mat4x4f(
        r, 0.0, 0.0, 0.0,
        0.0, r, 0.0, 0.0,
        0.0, 0.0, r, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
    ) -> VertexOut
{
    let blade = bladePositions[instanceIndex];

    var c1 = blade.c1;
    let bobbingAmplitude = 0.2 * distance(blade.c0.xz, blade.c1.xz) / blade.height;
    let bobbingPhase = blade.idHash * pos.y;
    let bobbingFreq = 0.0;
    // Bobbing up and down gives a better swaying effect than just tilting uniformally
    c1 += pos.y * bobbingAmplitude * sin(bobbingFreq * (time + bobbingPhase));

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

    // Font and back faces have different vectors
    let vertToCamVector = normalize(cam.position - worldPos.xyz);
    let inversion = sign(dot(modifiedNormal, vertToCamVector));
    modifiedNormal *= inversion;
    bitangent *= inversion;

    var viewDotTangent = dot(tangent.xz, vertToCamVector.xz);
    var viewSpaceShiftFactor = smoothstep(0.4, 1.0, abs(viewDotTangent));
    // We used to shift the vert in view space but this caused weird z-fighting. We do it in world space now.
    var thicknessAmount = viewSpaceShiftFactor * sign(-viewDotTangent) * pos.z * 0.3;
    // worldPos.x += thicknessAmount * modifiedNormal.x;
    // worldPos.z += thicknessAmount * modifiedNormal.z;
    var mvPos = cam.view * worldPos;

    var output: VertexOut;
    output.position = cam.proj * mvPos;
    output.worldPosition = worldPos.xyz / worldPos.w;
    output.texCoord = texCoord;
    output.height = pos.y;
    output.normal = modifiedNormal;
    output.tangent = tangent;
    output.bitangent = bitangent;
    output.color = vec3f(distance(c1.x, blade.c0.x));
    return output;
}
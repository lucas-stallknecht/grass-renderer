struct MovSettings {
    wind: vec4f,
    windFrequency: f32,
};

// Sphercial collisions
// https://www.cg.tuwien.ac.at/research/publications/2016/JAHRMANN-2016-IGR/JAHRMANN-2016-IGR-thesis.pdf
//    var dist = distance(spherePos, (*p).xyz);
//
//    var toDisplaceT = sphereRadius - clamp(dist, 0.0, sphereRadius);
//    var collisionT = normalize((*p).xyz - spherePos) * toDisplaceT;
//    *p += vec4f(collisionT, 0.0);

@group(0) @binding(0) var<storage, read_write> bladePositions: array<Blade>;
@group(1) @binding(0) var<uniform> movSettings: MovSettings;
@group(1) @binding(1) var<uniform> time: f32;


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


@compute
@workgroup_size(1, 1, 1)
fn main(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>
) {
    // Check if the index is within bounds
    let workgroup_index =
         workgroup_id.x +
         workgroup_id.y * num_workgroups.x +
         workgroup_id.z * num_workgroups.x * num_workgroups.y;
    let global_invocation_index = workgroup_index + local_invocation_index;
    if (global_invocation_index >= num_workgroups.x * num_workgroups.y) {
        return;
    }

    let c0 = bladePositions[global_invocation_index].c0;
    let height = bladePositions[global_invocation_index].height;
    let idHash = bladePositions[global_invocation_index].idHash;

    var f = time * movSettings.windFrequency * movSettings.wind.xyz;
    var windNoise = 0.5 + 0.5 * (
        simplexNoise2((c0).xz * 0.2 - f.xz ) * 0.7 +
        simplexNoise2((c0).xz * 1.2 - f.xz) * 0.3
    );
    var varianceFactor = mix(0.85, 1.0, idHash);

    //                                               taller blades will sway more distance
    var tiltVec = movSettings.wind.xyz * windNoise * (movSettings.wind.w * height) * varianceFactor;

    var c1 = c0 + tiltVec + vec3f(0.0, height, 0.0);
    // more tilted blades should bend more
    var bendingFactor = 0.5 + 0.25 * smoothstep(0.0, height, length(tiltVec));
    var c2 = c0 + vec3f(0.0, height * 0.75, 0.0);
    bladePositions[global_invocation_index].c1 = c1;
    bladePositions[global_invocation_index].c2 = c2;

}
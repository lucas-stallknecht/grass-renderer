// Sphercial collisions :
// https://www.cg.tuwien.ac.at/research/publications/2016/JAHRMANN-2016-IGR/JAHRMANN-2016-IGR-thesis.pdf
// https://www.cg.tuwien.ac.at/research/publications/2017/JAHRMANN-2017-RRTG/JAHRMANN-2017-RRTG-draft.pdf

struct MovSettings {
    wind: vec4f,
    windFrequency: f32,
};

@group(0) @binding(0) var<storage, read_write> bladePositions: array<Blade>;
@group(1) @binding(0) var<uniform> movSettings: MovSettings;
@group(1) @binding(1) var<uniform> time: f32;


// Spheres are just placed by hand for now
const nSpheres: u32 = 3;
const spheres = array<vec4f, nSpheres>(
    vec4f(-0.23, 0.42, 1.15, 0.7), // stone
    vec4f(0.0, 0.86, 0.55, 0.6), // pillar next to stone
    vec4f(1.22, 0.88, 0.5, 0.6), // pillar close to the cam
);


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

fn calcSphereTranslation(p: vec3f, c: vec3f, r: f32) -> vec3f {
    var dist = distance(c, p);
    return min(dist - r, 0.0) * (c - p) / dist;
}

fn calcC2(c0: vec3f, c1: vec3f, height: f32) -> vec3f {
    var tiltProjectionLength = length(c1 - c0 - UP * dot(c1 - c0, UP));
    //                      more tilted blades should bend more
    return c0 + height * mix(0.25, 0.9, tiltProjectionLength / height) * UP;
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

    let blade = bladePositions[global_invocation_index];

    var noisePhase = time * movSettings.windFrequency * movSettings.wind.xyz;
    var windNoise = 0.5 + 0.5 * (
        simplexNoise2((blade.c0).xz * 0.1 - noisePhase.xz) * 0.5 +
        simplexNoise2((blade.c0).xz * 0.5 - noisePhase.xz) * 0.1 +
        simplexNoise2((blade.c0).xz * 3.5 - noisePhase.xz) * 0.4
    );
    var varianceFactor = mix(0.85, 1.0, blade.idHash);

    //                                               taller blades will sway more distance
    var tiltDist = movSettings.wind.xyz * windNoise * (movSettings.wind.w * blade.height) * varianceFactor;

    // Calculate bezier control points
    var c1 = blade.c0 + max(1.0 - blade.collisionStrength, 0.0) * tiltDist + blade.height * UP;
    var c2 = calcC2(blade.c0, c1, blade.height);

    var newCollisionStrength = 0.0;
    // Experimental : Spheres collision
    for (var i: u32 = 0u; i < nSpheres; i = i + 1u) {
       let sphere = spheres[i];
       let d = distance(blade.c0, sphere.xyz);
           if (d < blade.height + sphere.w) {
                // middle point
               var m = 0.25 * blade.c0 + 0.5 * c2 + 0.25 * c1;
               var t = calcSphereTranslation(c1, sphere.xyz, sphere.w) + 4.0 * calcSphereTranslation(m, sphere.xyz, sphere.w);
               c1 += t;
               // If a blade is in collision, it has less chance to be affected by wind next step
               newCollisionStrength = max(length(t)/sphere.w, newCollisionStrength);
           }
    }

    // State checking
    // Groudn collision : c1 -= UP * min(dot(UP, c1 - blade.c0), 0.0);
    c2 = calcC2(blade.c0, c1, blade.height);

    bladePositions[global_invocation_index].c1 = c1;
    bladePositions[global_invocation_index].c2 = c2;
    bladePositions[global_invocation_index].collisionStrength = newCollisionStrength;

}
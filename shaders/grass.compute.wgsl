// https://gist.github.com/munrocket/236ed5ba7e409b8bdf1ff6eca5dcdc39

fn hash11(n: u32) -> u32 {
    var h = n * 747796405u + 2891336453u;
    h = ((h >> ((h >> 28u) + 4u)) ^ h) * 277803737u;
    return (h >> 22u) ^ h;
}

fn rand11(f: f32) -> f32 { return f32(hash11(bitcast<u32>(f))) / f32(0xffffffff); }

fn hash22(p: vec2u) -> u32 {
    let p1 = 374761393u;
    let p2 = 1103515245u;
    let p3 = 12345u;
    var h = (p.x * p1) ^ (p.y * p2);
    h = h ^ (h >> 16);
    h *= p3;
    return h ^ (h >> 16);
}

fn rand22(f: vec2f) -> vec2f {
    let p = bitcast<vec2u>(f);
    return vec2f(f32(hash22(p)), f32(hash22(p + vec2u(1, 0)))) / f32(0xffffffff);
}

fn valueNoise2(n: vec2f) -> vec2f {
    let d = vec2f(0., 1.);
    let b = floor(n);
    let f = smoothstep(vec2f(0.), vec2f(1.), fract(n));

    return mix(
        mix(rand22(b), rand22(b + d.yx), f.x),
        mix(rand22(b + d.xy), rand22(b + d.yy), f.x),
        f.y
    );
}

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

    // First corner
    var i = floor(v + dot(v, C.yy));
    let x0 = v - i + dot(i, C.xx);

    // Other corners
    var i1 = select(vec2(0., 1.), vec2(1., 0.), x0.x > x0.y);

    // x0 = x0 - 0.0 + 0.0 * C.xx ;
    // x1 = x0 - i1 + 1.0 * C.xx ;
    // x2 = x0 - 1.0 + 2.0 * C.xx ;
    var x12 = x0.xyxy + C.xxzz;
    x12.x = x12.x - i1.x;
    x12.y = x12.y - i1.y;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation

    var p = permute3(permute3(i.y + vec3(0., i1.y, 1.)) + i.x + vec3(0., i1.x, 1.));
    var m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), vec3(0.));
    m *= m;
    m *= m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    let x = 2. * fract(p * C.www) - 1.;
    let h = abs(x) - 0.5;
    let ox = floor(x + 0.5);
    let a0 = x - ox;

    // Normalize gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);

    // Compute final noise value at P
    let g = vec3(a0.x * x0.x + h.x * x0.y, a0.yz * x12.xz + h.yz * x12.yw);
    return 130. * dot(m, g);
}

struct GrassSettings {
    sideLength: f32,
    density: f32,
    maxNoisePositionOffset: f32,
    bladeHeight: f32,
    bladeDeltaFactor: f32
};


@group(0) @binding(0) var<uniform> grassSettings: GrassSettings;
@group(0) @binding(1) var<storage, read_write> bladePositions: array<Blade>;

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

    var pos: vec3f = vec3f(-grassSettings.sideLength + f32(workgroup_id.x) / grassSettings.density,
                            0.0,
                           -grassSettings.sideLength + f32(workgroup_id.y) / grassSettings.density);
    let n = (valueNoise2(pos.xz * vec2f(grassSettings.density)) - 0.5) * grassSettings.maxNoisePositionOffset;
    pos.x += n.x;
    pos.z += n.y;

    let randomYSizeAddition = 0.75 * simplexNoise2(pos.xz * 0.4) + 0.25 * simplexNoise2(pos.xz * 0.8);
    let ySize = grassSettings.bladeHeight + randomYSizeAddition * grassSettings.bladeDeltaFactor;

    var blade: Blade;
    blade.position = pos;
    blade.size = ySize;
    blade.angle = rand11(f32(global_invocation_index)) * radians(360.0);

    bladePositions[global_invocation_index] = blade;

}
struct GrassSettings {
    sideLength: f32,
    density: f32,
    maxStep: f32,
};

@group(0) @binding(0) var<uniform> grassSettings: GrassSettings;
@group(0) @binding(1) var<storage, read_write> bladePositions: array<vec4<f32>>;

fn rand22(n: vec2f) -> f32 { return fract(sin(dot(n, vec2f(12.9898, 4.1414))) * 43758.5453); }

fn noise2(n: vec2f) -> f32 {
    let d = vec2f(0., 1.);
    let b = floor(n);
    let f = smoothstep(vec2f(0.), vec2f(1.), fract(n));
    return mix(mix(rand22(b), rand22(b + d.yx), f.x), mix(rand22(b + d.xy), rand22(b + d.yy), f.x), f.y);
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

    var pos: vec3f = vec3f(-grassSettings.sideLength + f32(workgroup_id.x) / grassSettings.density,
                            0.0,
                           -grassSettings.sideLength + f32(workgroup_id.y) / grassSettings.density);
    let noiseValue = (noise2(pos.xz * vec2f(grassSettings.density)) - 0.5) * grassSettings.maxStep;
    pos.x += noiseValue;
    pos.z += noiseValue;

    bladePositions[global_invocation_index] = vec4<f32>(pos, 0.0);

}
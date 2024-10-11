@group(0) @binding(0) var<storage, read_write> bladePositions: array<vec4<f32>>;

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

    bladePositions[global_invocation_index] = vec4<f32>(
    -5.0 + f32(workgroup_id.x) / 5.0,
    0.0,
    -5.0 + f32(workgroup_id.y) / 5.0,
    0.0);
    // bladePositions[global_invocation_index] = vec3<f32>(1.33, 3.44, 0.0);
}
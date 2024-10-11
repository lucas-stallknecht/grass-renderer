
@group(0) @binding(0) var<storage, read_write> testColor: vec3<f32>;

@compute
@workgroup_size(1, 1, 1)
fn main() {
    testColor = vec3(1.0, 0.5, 0.0);
}
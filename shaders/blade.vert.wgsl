struct Camera {
    viewProj: mat4x4f,
    dir: vec3f,
}

struct VertexOut {
    @builtin(position) position : vec4f,
    @location(0) color: vec3f
}

const bladeForwardXZ = vec2f(1.0, 0.0);

@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<storage, read> bladePositions: array<vec4f>;


fn translate(pos: vec3f) -> mat4x4f {
    return mat4x4f(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        pos.x, pos.y, pos.z, 1.0
    );
}

fn rotateY(angle: f32) -> mat4x4f {
    let c = cos(angle);
    let s = sin(angle);
    return mat4x4f(
        c, 0, s, 0,
        0, 1, 0, 0,
        -s, 0, c, 0,
        0, 0, 0, 1
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
    let camXZDir = normalize(cam.dir.xz);
    let bladePos = bladePositions[instanceIndex].xyz;

    var angleDiff = atan2(camXZDir.y, camXZDir.x) - atan2(bladeForwardXZ.y, bladeForwardXZ.x);

    var modelMatrix = translate(bladePos);
    modelMatrix *= rotateY(angleDiff);

    var output: VertexOut;
    output.position = cam.viewProj * modelMatrix * vec4f(pos, 1.0);

    let greenColor = vec3f(0.459, 0.89, 0.333);
    output.color = greenColor * mix(vec3f(0.3), vec3f(1.0), pos.y);

    return output;
}
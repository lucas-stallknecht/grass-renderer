struct Camera {
    view: mat4x4f,
    proj: mat4x4f,
    position: vec3f,
    direction: vec3f,
}

const lightDirection = vec3f(0.7, 1.0, 0.8);
const bladeForwardXZ = vec3f(0.0, 0.0, 1.0);

@group(0) @binding(0) var<uniform> cam: Camera;
@group(0) @binding(1) var<storage, read> bladePositions: array<Blade>;


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


@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
    ) -> VertexOut
{
    let bladePos = bladePositions[instanceIndex].position;

    var modelMatrix = translate(bladePos);
    modelMatrix *= rotateY(bladePositions[instanceIndex].angle);
    modelMatrix *= resizeY(bladePositions[instanceIndex].size);
    let roatedNormal = rotateY(bladePositions[instanceIndex].angle) * vec4f(normal, 0.0);

    let worldSpacePos = modelMatrix * vec4f(pos, 1.0);
    let camToVertVector = normalize(worldSpacePos.xyz - cam.position);

    var viewDotNormal = dot(roatedNormal.xz, camToVertVector.xz);
    var viewSpaceShiftFactor = smoothstep(0.5, 1.0, 1.0 - abs(viewDotNormal));

    var output: VertexOut;
    var mvPosition = cam.view * worldSpacePos;
    mvPosition.x += viewSpaceShiftFactor * sign(viewDotNormal) * pos.z * 0.5;
    output.position = cam.proj * mvPosition;

    let greenColor = vec3f(0.459, 0.89, 0.333);
    let dirLight = abs(dot(lightDirection, roatedNormal.xyz));
    output.color = greenColor * mix(vec3f(0.1), vec3f(0.9), pos.y);
    // output.color = vec3f(viewSpaceThickenFactor);
    // output.color = greenColor * (0.75 + 0.25 * dirLight);

    return output;
}
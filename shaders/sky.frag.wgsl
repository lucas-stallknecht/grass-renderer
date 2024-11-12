@group(0) @binding(0) var<uniform> global: Global;

fn easeOutExpo(x: f32) -> f32 {
    return 1 - pow(2.0, -10.0 * x);
}

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
) -> @location(0) vec4f {
    // normalized device coordinates (NDC) position
    let ndcPos = vec4f(in.texCoord * 2.0 - vec2f(1.0), 1.0, 1.0);

    let viewPos = global.cam.invProj * ndcPos;
    let viewDirection = normalize(viewPos.xyz / viewPos.w);

    let worldViewDir = (global.cam.invView * vec4f(viewDirection, 0.0)).xyz;

    let upFactor = easeOutExpo(max(dot(worldViewDir, UP), 0.0));
    let color = mix(global.light.skyGroundCol, global.light.skyUpCol, upFactor);

    return vec4f(color, 1.0);
}
@group(0) @binding(0) var<uniform> global: Global;

fn easeOutExpo(x: f32) -> f32 {
    return 1 - pow(2.0, -10.0 * x);
}

@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    var ndcPos = vec4f(in.texCoord * 2.0 - vec2f(1.0), 1.0, 1.0);

    var tar = global.cam.invProj * ndcPos;
    var normalizedTarget = normalize(tar.xyz / tar.w);
    var viewVec = (global.cam.invView * vec4f(normalizedTarget, 0.0)).xyz;
    var upFactor = max(dot(viewVec, vec3f(0.0, 1.0, 0.0)), 0.0);
    upFactor = easeOutExpo(upFactor);


    var col = mix(global.light.skyGroundCol, global.light.skyUpCol, upFactor);
    return vec4f(col, 1.0);
}
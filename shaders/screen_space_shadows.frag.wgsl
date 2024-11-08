struct SSSUniform {
    max_steps: u32,
    ray_max_distance: f32,
    thickness: f32,
    max_delta_from_original_depth: f32
}

@group(0) @binding(0) var<uniform> global: Global;
@group(1) @binding(0) var depthSampler: sampler_comparison;
@group(1) @binding(1) var depthTex: texture_depth_2d;
@group(1) @binding(2) var<uniform> s: SSSUniform;


fn isSaturated(p: vec2f) -> bool {
    return p.x >= -1.0 && p.x <= 1.0 && p.y >= -1.0 && p.y <= 1.0;
}

fn getDepthValue(uv: vec2f, dims: vec2u) -> f32 {
    let intCoords = vec2u(vec2f(uv.x, 1.0 - uv.y) * vec2f(dims));
    return textureLoad(depthTex, intCoords, 0);
}

fn InterleavedGradientNoise(uv: vec2f, FrameId: u32) -> f32{
	let tc = uv + f32(FrameId)  * (vec2f(47, 17) * 0.695f);
    var magic = vec3f( 0.06711056f, 0.00583715f, 52.9829189f );
    return fract(magic.z * fract(dot(tc, magic.xy)));
}


@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
    ) -> @location(0) vec4f
{
    let max_steps: u32 = s.max_steps;
    let ray_max_distance: f32 = s.ray_max_distance;
    let thickness: f32 = s.thickness;
    let max_delta_from_original_depth: f32 = s.max_delta_from_original_depth;
    let step_length: f32 = ray_max_distance / f32(max_steps);

    let dims = textureDimensions(depthTex);
    let depth = getDepthValue(in.texCoord, dims);

    let ndcPos = vec4f(in.texCoord * 2.0 - vec2f(1.0), depth, 1.0);

    var viewPos = global.cam.invProj * ndcPos;
    viewPos /= viewPos.w;

    var ro = viewPos.xyz;
    var rd = normalize(global.cam.view * vec4f(global.light.sunDir, 0.0)).xyz;
    var offset = InterleavedGradientNoise(vec2f(in.texCoord.x, 1.0 - in.texCoord.y) * vec2f(dims), u32(global.time * 150)) * 2.0f - 1.0f;
    var rayStep = rd * step_length;
    ro += rayStep * offset;

    var occlusion = 0.0;
    var rayUV: vec2f;

    var tempCol :f32;

    // just one step to test for now
    if(depth < 0.99) {
        for(var i = 0u; i < max_steps; i = i + 1u) {
            ro += rayStep;
            var rayProj = global.cam.proj * vec4f(ro, 1.0);
            var rayNdcPos =  rayProj.xyz / rayProj.w;
            rayUV = 0.5 + 0.5 * rayNdcPos.xy;

            if(isSaturated(rayUV.xy)) {
                var depthZ = getDepthValue(rayUV.xy, dims);
                var depthDelta = rayNdcPos.z - depthZ;

                var canCameraSeeRay = (depthDelta > 0.0f) && (depthDelta < thickness) && (abs(depthZ - 1.0) >= 0.01);
                var occludedByOriginalPixel = abs(rayNdcPos.z - depth) < max_delta_from_original_depth;

                if (canCameraSeeRay)
                {
                    occlusion = 0.1;
                    break;
                }
            }
        }
    }

    return vec4f(0.0, 0.0, 0.0, occlusion);
}
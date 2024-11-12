struct SSSUniform {
    max_steps: u32,
    ray_max_distance: f32,
    thickness: f32,
    max_delta_from_original_depth: f32
}

@group(0) @binding(0) var<uniform> global: Global;
@group(1) @binding(0) var depthTex: texture_depth_multisampled_2d;
@group(1) @binding(1) var<uniform> s: SSSUniform;
@group(1) @binding(2) var shadowTex: texture_storage_2d<rgba8unorm, write>;

const OCCLUSION_STRENGTH = 0.2;


fn isSaturated(p: vec2f) -> bool {
    return p.x >= -1.0 && p.x <= 1.0 && p.y >= -1.0 && p.y <= 1.0;
}

fn uvToTexCoords(uv: vec2f, dims: vec2u) -> vec2f {
    return vec2f(uv.x, 1.0 - uv.y) * vec2f(dims);
}

fn getDepthValue(uv: vec2f, dims: vec2u) -> f32 {
    let intCoords = vec2u(uvToTexCoords(uv, dims));
    return textureLoad(depthTex, intCoords, 0);
}

fn interleavedGradientNoise(uv: vec2f, frameId: u32) -> f32 {
    let tc = uv + f32(frameId) * (vec2f(47, 17) * 0.695f);
    var magic = vec3f(0.06711056f, 0.00583715f, 52.9829189f);
    return fract(magic.z * fract(dot(tc, magic.xy)));
}


@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: VertexOut
) -> @location(0) vec4f {
    let dims = textureDimensions(depthTex);
    let step_length = s.ray_max_distance / f32(s.max_steps);
    let originDepth = getDepthValue(in.texCoord, dims);

    let ndcPos = vec4f(in.texCoord * 2.0 - vec2f(1.0), originDepth, 1.0);
    var viewPos = global.cam.invProj * ndcPos;
    viewPos /= viewPos.w;

    var ro = viewPos.xyz;
    var rd = normalize(global.cam.view * vec4f(global.light.sunDir, 0.0)).xyz;
    var ditherOffset = interleavedGradientNoise(uvToTexCoords(in.texCoord, dims), global.frameNumber) * 2.0f - 1.0f;
    var rayStep = rd * step_length;
    ro += rayStep * ditherOffset;

    var occlusion = 0.0;
    var rayUV: vec2f;

    if originDepth < 0.995 {
        for (var i = 0u; i < s.max_steps; i = i + 1u) {
            ro += rayStep;
            var rayProj = global.cam.proj * vec4f(ro, 1.0);
            var rayNdcPos = rayProj.xyz / rayProj.w;
            rayUV = 0.5 + 0.5 * rayNdcPos.xy;

            if isSaturated(rayUV.xy) {
                var depthZ = getDepthValue(rayUV.xy, dims);
                var depthDelta = rayNdcPos.z - depthZ;

                var canCameraSeeRay = (depthDelta > 0.0f) && (depthDelta < s.thickness);
                var occludedByOriginalPixel = abs(rayNdcPos.z - originDepth) < s.max_delta_from_original_depth;

                if canCameraSeeRay {
                    occlusion = OCCLUSION_STRENGTH;
                    break;
                }
            }
        }
    }
    textureStore(shadowTex, vec2<i32>(uvToTexCoords(in.texCoord, dims)), vec4f(vec3f(1.0 - occlusion), 1.0));

    return vec4f(0.0, 0.0, 0.0, 0.0);
}
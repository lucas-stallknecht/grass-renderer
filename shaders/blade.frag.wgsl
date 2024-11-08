@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(0) @binding(2) var depthTex: texture_depth_multisampled_2d;
@group(1) @binding(0) var<uniform> settings: BladeSettings;
@group(1) @binding(1) var normalTex: texture_2d<f32>;

// TODO sky L1 SH to replace constant ambient
const greenCol: vec3f = vec3f(0.29, 0.57, 0.24);

const max_steps: u32 = 32;
const ray_max_distance: f32 = 2.00;
const thickness: f32 = 0.1;
const max_delta_from_original_depth = 0.01;
const step_length: f32 = ray_max_distance / f32(max_steps);

fn isSaturated(p: vec2f) -> bool {
    return p.x >= -1.0 && p.x <= 1.0 && p.y >= -1.0 && p.y <= 1.0;
}

fn getDepthValue(uv: vec2f, dims: vec2u) -> f32 {
    let intCoords = vec2u(vec2f(uv.x, 1.0 - uv.y) * vec2f(dims));
    return textureLoad(depthTex, intCoords, 0);
}

fn interleavedGradientNoise(uv: vec2f, FrameId: u32) -> f32{
	let tc = uv + f32(FrameId)  * (vec2f(47, 17) * 0.695f);
    var magic = vec3f( 0.06711056f, 0.00583715f, 52.9829189f );
    return fract(magic.z * fract(dot(tc, magic.xy)));
}


fn calcScreenSpaceShadow(viewPos: vec3f) -> f32{
    let dims = textureDimensions(depthTex);

    var p = global.cam.proj * vec4f(viewPos, 1.0);
    var ndcPos =  p.xyz / p.w;
    var uv = 0.5 + 0.5 * ndcPos.xy;

    var ro = viewPos.xyz;
    var rd = normalize(global.cam.view * vec4f(global.light.sunDir, 0.0)).xyz;
    var offset = interleavedGradientNoise(vec2f(uv.x, 1.0 - uv.y) * vec2f(dims), u32(global.time * 150)) * 2.0f - 1.0f;
    // var offset = 0.0;
    var rayStep = rd * step_length;
    ro += rayStep * offset;

    var occlusion = 0.0;
    var rayUV: vec2f;

    for(var i = 0u; i < max_steps; i = i + 1u) {
        ro += rayStep;
        var rayProj = global.cam.proj * vec4f(ro, 1.0);
        var rayNdcPos =  rayProj.xyz / rayProj.w;
        rayUV = 0.5 + 0.5 * rayNdcPos.xy;

        if(isSaturated(rayUV.xy)) {
            var depthZ = getDepthValue(rayUV.xy, dims);
            var depthDelta = rayNdcPos.z - depthZ;
            // occlusion = mix(rayNdcPos.z, depthZ, step(rayUV.x, 0.5));

            var canCameraSeeRay = (depthDelta > 0.0f) && (depthDelta < thickness);
            // var occludedByOriginalPixel = abs(rayNdcPos.z - depth) < max_delta_from_original_depth;

            if (canCameraSeeRay)
            {
                occlusion = 1.0;
                break;
            }
        }
    }
    return 1.0 - 0.2 * occlusion;
}


@fragment
fn fragment_main(
    @builtin(front_facing) front_facing: bool,
    in: BladeVertexOut
    ) -> @location(0) vec4f
{
    var uv = in.texCoord;
    if(front_facing) {
        uv.x = 1.0 - uv.x;
    }

    // Convert texture tangent space to world space
    // Maybe overkill (cause we blend normal to up vector anyways) but needed for rounded normals
    var tangentToWorld = mat3x3f(in.tangent, in.bitangent, in.normal);
    var normal = textureSample(normalTex, texSampler, uv).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(tangentToWorld * normal);

    var ambientCol = settings.ambientStrength * mix(
        global.light.skyGroundCol,
        global.light.skyUpCol,
        normal.y * 0.5 + 0.5
    );

    // TODO change constant up vector with terrain normal
    // Blend normal towards up vector for homogen diffuse
    var NdotL = dot(global.light.sunDir, mix(normal, vec3f(0.0, 1.0, 0.0), 0.7));
    // Wrapped diffuse
    var diff = max(0.0, (NdotL + settings.wrapValue) / (1.0 + settings.wrapValue));
    var diffuseCol = settings.diffuseStrength * diff * global.light.sunCol;

    // Blinn-Phong specular
    var halfwayDir = normalize(global.light.sunDir - normalize((in.worldPosition - global.cam.position)));
    var spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    var specCol = settings.specularStrength * spec * settings.specularCol;

    var shadow = 1.0;

    var AO = mix(vec3f(0.2), vec3f(1.0), in.AOValue);

    var fogFactor = 1.0 - pow(4.0, -pow(0.09 * distance(global.cam.position, in.worldPosition), 4.0));

    var col = ((ambientCol + diffuseCol * shadow) * AO) * mix(settings.smallerBladeCol, settings.tallerBladeCol, in.relativeHeight) + specCol;
    col = mix(col, global.light.skyGroundCol, fogFactor);
    return vec4f(col, 1.0);
}
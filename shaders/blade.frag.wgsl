@group(0) @binding(0) var<uniform> global: Global;
@group(0) @binding(1) var texSampler: sampler;
@group(0) @binding(2) var depthTex: texture_depth_multisampled_2d;
@group(1) @binding(0) var<uniform> settings: BladeSettings;
@group(1) @binding(2) var normalTex: texture_2d<f32>;
@group(1) @binding(3) var shadowTex: texture_2d<f32>;


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

    var miaou = in.uvPos.xy / in.uvPos.w;
    miaou = miaou * 0.5 + 0.5;
    miaou.y = 1.0 - miaou.y;
    var shadow = textureSample(shadowTex, texSampler, miaou).r;
    shadow = mix(1.0, shadow, settings.useShadows);

    var AO = mix(vec3f(0.2), vec3f(1.0), in.AOValue);

    var fogFactor = 1.0 - pow(4.0, -pow(0.09 * distance(global.cam.position, in.worldPosition), 4.0));

    var col = ((ambientCol + diffuseCol * shadow) * AO) * mix(settings.smallerBladeCol, settings.tallerBladeCol, in.relativeHeight) + specCol;
    col = mix(col, global.light.skyGroundCol, fogFactor);

    return vec4f(col, 1.0);
}
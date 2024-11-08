@vertex
fn vertex_main(
    @builtin(instance_index) instanceIndex: u32,
    @location(0) pos: vec3f,
    @location(1) normal: vec3f,
    @location(2) texCoord: vec2f
    ) -> VertexOut
{
    var output: VertexOut;
    output.position = vec4(pos, 1.0);
    output.texCoord = texCoord;
    return output;
}
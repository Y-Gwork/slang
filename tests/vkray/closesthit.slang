// closesthit.slang
//TEST:CROSS_COMPILE: -profile sm_6_3 -stage closesthit -entry main -target spirv-assembly

struct ReflectionRay
{
    float4 color;
};

StructuredBuffer<float4> colors;

layout(shaderRecordNV)
cbuffer ShaderRecord
{
	uint shaderRecordID;
}

void main(
	BuiltInTriangleIntersectionAttributes 	attributes,
	in out ReflectionRay 					ioPayload)
{
	uint materialID = InstanceIndex()
		+ InstanceID()
		+ PrimitiveIndex()
		+ HitKind()
		+ shaderRecordID;

	float4 color = colors[materialID];

	color *= RayTCurrent() - RayTMin();

	ioPayload.color = color;
}

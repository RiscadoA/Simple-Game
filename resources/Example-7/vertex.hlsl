// This shader was automatically generated from binary bytecode by the D3D11Assembler::Assemble function
// Vertex shader
// DO NOT MODIFY THIS FILE BY HAND

struct ShaderInput
{
	float4 in_32 : IN31IN;
	float4 in_33 : IN32IN;
};

struct ShaderOutput
{
	float4 out_9 : VOUT9VOUT;
	float4 v_pos : SV_POSITION;
};

ShaderOutput VS(ShaderInput input)
{
	ShaderOutput output;
{
{
	output.v_pos = input.in_32;
}
{
	output.out_9 = input.in_33;
}
}
	return output;
}

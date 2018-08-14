#include "D3D11Assembler.h"
#include "../Memory/Endianness.h"

const static mfmU8 mfgD3D11AssemblerMinorVersion = 0x00;

mfgError mfgD3D11WriteType(mfmU8 type, mfsStream* out)
{
	if (out == NULL)
		return MFG_ERROR_INVALID_ARGUMENTS;

	const mfsUTF8CodeUnit* str = NULL;

	if (type == MFG_INT1)
		str = u8"int";
	else if (type == MFG_INT2)
		str = u8"int2";
	else if (type == MFG_INT3)
		str = u8"int3";
	else if (type == MFG_INT4)
		str = u8"int4";
	else if (type == MFG_INT22)
		str = u8"int2x2";
	else if (type == MFG_INT33)
		str = u8"int3x3";
	else if (type == MFG_INT44)
		str = u8"int4x4";

	else if (type == MFG_FLOAT1)
		str = u8"float";
	else if (type == MFG_FLOAT2)
		str = u8"float2";
	else if (type == MFG_FLOAT3)
		str = u8"float3";
	else if (type == MFG_FLOAT4)
		str = u8"float4";
	else if (type == MFG_FLOAT22)
		str = u8"float2x2";
	else if (type == MFG_FLOAT33)
		str = u8"float3x3";
	else if (type == MFG_FLOAT44)
		str = u8"float4x4";

	else return MFG_ERROR_INVALID_DATA;

	mfsError err = mfsPrintFormatUTF8(out, str);
	if (err != MFS_ERROR_OKAY)
		return MFG_ERROR_FAILED_TO_WRITE;
	return MFG_ERROR_OKAY;
}

mfgError mfgD3D11PutID(mfmU16 id, const mfgMetaData* metaData, mfsStream* out)
{
	// Check if it is an input variable
	{
		mfgMetaDataInputVariable* var = metaData->firstInputVar;
		while (var != NULL)
		{
			if (var->id == id)
			{
				if (mfsPrintFormatUTF8(out, u8"input.in_%d", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				return MFG_ERROR_OKAY;
			}
			var = var->next;
		}
	}

	// Check if it is an output variable
	{
		mfgMetaDataOutputVariable* var = metaData->firstOutputVar;
		while (var != NULL)
		{
			if (var->id == id)
			{
				if (mfsPrintFormatUTF8(out, u8"output.out_%d", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				return MFG_ERROR_OKAY;
			}
			var = var->next;
		}
	}

	// Check if it is a binding point
	{
		mfgMetaDataBindingPoint* bp = metaData->firstBindingPoint;
		while (bp != NULL)
		{
			if (bp->type == MFG_CONSTANT_BUFFER)
			{
				mfgMetaDataConstantBuffer* cb = bp;
				mfgMetaDataConstantBufferVariable* var = cb->firstVariable;
				while (var != NULL)
				{
					if (var->id == id)
					{
						if (mfsPrintFormatUTF8(out, u8"buf_%s_%d", bp->name, id) != MFS_ERROR_OKAY)
							return MFG_ERROR_FAILED_TO_WRITE;
						return MFG_ERROR_OKAY;
					}
					var = var->next;
				}
			}
			else if (bp->type == MFG_TEXTURE_1D)
			{
				mfgMetaDataTexture1D* tex = bp;
				if (tex->id == id)
				{
					if (mfsPrintFormatUTF8(out, u8"tex1d_%d", id) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
					return MFG_ERROR_OKAY;
				}
			}
			else if (bp->type == MFG_TEXTURE_2D)
			{
				mfgMetaDataTexture1D* tex = bp;
				if (tex->id == id)
				{
					if (mfsPrintFormatUTF8(out, u8"tex2d_%d", id) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
					return MFG_ERROR_OKAY;
				}
			}
			else if (bp->type == MFG_TEXTURE_3D)
			{
				mfgMetaDataTexture1D* tex = bp;
				if (tex->id == id)
				{
					if (mfsPrintFormatUTF8(out, u8"tex3d_%d", id) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
					return MFG_ERROR_OKAY;
				}
			}

			bp = bp->next;
		}
	}

	if (mfsPrintFormatUTF8(out, u8"local_%d", id) != MFS_ERROR_OKAY)
		return MFG_ERROR_FAILED_TO_WRITE;
	return MFG_ERROR_OKAY;
}

mfgError mfgD3D11Assemble(const mfmU8* bytecode, mfmU64 bytecodeSize, const mfgMetaData* metaData, mfsStream* outputStream)
{
	// Check if the arguments are valid
	if (bytecode == NULL || metaData == NULL || outputStream == NULL)
		return MFG_ERROR_INVALID_ARGUMENTS;

	// Check if the file is valid
	if (bytecodeSize < 6)
		return MFG_ERROR_INVALID_DATA;
	if (bytecode[0] != MFG_BYTECODE_HEADER_MARKER_0 ||
		bytecode[1] != MFG_BYTECODE_HEADER_MARKER_1 ||
		bytecode[2] != MFG_BYTECODE_HEADER_MARKER_2 ||
		bytecode[3] != MFG_BYTECODE_HEADER_MARKER_3)
		return MFG_ERROR_INVALID_DATA;

	// Check version
	if (bytecode[4] != 0x02) // Check if the major version is supported
		return MFG_ERROR_UNSUPPORTED_MAJOR_VER;
	if (bytecode[5] > mfgD3D11AssemblerMinorVersion) // Check if minor version is supported
		return MFG_ERROR_UNSUPPORTED_MINOR_VER;

	// Check shader type
	if (metaData->shaderType == MFG_VERTEX_SHADER)
	{
		const mfsUTF8CodeUnit str[] =
			u8"// This HLSL shader was automatically generated from binary bytecode by the mfgD3D11Assemble function\n"
			u8"// Vertex shader\n"
			u8"// DO NOT MODIFY THIS FILE BY HAND\n\n";
		mfsError err = mfsWrite(outputStream, str, sizeof(str) - 1, NULL);
		if (err != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}
	else if (metaData->shaderType == MFG_PIXEL_SHADER)
	{
		const mfsUTF8CodeUnit str[] =
			u8"// This HLSL shader was automatically generated from binary bytecode by the mfgD3D11Assemble function\n"
			u8"// Pixel shader\n"
			u8"// DO NOT MODIFY THIS FILE BY HAND\n\n";
		mfsError err = mfsWrite(outputStream, str, sizeof(str) - 1, NULL);
		if (err != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}
	else return MFG_ERROR_INVALID_DATA;

	// Add binding points
	{
		mfgMetaDataBindingPoint* bp  = metaData->firstBindingPoint;
		while (bp != NULL)
		{
			if (bp->type == MFG_CONSTANT_BUFFER)
			{
				mfgMetaDataConstantBuffer* cb = bp;
				if (mfsPrintFormatUTF8(outputStream, u8"cbuffer buf_%s\n{\n", bp->name) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;

				mfgMetaDataConstantBufferVariable* var = cb->firstVariable;
				while (var != NULL)
				{
					if (mfsPutByte(outputStream, '\t') != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;

					mfgError err = mfgD3D11WriteType(var->type, outputStream);
					if (err != MFG_ERROR_OKAY)
						return err;

					if (mfsPrintFormatUTF8(outputStream, u8" buf_%s_%d;\n", bp->name, var->id) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;

					var = var->next;
				}

				if (mfsPrintFormatUTF8(outputStream, u8"};\n\n") != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else if (bp->type == MFG_TEXTURE_1D)
			{
				mfgMetaDataTexture1D* tex = bp;
				if (mfsPrintFormatUTF8(outputStream, u8"Texture1D tex1d_%d : register(t%d);\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				if (mfsPrintFormatUTF8(outputStream, u8"SamplerState tex1d_%d_sampler : register(s%d);\n\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else if (bp->type == MFG_TEXTURE_2D)
			{
				mfgMetaDataTexture2D* tex = bp;
				if (mfsPrintFormatUTF8(outputStream, u8"Texture2D tex2d_%d : register(t%d);\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				if (mfsPrintFormatUTF8(outputStream, u8"SamplerState tex2d_%d_sampler : register(s%d);\n\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else if (bp->type == MFG_TEXTURE_3D)
			{
				mfgMetaDataTexture3D* tex = bp;
				if (mfsPrintFormatUTF8(outputStream, u8"Texture3D tex3d_%d : register(t%d);\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				if (mfsPrintFormatUTF8(outputStream, u8"SamplerState tex3d_%d_sampler : register(s%d);\n\n", tex->id, tex->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else
			{
				mfsError err = mfsPrintFormatUTF8(outputStream, u8"// UNSUPPORTED BINDING POINT TYPE '%x'\n\n", bp->type);
				if (err != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}

			bp = bp->next;
		}
	}

	// Add input variables
	{
		if (mfsPrintFormatUTF8(outputStream, u8"struct ShaderInput\n{\n") != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;

		mfgMetaDataInputVariable* var = metaData->firstInputVar;
		while (var != NULL)
		{
			if (mfsPutByte(outputStream, '\t') != MFS_ERROR_OKAY)
				return MFG_ERROR_FAILED_TO_WRITE;

			mfgError err = mfgD3D11WriteType(var->type, outputStream);
			if (err != MFG_ERROR_OKAY)
				return err;

			if (metaData->shaderType == MFG_VERTEX_SHADER)
			{
				if (mfsPrintFormatUTF8(outputStream, u8" in_%d : IN%dIN;\n", var->id, var->id - 1) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else if (metaData->shaderType == MFG_PIXEL_SHADER)
			{
				if (mfsPrintFormatUTF8(outputStream, u8" in_%d : VOUT%dVOUT;\n", var->id, var->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}

			var = var->next;
		}

		if (mfsPrintFormatUTF8(outputStream, u8"};\n\n") != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}

	// Add output variables
	{
		if (mfsPrintFormatUTF8(outputStream, u8"struct ShaderOutput\n{\n") != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;

		mfgMetaDataOutputVariable* var = metaData->firstOutputVar;
		while (var != NULL)
		{
			if (mfsPutByte(outputStream, '\t') != MFS_ERROR_OKAY)
				return MFG_ERROR_FAILED_TO_WRITE;

			mfgError err = mfgD3D11WriteType(var->type, outputStream);
			if (err != MFG_ERROR_OKAY)
				return err;

			if (metaData->shaderType == MFG_VERTEX_SHADER)
			{
				if (var->id == 0)
				{
					if (mfsPrintFormatUTF8(outputStream, u8" out_0 : SV_POSITION;\n") != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				}
				else if (mfsPrintFormatUTF8(outputStream, u8" out_%d : VOUT%dVOUT;\n", var->id, var->id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}
			else if (metaData->shaderType == MFG_PIXEL_SHADER)
			{
				if (var->id == 0)
				{
					if (mfsPrintFormatUTF8(outputStream, u8" out_0 : SV_Depth;\n") != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				}
				else if (mfsPrintFormatUTF8(outputStream, u8" out_%d : SV_Target%dVOUT;\n", var->id, var->id - 1) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
			}

			var = var->next;
		}

		if (mfsPrintFormatUTF8(outputStream, u8"};\n\n") != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}

	// Write entry function
	if (metaData->shaderType == MFG_VERTEX_SHADER)
	{
		const mfsUTF8CodeUnit str[] =
			u8"ShaderOutput VS(ShaderInput input)\n"
			u8"{\n"
			u8"\tShaderOutput output;\n";
		mfsError err = mfsWrite(outputStream, str, sizeof(str) - 1, NULL);
		if (err != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}
	else if (metaData->shaderType == MFG_PIXEL_SHADER)
	{
		const mfsUTF8CodeUnit str[] =
			u8"ShaderOutput PS(ShaderInput input)\n"
			u8"{\n"
			u8"\tShaderOutput output;\n";
		mfsError err = mfsWrite(outputStream, str, sizeof(str) - 1, NULL);
		if (err != MFS_ERROR_OKAY)
			return MFG_ERROR_FAILED_TO_WRITE;
	}

	// Convert bytecode to HLSL
	const mfmU8* it = bytecode + 6;
	mfmU64 tabs = 1;
	while (it < bytecode + bytecodeSize)
	{
		for (mfmU64 i = 0; i < tabs; ++i)
			if (mfsPutByte(outputStream, '\t') != MFS_ERROR_OKAY)
				return MFG_ERROR_FAILED_TO_WRITE;

		switch (*it)
		{
			case MFG_BYTECODE_DECLB1:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"bool local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;

			case MFG_BYTECODE_DECLI1:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI2:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int2 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI3:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int3 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI4:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int4 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI22:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int22 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI33:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int33 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLI44:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int44 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;

			case MFG_BYTECODE_DECLI1A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI2A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int2 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI3A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int3 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI4A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int4 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI22A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int22 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI33A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int33 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLI44A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"int44 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;

			case MFG_BYTECODE_DECLF1:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF2:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int2 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF3:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int3 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF4:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int4 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF22:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int22 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF33:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int33 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;
			case MFG_BYTECODE_DECLF44:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				if (mfsPrintFormatUTF8(outputStream, u8"int44 local_%d;\n", id) != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 3;
			} break;

			case MFG_BYTECODE_DECLF1A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF2A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float2 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF3A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float3 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF4A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float4 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF22A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float22 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF33A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float33 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;
			case MFG_BYTECODE_DECLF44A:
			{
				mfmU16 id = 0;
				mfmFromBigEndian2(it + 1, &id);
				mfmU16 count = 0;
				mfmFromBigEndian2(it + 3, &count);
				for (mfmU16 i = 0; i < count; ++i)
					if (mfsPrintFormatUTF8(outputStream, u8"float44 local_%d;\n", id++) != MFS_ERROR_OKAY)
						return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
			} break;

			case MFG_BYTECODE_ASSIGN:
			{
				mfmU16 id1 = 0;
				mfmFromBigEndian2(it + 1, &id1);
				mfmU16 id2 = 0;
				mfmFromBigEndian2(it + 3, &id2);
				mfgError err = mfgD3D11PutID(id1, metaData, outputStream);
				if (err != MFG_ERROR_OKAY)
					return err;
				if (mfsPrintFormatUTF8(outputStream, u8" = ") != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				err = mfgD3D11PutID(id2, metaData, outputStream);
				if (err != MFG_ERROR_OKAY)
					return err;
				if (mfsPrintFormatUTF8(outputStream, u8";\n") != MFS_ERROR_OKAY)
					return MFG_ERROR_FAILED_TO_WRITE;
				it += 5;
				break;
			}

			default:
				return MFG_ERROR_INVALID_DATA;
				break;
		}
	}
	
	if (mfsPrintFormatUTF8(outputStream, u8"\treturn output;\n}\n") != MFS_ERROR_OKAY)
		return MFG_ERROR_FAILED_TO_WRITE;

	return MFG_ERROR_OKAY;
}

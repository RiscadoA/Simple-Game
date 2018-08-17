#include "OGL4RenderDevice.h"
#include "OGL4Assembler.h"
#include "Config.h"
	
#include "../Memory/PoolAllocator.h"
#include "../String/StringStream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MAGMA_FRAMEWORK_USE_OPENGL

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

typedef struct
{
	mfgRenderDevice base;

	mfiWindow* window;

	void* allocator;

	mfsUTF8CodeUnit errorString[256];
	mfmU64 errorStringSize;

	mfmPoolAllocator* pool;
	mfmU8 poolMemory[280000];

	mfgRasterState* defaultRasterState;
	mfgDepthStencilState* defaultDepthStencilState;
	mfgBlendState* defaultBlendState;
} mfgOGL4RenderDevice;

typedef struct
{
	mfgRenderDeviceObject base;
	GLint program;
	const mfgMetaData* md;
} mfgOGL4Shader;

typedef struct
{
	mfgRenderDeviceObject base;
	GLint pipeline;
} mfgOGL4Pipeline;

typedef struct
{
	mfgRenderDeviceObject base;
	GLboolean cullEnabled;
	GLenum frontFace;
	GLenum cullFace;
	GLenum polygonMode;
} mfgOGL4RasterState;

typedef struct
{
	mfgRenderDeviceObject base;

	GLboolean depthEnabled;
	GLboolean depthWriteEnabled;
	GLfloat depthNear;
	GLfloat depthFar;
	GLenum depthFunc;

	GLuint stencilRef;
	GLboolean stencilEnabled;
	GLuint stencilReadMask;
	GLuint stencilWriteMask;

	GLenum frontStencilFunc;
	GLenum frontFaceStencilFail;
	GLenum frontFaceStencilPass;
	GLenum frontFaceDepthFail;

	GLenum backStencilFunc;
	GLenum backFaceStencilFail;
	GLenum backFaceStencilPass;
	GLenum backFaceDepthFail;
} mfgOGL4DepthStencilState;

typedef struct
{
	mfgRenderDeviceObject base;
	
	GLboolean blendEnabled;
	GLenum srcFactor;
	GLenum dstFactor;
	GLenum blendOp;
	GLenum srcAlphaFactor;
	GLenum dstAlphaFactor;
	GLenum alphaBlendOp;
} mfgOGL4BlendState;

#define MFG_RETURN_ERROR(code, msg) {\
	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;\
	oglRD->errorStringSize = snprintf(oglRD->errorString, sizeof(oglRD->errorString),\
									 msg) + 1;\
	if (oglRD->errorStringSize > sizeof(oglRD->errorString))\
	oglRD->errorStringSize = sizeof(oglRD->errorString);\
	return code; }

#ifdef MAGMA_FRAMEWORK_DEBUG
#define MFG_CHECK_GL_ERROR() do { GLenum err = glGetError();\
if (err != 0) {\
	MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, u8"glGetError() returned non zero on " __FUNCTION__)\
}} while (0)
#else
#define MFG_CHECK_GL_ERROR()
#endif

void mfgOGL4DestroyVertexShader(void* vs)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (vs == NULL) abort();
#endif
	mfgOGL4Shader* oglVS = vs;
	glDeleteProgram(oglVS->program);
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglVS->base.renderDevice)->pool, oglVS) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}

mfgError mfgOGL4CreateVertexShader(mfgRenderDevice* rd, mfgVertexShader** vs, const mfmU8* bytecode, mfmU64 bytecodeSize, const mfgMetaData* metaData)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || vs == NULL || bytecode == NULL || metaData == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	// Allocate vertex shader
	mfgOGL4Shader* oglVS = NULL;
	if (mfmAllocate(oglRD->pool, &oglVS, sizeof(mfgOGL4Shader)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate vertex shader on pool");

	// Init object
	oglVS->base.object.destructorFunc = &mfgOGL4DestroyVertexShader;
	oglVS->base.object.referenceCount = 0;
	oglVS->base.renderDevice = rd;

	// Create string stream
	mfsStream* ss;
	GLchar buffer[4096];
	if (mfsCreateStringStream(&ss, buffer, sizeof(buffer), oglRD->allocator) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, u8"Failed to create string stream for mfgOGL4Assemble");
	mfgError err = mfgOGL4Assemble(bytecode, bytecodeSize, metaData, ss);
	if (err != MFG_ERROR_OKAY)
		MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, u8"mfgOGL4Assemble failed");
	mfsPutByte(ss, '\0');
	mfsDestroyStringStream(ss);

	// Create shader program
	oglVS->md = metaData;
	{
		const GLchar* bufferSrc = buffer;
		oglVS->program = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &bufferSrc);

		GLint success;
		glGetProgramiv(oglVS->program, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetProgramInfoLog(oglVS->program, 512, NULL, infoLog);
			glDeleteProgram(oglVS->program);
			mfsPrintFormatUTF8(mfsOutStream, u8"Failed to compile/link vertex shader, source:\n");
			mfsPrintFormatUTF8(mfsOutStream, bufferSrc);
			mfsPrintFormatUTF8(mfsOutStream, u8"\n");
			mfsFlush(mfsOutStream);
			MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, infoLog);
		}
	}

	*vs = oglVS;

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

void mfgOGL4DestroyPixelShader(void* ps)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (ps == NULL) abort();
#endif
	mfgOGL4Shader* oglPS = ps;
	glDeleteProgram(oglPS->program);
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglPS->base.renderDevice)->pool, oglPS) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}


mfgError mfgOGL4CreatePixelShader(mfgRenderDevice* rd, mfgPixelShader** ps, const mfmU8* bytecode, mfmU64 bytecodeSize, const mfgMetaData* metaData)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || ps == NULL || bytecode == NULL || metaData == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;
	
	// Allocate pixel shader
	mfgOGL4Shader* oglPS = NULL;
	if (mfmAllocate(oglRD->pool, &oglPS, sizeof(mfgOGL4Shader)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate pixel shader on pool");

	// Init object
	oglPS->base.object.destructorFunc = &mfgOGL4DestroyPixelShader;
	oglPS->base.object.referenceCount = 0;
	oglPS->base.renderDevice = rd;

	// Create string stream
	mfsStream* ss;
	GLchar buffer[4096];
	if (mfsCreateStringStream(&ss, buffer, sizeof(buffer), oglRD->allocator) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, u8"Failed to create string stream for mfgOGL4Assemble");
	mfgError err = mfgOGL4Assemble(bytecode, bytecodeSize, metaData, ss);
	if (err != MFG_ERROR_OKAY)
		MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, u8"mfgOGL4Assemble failed");
	mfsPutByte(ss, '\0');
	mfsDestroyStringStream(ss);

	// Create shader program
	oglPS->md = metaData;
	{
		const GLchar* bufferSrc = buffer;
		oglPS->program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &bufferSrc);

		GLint success;
		glGetProgramiv(oglPS->program, GL_LINK_STATUS, &success);
		if (!success)
		{
			GLchar infoLog[512];
			glGetProgramInfoLog(oglPS->program, 512, NULL, infoLog);
			glDeleteProgram(oglPS->program);
			mfsPrintFormatUTF8(mfsOutStream, u8"Failed to compile/link pixel shader, source:\n");
			mfsPrintFormatUTF8(mfsOutStream, bufferSrc);
			mfsPrintFormatUTF8(mfsOutStream, u8"\n");
			mfsFlush(mfsOutStream);
			MFG_RETURN_ERROR(MFG_ERROR_INTERNAL, infoLog);
		}
	}

	*ps = oglPS;

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

void mfgOGL4DestroyPipeline(void* pp)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (pp == NULL) abort();
#endif
	mfgOGL4Pipeline* oglPP = pp;
	glDeleteProgramPipelines(1, &oglPP->pipeline);
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglPP->base.renderDevice)->pool, oglPP) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}


mfgError mfgOGL4CreatePipeline(mfgRenderDevice* rd, mfgPipeline** pp, mfgVertexShader* vs, mfgPixelShader* ps)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || pp == NULL || vs == NULL || ps == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	// Allocate pipeline
	mfgOGL4Pipeline* oglPP = NULL;
	if (mfmAllocate(oglRD->pool, &oglPP, sizeof(mfgOGL4Pipeline)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate pipeline on pool");

	// Init object
	oglPP->base.object.destructorFunc = &mfgOGL4DestroyPipeline;
	oglPP->base.object.referenceCount = 0;
	oglPP->base.renderDevice = rd;

	// Create shader pipeline
	glGenProgramPipelines(1, &oglPP->pipeline);
	glUseProgramStages(oglPP->pipeline, GL_VERTEX_SHADER_BIT, ((mfgOGL4Shader*)vs)->program);
	glUseProgramStages(oglPP->pipeline, GL_FRAGMENT_SHADER_BIT, ((mfgOGL4Shader*)ps)->program);

	*pp = oglPP;

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4ClearColor(mfgRenderDevice* rd, mfmF32 r, mfmF32 g, mfmF32 b, mfmF32 a)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4ClearDepth(mfgRenderDevice* rd, mfmF32 depth)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4ClearStencil(mfgRenderDevice* rd, mfmI32 stencil)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	glClearStencil(stencil);
	glClear(GL_STENCIL_BUFFER_BIT);
	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4SwapBuffers(mfgRenderDevice* rd)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	glfwSwapBuffers((GLFWwindow*)mfiGetGLWindowGLFWHandle(((mfgOGL4RenderDevice*)rd)->window));
	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

void mfgOGL4DestroyRasterState(void* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (state == NULL) abort();
#endif
	mfgOGL4RasterState* oglState = state;
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglState->base.renderDevice)->pool, oglState) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}

mfgError mfgOGL4CreateRasterState(mfgRenderDevice* rd, mfgRasterState** state, const mfgRasterStateDesc* desc)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || state == NULL || desc == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	// Allocate state
	mfgOGL4RasterState* oglState = NULL;
	if (mfmAllocate(oglRD->pool, &oglState, sizeof(mfgOGL4RasterState)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate raster state on pool");

	// Init object
	oglState->base.object.destructorFunc = &mfgOGL4DestroyRasterState;
	oglState->base.object.referenceCount = 0;
	oglState->base.renderDevice = rd;

	*state = oglState;

	// Set properties
	if (desc->cullEnabled == MFM_FALSE)
		oglState->cullEnabled = GL_FALSE;
	else
		oglState->cullEnabled = GL_TRUE;

	switch (desc->frontFace)
	{
		case MFG_CW: oglState->frontFace = GL_CW; break;
		case MFG_CCW: oglState->frontFace = GL_CCW; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->cullFace)
	{
		case MFG_FRONT: oglState->cullFace = GL_FRONT; break;
		case MFG_BACK: oglState->cullFace = GL_BACK; break;
		case MFG_FRONT_AND_BACK: oglState->cullFace = GL_FRONT_AND_BACK; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->rasterMode)
	{
		case MFG_WIREFRAME: oglState->polygonMode = GL_LINE; break;
		case MFG_FILL: oglState->polygonMode = GL_FILL; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4SetRasterState(mfgRenderDevice* rd, mfgRasterState* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	if (state == NULL)
		return mfgOGL4SetRasterState(rd, ((mfgOGL4RenderDevice*)rd)->defaultRasterState);

	mfgOGL4RasterState* oglRS = (mfgOGL4RasterState*)state;

	if (oglRS->cullEnabled)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
	glFrontFace(oglRS->frontFace);
	glCullFace(oglRS->cullFace);
	glPolygonMode(GL_FRONT_AND_BACK, oglRS->polygonMode);

	MFG_CHECK_GL_ERROR();

	return MFG_ERROR_OKAY;
}

void mfgOGL4DestroyDepthStencilState(void* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (state == NULL) abort();
#endif
	mfgOGL4DepthStencilState* oglState = state;
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglState->base.renderDevice)->pool, oglState) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}

mfgError mfgOGL4CreateDepthStencilState(mfgRenderDevice* rd, mfgDepthStencilState** state, const mfgDepthStencilStateDesc* desc)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || state == NULL || desc == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	// Allocate state
	mfgOGL4DepthStencilState* oglState = NULL;
	if (mfmAllocate(oglRD->pool, &oglState, sizeof(mfgOGL4DepthStencilState)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate depth stencil state on pool");

	// Init object
	oglState->base.object.destructorFunc = &mfgOGL4DestroyDepthStencilState;
	oglState->base.object.referenceCount = 0;
	oglState->base.renderDevice = rd;

	// Set properties
	oglState->depthEnabled = desc->depthEnabled;
	oglState->depthWriteEnabled = desc->depthWriteEnabled;
	oglState->depthNear = desc->depthNear;
	oglState->depthFar = desc->depthFar;

	switch (desc->depthCompare)
	{
		case MFG_NEVER: oglState->depthFunc = GL_NEVER; break;
		case MFG_LESS: oglState->depthFunc = GL_LESS; break;
		case MFG_LEQUAL: oglState->depthFunc = GL_LEQUAL; break;
		case MFG_GREATER: oglState->depthFunc = GL_GREATER; break;
		case MFG_GEQUAL: oglState->depthFunc = GL_GEQUAL; break;
		case MFG_EQUAL: oglState->depthFunc = GL_EQUAL; break;
		case MFG_NEQUAL: oglState->depthFunc = GL_NOTEQUAL; break;
		case MFG_ALWAYS: oglState->depthFunc = GL_ALWAYS; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	oglState->stencilEnabled = desc->stencilEnabled;
	oglState->stencilRef = desc->stencilRef;
	oglState->stencilReadMask = desc->stencilReadMask;
	oglState->stencilWriteMask = desc->stencilWriteMask;

	switch (desc->frontFaceStencilCompare)
	{
		case MFG_NEVER: oglState->frontStencilFunc = GL_NEVER; break;
		case MFG_LESS: oglState->frontStencilFunc = GL_LESS; break;
		case MFG_LEQUAL: oglState->frontStencilFunc = GL_LEQUAL; break;
		case MFG_GREATER: oglState->frontStencilFunc = GL_GREATER; break;
		case MFG_GEQUAL: oglState->frontStencilFunc = GL_GEQUAL; break;
		case MFG_EQUAL: oglState->frontStencilFunc = GL_EQUAL; break;
		case MFG_NEQUAL: oglState->frontStencilFunc = GL_NOTEQUAL; break;
		case MFG_ALWAYS: oglState->frontStencilFunc = GL_ALWAYS; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->frontFaceStencilFail)
	{
		case MFG_KEEP: oglState->frontFaceStencilFail = GL_KEEP; break;
		case MFG_ZERO: oglState->frontFaceStencilFail = GL_ZERO; break;
		case MFG_REPLACE: oglState->frontFaceStencilFail = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->frontFaceStencilFail = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->frontFaceStencilFail = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->frontFaceStencilFail = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->frontFaceStencilFail = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->frontFaceStencilFail = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->frontFaceStencilPass)
	{
		case MFG_KEEP: oglState->frontFaceStencilPass = GL_KEEP; break;
		case MFG_ZERO: oglState->frontFaceStencilPass = GL_ZERO; break;
		case MFG_REPLACE: oglState->frontFaceStencilPass = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->frontFaceStencilPass = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->frontFaceStencilPass = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->frontFaceStencilPass = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->frontFaceStencilPass = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->frontFaceStencilPass = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->frontFaceDepthFail)
	{
		case MFG_KEEP: oglState->frontFaceDepthFail = GL_KEEP; break;
		case MFG_ZERO: oglState->frontFaceDepthFail = GL_ZERO; break;
		case MFG_REPLACE: oglState->frontFaceDepthFail = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->frontFaceDepthFail = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->frontFaceDepthFail = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->frontFaceDepthFail = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->frontFaceDepthFail = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->frontFaceDepthFail = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->backFaceStencilCompare)
	{
		case MFG_NEVER: oglState->backStencilFunc = GL_NEVER; break;
		case MFG_LESS: oglState->backStencilFunc = GL_LESS; break;
		case MFG_LEQUAL: oglState->backStencilFunc = GL_LEQUAL; break;
		case MFG_GREATER: oglState->backStencilFunc = GL_GREATER; break;
		case MFG_GEQUAL: oglState->backStencilFunc = GL_GEQUAL; break;
		case MFG_EQUAL: oglState->backStencilFunc = GL_EQUAL; break;
		case MFG_NEQUAL: oglState->backStencilFunc = GL_NOTEQUAL; break;
		case MFG_ALWAYS: oglState->backStencilFunc = GL_ALWAYS; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->backFaceStencilFail)
	{
		case MFG_KEEP: oglState->backFaceStencilFail = GL_KEEP; break;
		case MFG_ZERO: oglState->backFaceStencilFail = GL_ZERO; break;
		case MFG_REPLACE: oglState->backFaceStencilFail = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->backFaceStencilFail = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->backFaceStencilFail = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->backFaceStencilFail = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->backFaceStencilFail = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->backFaceStencilFail = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->backFaceStencilPass)
	{
		case MFG_KEEP: oglState->backFaceStencilPass = GL_KEEP; break;
		case MFG_ZERO: oglState->backFaceStencilPass = GL_ZERO; break;
		case MFG_REPLACE: oglState->backFaceStencilPass = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->backFaceStencilPass = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->backFaceStencilPass = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->backFaceStencilPass = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->backFaceStencilPass = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->backFaceStencilPass = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->backFaceDepthFail)
	{
		case MFG_KEEP: oglState->backFaceDepthFail = GL_KEEP; break;
		case MFG_ZERO: oglState->backFaceDepthFail = GL_ZERO; break;
		case MFG_REPLACE: oglState->backFaceDepthFail = GL_REPLACE; break;
		case MFG_INCREMENT: oglState->backFaceDepthFail = GL_INCR; break;
		case MFG_INCREMENT_WRAP: oglState->backFaceDepthFail = GL_INCR_WRAP; break;
		case MFG_DECREMENT: oglState->backFaceDepthFail = GL_DECR; break;
		case MFG_DECREMENT_WRAP: oglState->backFaceDepthFail = GL_DECR_WRAP; break;
		case MFG_INVERT: oglState->backFaceDepthFail = GL_INVERT; break;
		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	*state = oglState;

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4SetDepthStencilState(mfgRenderDevice* rd, mfgDepthStencilState* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	if (state == NULL)
		return mfgOGL4SetDepthStencilState(rd, ((mfgOGL4RenderDevice*)rd)->defaultDepthStencilState);

	mfgOGL4DepthStencilState* oglDSS = (mfgOGL4DepthStencilState*)state;

	if (oglDSS->depthEnabled)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	
	glDepthFunc(oglDSS->depthFunc);
	glDepthMask(oglDSS->depthWriteEnabled ? GL_TRUE : GL_FALSE);
	glDepthRange(oglDSS->depthNear, oglDSS->depthFar);

	if (oglDSS->stencilEnabled)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);

	glStencilFuncSeparate(GL_FRONT, oglDSS->frontStencilFunc, oglDSS->stencilRef, oglDSS->stencilReadMask);
	glStencilMaskSeparate(GL_FRONT, oglDSS->stencilWriteMask);
	glStencilOpSeparate(GL_FRONT, oglDSS->frontFaceStencilFail, oglDSS->frontFaceDepthFail, oglDSS->frontFaceStencilPass);

	glStencilFuncSeparate(GL_BACK, oglDSS->backStencilFunc, oglDSS->stencilRef, oglDSS->stencilReadMask);
	glStencilMaskSeparate(GL_BACK, oglDSS->stencilWriteMask);
	glStencilOpSeparate(GL_BACK, oglDSS->backFaceStencilFail, oglDSS->backFaceDepthFail, oglDSS->backFaceStencilPass);

	MFG_CHECK_GL_ERROR();

	return MFG_ERROR_OKAY;
}

void mfgOGL4DestroyBlendState(void* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	if (state == NULL) abort();
#endif
	mfgOGL4BlendState* oglState = state;
	if (mfmDeallocate(((mfgOGL4RenderDevice*)oglState->base.renderDevice)->pool, oglState) != MFM_ERROR_OKAY)
		abort();
#ifdef MAGMA_FRAMEWORK_DEBUG
	GLenum err = glGetError();
	if (err != 0)
		abort();
#endif
}

mfgError mfgOGL4CreateBlendState(mfgRenderDevice* rd, mfgBlendState** state, const mfgBlendStateDesc* desc)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL || state == NULL || desc == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	// Allocate state
	mfgOGL4BlendState* oglState = NULL;
	if (mfmAllocate(oglRD->pool, &oglState, sizeof(mfgOGL4BlendState)) != MFM_ERROR_OKAY)
		MFG_RETURN_ERROR(MFM_ERROR_ALLOCATION_FAILED, u8"Failed to allocate raster state on pool");

	// Init object
	oglState->base.object.destructorFunc = &mfgOGL4DestroyRasterState;
	oglState->base.object.referenceCount = 0;
	oglState->base.renderDevice = rd;

	// Set properties
	oglState->blendEnabled = desc->blendEnabled;

	switch (desc->sourceFactor)
	{
		case MFG_ZERO: oglState->srcFactor = GL_ZERO; break;
		case MFG_ONE: oglState->srcFactor = GL_ONE; break;
		case MFG_SRC_COLOR: oglState->srcFactor = GL_SRC_COLOR; break;
		case MFG_INV_SRC_COLOR: oglState->srcFactor = GL_ONE_MINUS_SRC_COLOR; break;
		case MFG_DST_COLOR: oglState->srcFactor = GL_DST_COLOR; break;
		case MFG_INV_DST_COLOR: oglState->srcFactor = GL_ONE_MINUS_DST_COLOR; break;
		case MFG_SRC_ALPHA: oglState->srcFactor = GL_SRC_ALPHA; break;
		case MFG_INV_SRC_ALPHA: oglState->srcFactor = GL_ONE_MINUS_SRC_ALPHA; break;
		case MFG_DST_ALPHA: oglState->srcFactor = GL_DST_ALPHA; break;
		case MFG_INV_DST_ALPHA: oglState->srcFactor = GL_ONE_MINUS_DST_ALPHA; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->destinationFactor)
	{
		case MFG_ZERO: oglState->dstFactor = GL_ZERO; break;
		case MFG_ONE: oglState->dstFactor = GL_ONE; break;
		case MFG_SRC_COLOR: oglState->dstFactor = GL_SRC_COLOR; break;
		case MFG_INV_SRC_COLOR: oglState->dstFactor = GL_ONE_MINUS_SRC_COLOR; break;
		case MFG_DST_COLOR: oglState->dstFactor = GL_DST_COLOR; break;
		case MFG_INV_DST_COLOR: oglState->dstFactor = GL_ONE_MINUS_DST_COLOR; break;
		case MFG_SRC_ALPHA: oglState->dstFactor = GL_SRC_ALPHA; break;
		case MFG_INV_SRC_ALPHA: oglState->dstFactor = GL_ONE_MINUS_SRC_ALPHA; break;
		case MFG_DST_ALPHA: oglState->dstFactor = GL_DST_ALPHA; break;
		case MFG_INV_DST_ALPHA: oglState->dstFactor = GL_ONE_MINUS_DST_ALPHA; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->blendOperation)
	{
		case MFG_ADD: oglState->blendOp = GL_FUNC_ADD; break;
		case MFG_SUBTRACT: oglState->blendOp = GL_FUNC_SUBTRACT; break;
		case MFG_REV_SUBTRACT: oglState->blendOp = GL_FUNC_REVERSE_SUBTRACT; break;
		case MFG_MIN: oglState->blendOp = GL_MIN; break;
		case MFG_MAX: oglState->blendOp = GL_MAX; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->sourceAlphaFactor)
	{
		case MFG_ZERO: oglState->srcAlphaFactor = GL_ZERO; break;
		case MFG_ONE: oglState->srcAlphaFactor = GL_ONE; break;
		case MFG_SRC_COLOR: oglState->srcAlphaFactor = GL_SRC_COLOR; break;
		case MFG_INV_SRC_COLOR: oglState->srcAlphaFactor = GL_ONE_MINUS_SRC_COLOR; break;
		case MFG_DST_COLOR: oglState->srcAlphaFactor = GL_DST_COLOR; break;
		case MFG_INV_DST_COLOR: oglState->srcAlphaFactor = GL_ONE_MINUS_DST_COLOR; break;
		case MFG_SRC_ALPHA: oglState->srcAlphaFactor = GL_SRC_ALPHA; break;
		case MFG_INV_SRC_ALPHA: oglState->srcAlphaFactor = GL_ONE_MINUS_SRC_ALPHA; break;
		case MFG_DST_ALPHA: oglState->srcAlphaFactor = GL_DST_ALPHA; break;
		case MFG_INV_DST_ALPHA: oglState->srcAlphaFactor = GL_ONE_MINUS_DST_ALPHA; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->destinationAlphaFactor)
	{
		case MFG_ZERO: oglState->dstAlphaFactor = GL_ZERO; break;
		case MFG_ONE: oglState->dstAlphaFactor = GL_ONE; break;
		case MFG_SRC_COLOR: oglState->dstAlphaFactor = GL_SRC_COLOR; break;
		case MFG_INV_SRC_COLOR: oglState->dstAlphaFactor = GL_ONE_MINUS_SRC_COLOR; break;
		case MFG_DST_COLOR: oglState->dstAlphaFactor = GL_DST_COLOR; break;
		case MFG_INV_DST_COLOR: oglState->dstAlphaFactor = GL_ONE_MINUS_DST_COLOR; break;
		case MFG_SRC_ALPHA: oglState->dstAlphaFactor = GL_SRC_ALPHA; break;
		case MFG_INV_SRC_ALPHA: oglState->dstAlphaFactor = GL_ONE_MINUS_SRC_ALPHA; break;
		case MFG_DST_ALPHA: oglState->dstAlphaFactor = GL_DST_ALPHA; break;
		case MFG_INV_DST_ALPHA: oglState->dstAlphaFactor = GL_ONE_MINUS_DST_ALPHA; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	switch (desc->blendAlphaOperation)
	{
		case MFG_ADD: oglState->alphaBlendOp = GL_FUNC_ADD; break;
		case MFG_SUBTRACT: oglState->alphaBlendOp = GL_FUNC_SUBTRACT; break;
		case MFG_REV_SUBTRACT: oglState->alphaBlendOp = GL_FUNC_REVERSE_SUBTRACT; break;
		case MFG_MIN: oglState->alphaBlendOp = GL_MIN; break;
		case MFG_MAX: oglState->alphaBlendOp = GL_MAX; break;

		default: return MFG_ERROR_INVALID_ARGUMENTS;
	}

	*state = oglState;

	MFG_CHECK_GL_ERROR();
	return MFG_ERROR_OKAY;
}

mfgError mfgOGL4SetBlendState(mfgRenderDevice* rd, mfgBlendState* state)
{
#ifdef MAGMA_FRAMEWORK_DEBUG
	{ if (rd == NULL) return MFG_ERROR_INVALID_ARGUMENTS; }
#endif
	if (state == NULL)
		return mfgOGL4SetBlendState(rd, ((mfgOGL4RenderDevice*)rd)->defaultBlendState);

	mfgOGL4BlendState* oglBS = (mfgOGL4BlendState*)state;
	
	if (oglBS->blendEnabled)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	glBlendFuncSeparate(oglBS->srcFactor,
						oglBS->dstFactor,
						oglBS->srcAlphaFactor,
						oglBS->dstAlphaFactor);

	glBlendEquationSeparate(oglBS->blendOp,
							oglBS->alphaBlendOp);

	MFG_CHECK_GL_ERROR();

	return MFG_ERROR_OKAY;
}

mfmBool mfgOGL4GetErrorString(mfgRenderDevice* rd, mfsUTF8CodeUnit* str, mfmU64 maxSize)
{
	if (rd == NULL || str == NULL || maxSize == 0)
		abort();

	mfgOGL4RenderDevice* oglRD = (mfgOGL4RenderDevice*)rd;

	if (oglRD->errorStringSize == 0)
		return MFM_FALSE;

	if (maxSize >= oglRD->errorStringSize)
		memcpy(str, oglRD->errorString, oglRD->errorStringSize);
	else
	{
		memcpy(str, oglRD->errorString, maxSize - 1);
		str[maxSize] = '\0';
	}

	return MFM_TRUE;
}

mfgError mfgCreateOGL4RenderDevice(mfgRenderDevice ** renderDevice, mfiWindow* window, const mfgRenderDeviceDesc * desc, void * allocator)
{
	// Check if params are valid
	if (renderDevice == NULL || window == NULL || desc == NULL || window->type != MFI_OGLWINDOW)
		return MFG_ERROR_INVALID_ARGUMENTS;
	
	// Allocate render device
	mfgOGL4RenderDevice* rd;
	if (mfmAllocate(allocator, &rd, sizeof(mfgOGL4RenderDevice)) != MFM_ERROR_OKAY)
		return MFG_ERROR_ALLOCATION_FAILED;

	// Create pool allocator
	{
		mfmPoolAllocatorDesc desc;
		desc.expandable = MFM_FALSE;
		desc.slotCount = 2048;
		desc.slotSize = 96;
		mfmError err = mfmCreatePoolAllocatorOnMemory(&rd->pool, &desc, rd->poolMemory, sizeof(rd->poolMemory));
		if (err != MFM_ERROR_OKAY)
			return MFG_ERROR_ALLOCATION_FAILED;
	}

	// Initialize some properties
	rd->base.object.destructorFunc = &mfgDestroyOGL4RenderDevice;
	rd->base.object.referenceCount = 0;
	rd->window = window;
	rd->allocator = allocator;
	memset(rd->errorString, '\0', sizeof(rd->errorString));
	rd->errorStringSize = 0;

	// Init context
	glfwMakeContextCurrent((GLFWwindow*)mfiGetGLWindowGLFWHandle(((mfgOGL4RenderDevice*)rd)->window));
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		mfsPrintFormatUTF8(mfsErrStream, u8"Failed to create OGL4RenderDevice:\nFailed to init GLEW:\n%s", (const mfsUTF8CodeUnit*)glewGetErrorString(err));
		return MFS_ERROR_INTERNAL;
	}

	// Check extensions
	if (!GLEW_EXT_texture_filter_anisotropic)
		return MFG_ERROR_NO_EXTENSION;

	// Enable or disable VSync
	if (desc->vsyncEnabled == MFM_TRUE)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);

	// Set functions

	rd->base.createVertexShader = &mfgOGL4CreateVertexShader;
	rd->base.destroyVertexShader = &mfgOGL4DestroyVertexShader;
	rd->base.createPixelShader = &mfgOGL4CreatePixelShader;
	rd->base.destroyPixelShader = &mfgOGL4DestroyPixelShader;
	rd->base.createPipeline = &mfgOGL4CreatePipeline;
	rd->base.destroyPipeline = &mfgOGL4DestroyPipeline;

	rd->base.createRasterState = &mfgOGL4CreateRasterState;
	rd->base.destroyRasterState = &mfgOGL4DestroyRasterState;
	rd->base.setRasterState = &mfgOGL4SetRasterState;
	rd->base.createDepthStencilState = &mfgOGL4CreateDepthStencilState;
	rd->base.destroyDepthStencilState = &mfgOGL4DestroyDepthStencilState;
	rd->base.setDepthStencilState = &mfgOGL4SetDepthStencilState;
	rd->base.createBlendState = &mfgOGL4CreateBlendState;
	rd->base.destroyBlendState = &mfgOGL4DestroyBlendState;
	rd->base.setBlendState = &mfgOGL4SetBlendState;

	rd->base.clearColor = &mfgOGL4ClearColor;
	rd->base.clearDepth = &mfgOGL4ClearDepth;
	rd->base.clearStencil = &mfgOGL4ClearStencil;
	rd->base.swapBuffers = &mfgOGL4SwapBuffers;

	rd->base.getErrorString = &mfgOGL4GetErrorString;

	// Get and set the default raster state
	{
		mfgRasterStateDesc desc;
		mfgDefaultRasterStateDesc(&desc);
		mfgError err = mfgCreateRasterState(rd, &rd->defaultRasterState, &desc);
		if (err != MFG_ERROR_OKAY)
			return err;
		err = mfgSetRasterState(rd, rd->defaultRasterState);
		if (err != MFG_ERROR_OKAY)
			return err;
	}

	// Get and set the default depth stencil state
	{
		mfgDepthStencilStateDesc desc;
		mfgDefaultDepthStencilStateDesc(&desc);
		mfgError err = mfgCreateDepthStencilState(rd, &rd->defaultDepthStencilState, &desc);
		if (err != MFG_ERROR_OKAY)
			return err;
		err = mfgSetDepthStencilState(rd, rd->defaultDepthStencilState);
		if (err != MFG_ERROR_OKAY)
			return err;
	}

	// Get and set the default blend state
	{
		mfgBlendStateDesc desc;
		mfgDefaultBlendStateDesc(&desc);
		mfgError err = mfgCreateBlendState(rd, &rd->defaultBlendState, &desc);
		if (err != MFG_ERROR_OKAY)
			return err;
		err = mfgSetBlendState(rd, rd->defaultBlendState);
		if (err != MFG_ERROR_OKAY)
			return err;
	}

	// Successfully inited render device
	*renderDevice = rd;
	return MFG_ERROR_OKAY;
}

void mfgDestroyOGL4RenderDevice(void * renderDevice)
{
	if (renderDevice == NULL)
		abort();

	mfgOGL4RenderDevice* rd = (mfgOGL4RenderDevice*)renderDevice;

	// Destroy default states
	mfgDestroyRasterState(rd->defaultRasterState);
	mfgDestroyDepthStencilState(rd->defaultDepthStencilState);
	mfgDestroyBlendState(rd->defaultBlendState);

	// Destroy pool allocator
	mfmDestroyPoolAllocator(rd->pool);

	// Deallocate render device
	if (mfmDeallocate(rd->allocator, rd) != MFM_ERROR_OKAY)
		abort();
}

#else

mfgError mfgCreateOGL4RenderDevice(mfgRenderDevice ** renderDevice, mfiWindow* window, const mfgRenderDeviceDesc * desc, void * allocator)
{
	return MFG_ERROR_NOT_SUPPORTED;
}

void mfgDestroyOGL4RenderDevice(void * renderDevice)
{
	abort();
}

#endif
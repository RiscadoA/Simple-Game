#include "Entry.h"
#include "Config.h"

#include "String/Stream.h"
#include "File/Archive.h"
#include "Input/Window.h"
#include "Input/OGLWindow.h"
#include "Input/D3DWindow.h"

#include "Graphics/2.X/RenderDevice.h"
#include "Graphics/2.X/OGL4RenderDevice.h"
#include "Graphics/2.X/D3D11RenderDevice.h"

static mfmBool mfInitialized = MFM_FALSE;

mfError mfInit(int argc, const char** argv)
{
	if (mfInitialized == MFM_TRUE)
		return MF_ERROR_ALREADY_INITIALIZED;
	mfError err;

	// Init stream library
	err = mfsInitStream();
	if (err != MF_ERROR_OKAY)
		return err;

	// Init files library
	err = mffInitArchives();
	if (err != MF_ERROR_OKAY)
		return err;

	// Init windows library
	err = mfiInitWindows();
	if (err != MF_ERROR_OKAY)
		return err;

	// Register windows types
#ifdef MAGMA_FRAMEWORK_USE_OPENGL
	err = mfiRegisterWindowCreator(MFI_OGLWINDOW_TYPE_NAME, &mfiCreateOGLWindow);
	if (err != MF_ERROR_OKAY)
		return err;
#endif

#ifdef MAGMA_FRAMEWORK_USE_DIRECTX
	err = mfiRegisterWindowCreator(MFI_D3DWINDOW_TYPE_NAME, &mfiCreateD3DWindow);
	if (err != MF_ERROR_OKAY)
		return err;
#endif

	// Register render device types
#ifdef MAGMA_FRAMEWORK_USE_OPENGL
	err = mfgV2XRegisterRenderDeviceCreator(MFG_OGL4RENDERDEVICE_TYPE_NAME, &mfgV2XCreateOGL4RenderDevice);
	if (err != MF_ERROR_OKAY)
		return err;
#endif

#ifdef MAGMA_FRAMEWORK_USE_DIRECTX
	err = mfgV2XRegisterRenderDeviceCreator(MFG_D3D11RENDERDEVICE_TYPE_NAME, &mfgV2XCreateD3D11RenderDevice);
	if (err != MF_ERROR_OKAY)
		return err;
#endif

	mfInitialized = MFM_TRUE;

	return MF_ERROR_OKAY;
}

void mfTerminate()
{
	if (mfInitialized == MFM_FALSE)
		return;
	mfError err;

	// Terminate windows library
	mfiTerminateWindows();

	// Terminate files library
	mffTerminateArchives();

	// Terminate stream library
	err = mfsTerminateStream();
	if (err != MF_ERROR_OKAY)
		abort();

	mfInitialized = MFM_FALSE;
}

mfmBool mfIsInitialized()
{
	return mfInitialized;
}
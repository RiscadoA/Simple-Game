#pragma once

#include "RenderDevice.h"

#include "../Input/D3DWindow.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/// <summary>
	///		Creates a new Direct3D 11 render device.
	/// </summary>
	/// <param name="renderDevice">Pointer to render device handle</param>
	/// <param name="window">Window handle</param>
	/// <param name="desc">Render device description</param>
	/// <param name="allocator">Allocator to use on allocations</param>
	/// <returns>
	///		MFG_ERROR_OKAY if there were no errors.
	///		Otherwise returns an error code.
	/// </returns>
	mfError mfgCreateD3D11RenderDevice(mfgRenderDevice** renderDevice, mfiWindow* window, const mfgRenderDeviceDesc* desc, void* allocator);

	/// <summary>
	///		Destroys a Direct3D 11 render device.
	/// </summary>
	/// <param name="renderDevice">Render device handle</param>
	void mfgDestroyD3D11RenderDevice(void* renderDevice);

#ifdef __cplusplus
}
#endif

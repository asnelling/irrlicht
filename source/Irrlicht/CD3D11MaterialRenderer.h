
#ifndef __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_
#define WIN32_LEAN_AND_MEAN
#endif
#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_
#include <d3d11.h>
#include <D3Dcompiler.h>

#include "IMaterialRenderer.h"
#include "CD3D11Driver.h"

namespace irr
{

namespace video
{

class CD3D11MaterialRenderer : public IMaterialRenderer
{
public:
	CD3D11MaterialRenderer(ID3D11Device* device, IVideoDriver* driver, IMaterialRenderer* base)
		: Driver((CD3D11Driver*)driver), Device(device) , ImmediateContext(0), BaseRenderer(base)
	{
		if (Device)
		{
			Device->AddRef();
			Device->GetImmediateContext( &ImmediateContext );
		}	

		if (BaseRenderer)
			BaseRenderer->grab();
	}

	virtual ~CD3D11MaterialRenderer()
	{
		if (BaseRenderer)
			BaseRenderer->drop();

		SAFE_RELEASE(ImmediateContext);
		SAFE_RELEASE(Device);
	}

	virtual s32 getVariableID(bool vertexShader, const c8* name)
	{
		return -1;
	}

	//! sets a variable in the shader.
	//! \param vertexShader: True if this should be set in the vertex shader, false if
	//! in the pixel shader.
	//! \param index: Index of the variable
	//! \param floats: Pointer to array of floats
	//! \param count: Amount of floats in array.
	virtual bool setVariable(bool vertexShader, s32 index, const f32* floats, int count)
	{
		return false;
	}

	//! Int interface for the above.
	virtual bool setVariable(bool vertexShader, s32 index, const s32* ints, int count)
	{
		return false;
	}

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		return false;
	}

	//! get shader signature
	virtual void* getShaderByteCode() const = 0;

	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const = 0;

	inline D3DXVECTOR4 colorToD3DXVECTOR4(video::SColor& clr)
	{
		const float cf255 = 255.0f;
		const float r = (float)clr.getRed() / cf255;
		const float g = (float)clr.getGreen() / cf255;
		const float b = (float)clr.getBlue() / cf255;
		const float a = (float)clr.getAlpha() / cf255;
		return D3DXVECTOR4(r,g,b,a);
	}

	inline D3DXVECTOR4 colorToD3DXVECTOR4(const video::SColor& clr)
	{
		const float cf255 = 255.0f;
		const float r = (float)clr.getRed() / cf255;
		const float g = (float)clr.getGreen() / cf255;
		const float b = (float)clr.getBlue() / cf255;
		const float a = (float)clr.getAlpha() / cf255;
		return D3DXVECTOR4(r,g,b,a);
	}

protected:
	// DX 11 objects
	ID3D11Device* Device;
	ID3D11DeviceContext* ImmediateContext;

	// Irrlicht objects
	CD3D11Driver* Driver;
	IMaterialRenderer* BaseRenderer;

	SMaterial CurrentMaterial;
};
}
}

#endif
#endif
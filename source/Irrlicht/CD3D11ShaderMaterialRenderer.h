
#ifndef __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_
#include <d3d11.h>

#include "IMaterialRenderer.h"

namespace irr
{

namespace video
{
class IVideoDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;

class CD3D11ShaderMaterialRenderer : public IMaterialRenderer
{
public:
	CD3D11ShaderMaterialRenderer(ID3D11Device* device, IVideoDriver* driver, IShaderConstantSetCallBack* callback, IMaterialRenderer* base, s32 userData = 0);

	virtual ~CD3D11ShaderMaterialRenderer();

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	virtual void OnSetMaterial( const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services );

	virtual void OnUnsetMaterial();

	virtual bool isTransparent() const;

	//! sets a variable in the shader.
	//! \param name: Name of the variable
	//! \param floats: Pointer to array of floats
	//! \param count: Amount of floats in array.
	virtual bool setVariable(s32 index, const f32* floats, int count)
	{
		return false;
	}

	virtual bool setVariable(s32 index, const s32* ints, int count)
	{
		return false;
	}

	virtual s32 getVariableID( bool vertex, const c8* name ) 
	{
		return -1;
	}

	//! get shader signature
	virtual void* getShaderByteCode() const = 0;
	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const = 0;

protected:

	// DX 11 objects
	ID3D11Device* Device;
	ID3D11DeviceContext* Context;

	IShaderConstantSetCallBack* CallBack;
	// Irrlicht objects
	IVideoDriver* Driver;
	IMaterialRenderer* BaseMaterial;

	SMaterial CurrentMaterial;

	s32 UserData;
};
}
}

#endif
#endif
#endif
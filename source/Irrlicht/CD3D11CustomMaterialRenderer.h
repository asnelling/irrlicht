// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D11_MATERIAL_RENDERER_CUSTOM_H_INCLUDED__
#define __C_D3D11_MATERIAL_RENDERER_CUSTOM_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "CD3D11MaterialRenderer.h"
#include "IGPUProgrammingServices.h"

#include <d3dx11effect.h>

namespace irr
{
namespace video
{

class IVideoDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;

class CD3D11VertexDeclaration;

//! Class for using vertex and pixel shaders via HLSL with D3D11
class CD3D11CustomMaterialRenderer : public CD3D11MaterialRenderer
{
public:

	//! Public constructor
	CD3D11CustomMaterialRenderer(ID3D11Device* d3ddev, video::IVideoDriver* driver, s32& outMaterialTypeNr,
		const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8* geometryShaderProgram, const c8* geometryShaderEntryPointName, E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 verticesOut, E_VERTEX_TYPE vertexTypeOut,				// Only for DirectX 11
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData);

	//! Destructor
	virtual ~CD3D11CustomMaterialRenderer();

	//! sets a variable in the shader.
	//! \param name: Name of the variable
	//! \param floats: Pointer to array of floats
	//! \param count: Amount of floats in array.
	virtual bool setVariable(const c8* name, const f32* floats, int count);
	virtual bool setVariable(const c8* name, const s32* ints, int count);

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services);

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	/** Called during the IVideoDriver::setMaterial() call before the new
	material will get the OnSetMaterial() call. */
	virtual void OnUnsetMaterial();

	//! get shader signature
	virtual void* getShaderByteCode() const;

	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const;

	virtual bool isTransparent() const;

protected:
	ID3DX11Effect* Effect;
	ID3DX11EffectTechnique* Technique;
	D3DX11_PASS_DESC PassDescription;

	IShaderConstantSetCallBack* CallBack;
	s32 UserData;
	bool isStreamOutput;

	bool init(const core::stringc vertexShaderProgram,
			const c8* vertexShaderEntryPointName,
			E_VERTEX_SHADER_TYPE vsCompileTarget,
			const core::stringc pixelShaderProgram,
			const c8* pixelShaderEntryPointName,
			E_PIXEL_SHADER_TYPE psCompileTarget,
			const core::stringc geometryShaderProgram,
			const c8* geometryShaderEntryPointName,
			E_GEOMETRY_SHADER_TYPE gsCompileTarget,
			scene::E_PRIMITIVE_TYPE inType,				// Only for OpenGL
			scene::E_PRIMITIVE_TYPE outType,			// Only for OpenGL
			u32 verticesOut,							// Only for OpenGL
			E_VERTEX_TYPE vertexTypeOut);

	core::stringc parseStreamOutputDeclaration(CD3D11VertexDeclaration* declaration);
};

} // end namespace video
} // end namespace irr

#endif
#endif
#endif
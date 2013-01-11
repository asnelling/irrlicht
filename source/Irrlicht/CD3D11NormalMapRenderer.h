// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D11_NORMAL_MAPMATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D11_NORMAL_MAPMATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_
#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#include "irrMath.h"    // needed by borland for sqrtf define
#endif 

#include <d3dx11effect.h>

#include "CD3D11MaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"

namespace irr
{
namespace video
{

//! Renderer for normal maps
class CD3D11NormalMapRenderer : public CD3D11ShaderMaterialRenderer, IShaderConstantSetCallBack
{
public:

	CD3D11NormalMapRenderer(ID3D11Device* device, video::IVideoDriver* driver, s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial);
	~CD3D11NormalMapRenderer();

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	virtual void OnSetConstants( IMaterialRendererServices* services, s32 userData );

	//! Returns the render capability of the material.
	virtual s32 getRenderCapability() const;

	//! get shader signature
	virtual void* getShaderByteCode() const;

	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const;

	virtual void OnSetMaterial( const SMaterial& material );	

protected:
	bool init( const char* shader);

private:

	// Effects and techniques
	ID3DX11Effect* Effect;
	ID3DX11EffectTechnique* Technique;
	D3DX11_PASS_DESC PassDescription;

	// Transformation matrix variables
	ID3DX11EffectMatrixVariable* WorldMatrix;
	ID3DX11EffectMatrixVariable* WorldViewProjMatrix;
	ID3DX11EffectVectorVariable* LightPos1;
	ID3DX11EffectVectorVariable* LightColor1;
	ID3DX11EffectVectorVariable* LightPos2;
	ID3DX11EffectVectorVariable* LightColor2;
};

} // end namespace video
} // end namespace irr

#endif
#endif
#endif


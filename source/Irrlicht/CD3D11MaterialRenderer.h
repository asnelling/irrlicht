// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D11_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "CD3D11MaterialRenderer.h"
#include "IGPUProgrammingServices.h"

#include "IMaterialRenderer.h"

#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dCompiler.h>
#include <d3dx11core.h>

namespace irr
{
namespace io
{
class IFileSystem;
}

namespace video
{

class IVideoDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;
class CD3D11VertexDeclaration;

struct SShaderBuffer 
{
	SShaderBuffer()
		: data(NULL), name(""), size(-1)
	{
	}

	void AddRef()
	{
		if(data)
			data->AddRef();
	};

	void Release()
	{
		u32 o;

		if(data)
			if(!(o = data->Release()))
				delete this;
	};

	ID3D11Buffer* data;
	core::stringc name;
	s32 size;
};

struct SShaderVariable 
{
	SShaderVariable()
		: offset(-1), name(""), buffer(NULL), size(-1)
	{
	}

	SShaderBuffer* buffer;
	core::stringc name;
	s32 offset;
	s32 size;
};

struct SShader
{
	SShader()
		: shader(NULL)
	{
	}

	void AddRef()
	{
		const u32 size = buffers.size();

		for(u32 i = 0; i < size; i++)
			buffers[i]->AddRef();

		if(shader)
			shader->AddRef();
	}

	void Release()
	{
		u32 size = buffers.size();

		for(u32 i = 0; i < size; i++)
		{
			buffers[i]->Release();
		}

		size = vars.size();

		for(u32 i = 0; i < size; i++)
		{
			delete vars[i];
		}

		vars.clear();
		u32 o;
		if(shader)
			if(!(o = shader->Release()))
			{
				buffers.clear();
				delete this;
			}
	}

	core::array<SShaderBuffer*> buffers;
	core::array<SShaderVariable*> vars;

	IUnknown* shader;
};

//! Class for using vertex and pixel shaders via HLSL with D3D11
class CD3D11MaterialRenderer : public IMaterialRenderer
{
public:

	//! Public constructor
	CD3D11MaterialRenderer(ID3D11Device* device, video::IVideoDriver* driver, s32& outMaterialTypeNr,
		const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8* geometryShaderProgram, const c8* geometryShaderEntryPointName, E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 verticesOut, E_VERTEX_TYPE vertexTypeOut,				// Only for DirectX 11
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseRenderer, s32 userData, io::IFileSystem* fileSystem);

	//! Destructor
	virtual ~CD3D11MaterialRenderer();

	//! sets a variable in the shader.
	//! \param name: Name of the variable
	//! \param floats: Pointer to array of floats
	//! \param count: Amount of floats in array.
	virtual bool setVariable(s32 id, const f32* floats, int count, E_SHADER_TYPE type);

	virtual bool setVariable(s32 id, const s32* ints, int count, E_SHADER_TYPE type);

	virtual s32 getVariableID(const c8* name,E_SHADER_TYPE type);

	virtual s32 getConstantBufferID(const c8* name,E_SHADER_TYPE type);

	virtual bool setConstantBuffer(s32 id, const void* data, E_SHADER_TYPE type);

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	virtual void OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial, bool resetAllRenderstates, video::IMaterialRendererServices* services);
	
	virtual void OnUnsetMaterial();

	//! get shader signature
	virtual void* getShaderByteCode() const;

	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const;

protected:
	CD3D11MaterialRenderer(ID3D11Device* device, video::IVideoDriver* driver, IShaderConstantSetCallBack* callback, IMaterialRenderer* baseRenderer, io::IFileSystem* fileSystem = 0, s32 userData = 0);

	bool init(const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
			  const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
			  const c8* geometryShaderProgram = NULL, const c8* geometryShaderEntryPointName = NULL, E_GEOMETRY_SHADER_TYPE gsCompileTarget = EGST_COUNT,
			  const c8* hullShaderProgram = NULL, const c8* hullShaderEntryPointName = NULL, E_HULL_SHADER_TYPE hsCompileTarget = EHST_COUNT,
			  const c8* domainShaderProgram = NULL, const c8* domainShaderEntryPointName = NULL, E_DOMAIN_SHADER_TYPE dsCompileTarget = EDST_COUNT,
			  const c8* computeShaderProgram = NULL, const c8* computeShaderEntryPointName = NULL, E_COMPUTE_SHADER_TYPE csCompileTarget = ECST_COUNT);

	bool createShader(const char* code, const char* entryPointName, const char* targetName, UINT flags, E_SHADER_TYPE type);

	bool createConstantBuffer(ID3D10Blob* program, E_SHADER_TYPE type);

	void printVariables(E_SHADER_TYPE type);

	void printBuffers(E_SHADER_TYPE type);

	bool stubD3DXCompileShader(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
		LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode);
	
	SShader* getShader( E_SHADER_TYPE type );
	SShaderVariable* getVariable( SShader* sh, s32 id );
	SShaderBuffer* getBuffer( E_SHADER_TYPE type, s32 id );

	bool sameFile;

	// DX 11 objects
	ID3D11Device* Device;
	ID3D11DeviceContext* Context;

	IShaderConstantSetCallBack* CallBack;
	// Irrlicht objects
	IVideoDriver* Driver;
	IMaterialRenderer* BaseRenderer;

	SMaterial CurrentMaterial;

	s32 UserData;

	ID3D10Blob* Buffer;

	ID3DInclude* Includer;

	SShader* VsShader;
	SShader* PsShader;
	SShader* GsShader;
	SShader* HsShader;
	SShader* DsShader;
	SShader* CsShader;
};

} // end namespace video
} // end namespace irr

#endif
#endif
#endif
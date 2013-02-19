	// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#include "CD3D11MaterialRenderer.h"
#include "CD3D11VertexDeclaration.h"
#include "IShaderConstantSetCallBack.h"
#include "IVideoDriver.h"
#include "os.h"
#include "irrString.h"
#include "IFileSystem.h"
#include "irrMap.h"
#include "CD3D11Driver.h"
#include "CD3D11CallBridge.h"

#include <d3dcompiler.h>

class IncludeFX : public ID3DInclude 
{ 
public:
	IncludeFX(irr::io::IFileSystem* fileSystem)
	{
		FileSystem = fileSystem;
	} 

	virtual STDMETHODIMP Close( THIS_ LPCVOID pData ) 
	{
		if(pData)
			delete[] pData;

		return S_OK;
	}

	virtual STDMETHODIMP Open( THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes ) 
	{
		irr::io::IReadFile* file = FileSystem->createAndOpenFile(pFileName);
		if (!file)
		{
			irr::os::Printer::log("Could not open included shader program file.",
				pFileName, irr::ELL_WARNING);
			return S_FALSE;

		}

		irr::u32 mBytes = file->getSize();
		pBytes = &mBytes;

		irr::c8* retInclude = new irr::c8[mBytes+1];
		file->read(retInclude, mBytes);
		retInclude[mBytes] = 0;
		ppData = (LPCVOID*)retInclude;

		file->drop();

		return S_OK;
	}

private:
	irr::io::IFileSystem* FileSystem;
};

namespace irr
{
namespace video
{

//! Public constructor
CD3D11MaterialRenderer::CD3D11MaterialRenderer(ID3D11Device* device, video::IVideoDriver* driver, CD3D11CallBridge* bridgeCalls, s32& outMaterialTypeNr,
		const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8* geometryShaderProgram, const c8* geometryShaderEntryPointName, E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 verticesOut, E_VERTEX_TYPE vertexTypeOut,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseRenderer, s32 userData, io::IFileSystem* fileSystem)
	: Driver(driver), Device(device), Context(NULL), BridgeCalls(bridgeCalls),
	BaseRenderer(baseRenderer), CallBack(callback), UserData(userData),
	vsShader(NULL), psShader(NULL), gsShader(NULL), hsShader(NULL), dsShader(NULL), csShader(NULL),
	includer(NULL), buffer(NULL), sameFile(false)
{
#ifdef _DEBUG
	setDebugName("CD3D11MaterialRenderer");
#endif

	outMaterialTypeNr = -1;

	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext( &Context );
	}	

	if (BaseRenderer)
		BaseRenderer->grab();

	if(CallBack)
		CallBack->grab();

	if(fileSystem)
		includer = new IncludeFX(fileSystem);

	if(!init(vertexShaderProgram, vertexShaderEntryPointName, vsCompileTarget,
		     pixelShaderProgram, pixelShaderEntryPointName, psCompileTarget,
			 geometryShaderProgram, geometryShaderEntryPointName, gsCompileTarget,
			 NULL, NULL, (E_HULL_SHADER_TYPE)0,  // hull shader
			 NULL, NULL, (E_DOMAIN_SHADER_TYPE)0,  // domain shader
			 NULL, NULL, (E_COMPUTE_SHADER_TYPE)0)) // compute shader
		return;

	outMaterialTypeNr = driver->addMaterialRenderer(this);
}

CD3D11MaterialRenderer::CD3D11MaterialRenderer(ID3D11Device* device, video::IVideoDriver* driver, CD3D11CallBridge* bridgeCalls, IShaderConstantSetCallBack* callback, IMaterialRenderer* baseRenderer, io::IFileSystem* fileSystem, s32 userData)
											   : Driver(driver), Device(device), Context(NULL), BridgeCalls(bridgeCalls), BaseRenderer(baseRenderer), CallBack(callback), UserData(userData),
											   vsShader(NULL), psShader(NULL), gsShader(NULL), hsShader(NULL), dsShader(NULL), csShader(NULL),
											   includer(NULL), buffer(NULL), sameFile(false)
{
#ifdef _DEBUG
	setDebugName("CD3D11MaterialRenderer");
#endif

	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext( &Context );
	}	

	if (BaseRenderer)
		BaseRenderer->grab();

	if(CallBack)
		CallBack->grab();

	if(fileSystem)
		includer = new IncludeFX(fileSystem);
}

CD3D11MaterialRenderer::~CD3D11MaterialRenderer()
{
	if (BaseRenderer)
		BaseRenderer->drop();

	if(Context)
		Context->Release();

	if(Device)
		Device->Release();

	if(CallBack)
		CallBack->drop();

	if(includer)
		delete includer;

	if(vsShader)
		vsShader->Release();

	if(psShader)
		psShader->Release();

	if(gsShader)
		gsShader->Release();

	if(hsShader)
		hsShader->Release();

	if(dsShader)
		dsShader->Release();

	if(csShader)
		csShader->Release();

	if(buffer)
		buffer->Release();
}

bool CD3D11MaterialRenderer::createShader(const char* code,
										  const char* entryPointName,
										  const char* targetName, UINT flags, E_SHADER_TYPE type)
{
	if(!code)
		return true;

	HRESULT result = 0;
	ID3D10Blob* shaderBuffer = 0;

	if(!stubD3DXCompileShader(code, strlen(code), "", NULL, includer, entryPointName, targetName, flags, 0, &shaderBuffer))
		return false;

	if(!shaderBuffer)
		return false;

	SShader* s = new SShader();

	// Create the shader from the buffer.
	switch(type)
	{
	case EST_VERTEX_SHADER:
		vsShader = s;
		result = Device->CreateVertexShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11VertexShader**)(&vsShader->shader));
		break;
	case EST_PIXEL_SHADER:
		psShader = s;
		result = Device->CreatePixelShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11PixelShader**)(&psShader->shader));
		break;
	case EST_GEOMETRY_SHADER:
		gsShader = s;
		result = Device->CreateGeometryShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11GeometryShader**)(&gsShader->shader));
		break;
	case EST_DOMAIN_SHADER:
		dsShader = s;
		result = Device->CreateDomainShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11DomainShader**)(&dsShader->shader));
		break;
	case EST_HULL_SHADER:
		hsShader = s;
		result = Device->CreateHullShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11HullShader**)(&hsShader->shader));
		break;
	case EST_COMPUTE_SHADER:
		csShader = s;
		result = Device->CreateComputeShader(shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize(), NULL, (ID3D11ComputeShader**)(&csShader->shader));
		break;
	}

	if(FAILED(result))
	{
		LPTSTR errorText = NULL;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			result,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&errorText,
			0,
			NULL);

		if ( NULL != errorText )
		{
			core::stringc errorMsg = "Could not create ";

			switch(type)
			{
			case EST_VERTEX_SHADER:
				errorMsg += "vertexshader: ";
				break;
			case EST_PIXEL_SHADER:
				errorMsg += "pixelshader: ";
				break;
			case EST_GEOMETRY_SHADER:
				errorMsg += "geometryshader: ";
				break;
			case EST_DOMAIN_SHADER:
				errorMsg += "domainshader: ";
				break;
			case EST_HULL_SHADER:
				errorMsg += "hullshader: ";
				break;
			case EST_COMPUTE_SHADER:
				errorMsg += "computeshader: ";
				break;
			}

			errorMsg += errorText;
			os::Printer::log(errorMsg.c_str(), ELL_ERROR);

			// release memory allocated by FormatMessage()
			LocalFree(errorText);
			errorText = NULL;
		}

		return false;
	}

	createConstantBuffer(shaderBuffer, type);

	switch (type)
	{
	case EST_VERTEX_SHADER:
		buffer = shaderBuffer;
		buffer->AddRef();
		break;
	case EST_GEOMETRY_SHADER:
		if(!buffer)
		{
			buffer = shaderBuffer;
			buffer->AddRef();
		}
		break;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	shaderBuffer->Release();
	shaderBuffer = 0;

	return true;
}

SShader* CD3D11MaterialRenderer::getShader( E_SHADER_TYPE type )
{
	switch(type)
	{
	case EST_VERTEX_SHADER:
		return vsShader;
	case EST_PIXEL_SHADER:
		return psShader;
	case EST_GEOMETRY_SHADER:
		return gsShader;
	case EST_DOMAIN_SHADER:
		return dsShader;
	case EST_HULL_SHADER:
		return hsShader;
	case EST_COMPUTE_SHADER:
		return csShader;
	default:
		return NULL;
	}
}

SShaderVariable* CD3D11MaterialRenderer::getVariable( SShader* sh, s32 id )
{
	if(id < 0 || id >= (s32)sh->vars.size())
		return NULL;

	return sh->vars[id];
}

SShaderBuffer* CD3D11MaterialRenderer::getBuffer( E_SHADER_TYPE type, s32 id )
{
	SShader* sh = getShader(type);

	if(!sh)
		return NULL;

	if(id < 0 || id >= (s32)sh->buffers.size())
		return NULL;

	return sh->buffers[id];
}


bool CD3D11MaterialRenderer::setVariable(s32 id, const f32* floats, int count, E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	if(!sh)
		return false;

	SShaderVariable* var = getVariable(sh, id);
	
	if(!var)
		return false;

	SShaderBuffer* buff = var->buffer;

	core::matrix4* m = NULL;
	// if it is a matrix transpose it
	if(var->classType == D3D10_SVC_MATRIX_COLUMNS)
	{
		m = (core::matrix4*)floats;
		*m = m->getTransposed();
	}

	D3D11_MAPPED_SUBRESOURCE mappedData;

	if( FAILED( Context->Map(buff->data, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData ) ) )
		return false;

	c8* byteData = (c8*)buff->cData;
	
	byteData += var->offset;

	if(m)
		memcpy(byteData, m, count * sizeof(f32));
	else
		memcpy(byteData, floats, count * sizeof(f32));

	memcpy(mappedData.pData, buff->cData, buff->size);

	Context->Unmap(buff->data, 0);

	return true;
}

bool CD3D11MaterialRenderer::setVariable(s32 id, const s32* ints, int count, E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	if(!sh)
		return false;

	SShaderVariable* var = getVariable(sh, id);

	if(!var)
		return false;

	SShaderBuffer* buff = var->buffer;

	D3D11_MAPPED_SUBRESOURCE mappedData;

	if( FAILED( Context->Map(buff->data, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData ) ) )
		return false;

	c8* byteData = (c8*)buff->cData;

	byteData += var->offset;

	memcpy(byteData, ints, count * sizeof(s32));
	memcpy(mappedData.pData, buff->cData, buff->size);

	Context->Unmap(buff->data, 0);

	return true;
}

bool CD3D11MaterialRenderer::setConstantBuffer( s32 id, const void* data, E_SHADER_TYPE type)
{
	SShaderBuffer* buff = getBuffer(type, id);

	if(!buff)
		return false;

	D3D11_MAPPED_SUBRESOURCE mappedData;

	if( FAILED( Context->Map(buff->data, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData ) ) )
		return false;

	memcpy(mappedData.pData, data, buff->size);

	Context->Unmap(buff->data, 0);

	return false;
}


s32 CD3D11MaterialRenderer::getConstantBufferID(const c8* name, E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	const u32 size = sh->buffers.size();

	for(u32 i = 0; i < size; ++i)
	{
		if(sh->buffers[i]->name == name)
			return i;
	}

	core::stringc s = "HLSL buffer to get ID not found: '";
	s += name;
	s += "'. Available buffers are:";
	os::Printer::log(s.c_str(), ELL_WARNING);

	printBuffers(type);

	return -1;
}

s32 CD3D11MaterialRenderer::getVariableID(const c8* name, E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	const u32 size = sh->vars.size();

	for(u32 i = 0; i < size; ++i)
	{
		if(sh->vars[i]->name == name)
			return i;
	}

	core::stringc s = "HLSL variable to get ID not found: '";
	s += name;
	s += "'. Available variables are:";
	os::Printer::log(s.c_str(), ELL_WARNING);

	printVariables(type);

	return -1;
}

bool CD3D11MaterialRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	if (!Context)
		return false;

	if (BaseRenderer)
		BaseRenderer->OnRender(service, vtxtype);

	if (CallBack && (vsShader || psShader || gsShader))
		CallBack->OnSetConstants(service, UserData);

	BridgeCalls->setVertexShader(vsShader);
	BridgeCalls->setPixelShader(psShader);
	BridgeCalls->setGeometryShader(gsShader);

	BridgeCalls->setHullShader(hsShader);
	BridgeCalls->setDomainShader(dsShader);
	BridgeCalls->setComputeShader(csShader);

	return true;
}

void CD3D11MaterialRenderer::OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial,
												bool resetAllRenderstates, video::IMaterialRendererServices* services)
{
	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (BaseRenderer)
			BaseRenderer->OnSetMaterial(material, material, resetAllRenderstates, services);
	}

	//let callback know used material
	if (CallBack)
		CallBack->OnSetMaterial(material);

	services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void CD3D11MaterialRenderer::OnUnsetMaterial()
{
	if (BaseRenderer)
		BaseRenderer->OnUnsetMaterial();
}

//! get shader signature
void* CD3D11MaterialRenderer::getShaderByteCode() const
{
	return buffer ? buffer->GetBufferPointer() : NULL;
}

//! get shader signature size
u32 CD3D11MaterialRenderer::getShaderByteCodeSize() const
{
	return buffer ? buffer->GetBufferSize() : 0;
}

void CD3D11MaterialRenderer::printVariables(E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	if(!sh)
		return;

	core::stringc text = "";

	const u32 size = sh->vars.size();
	for(u32 i = 0; i < size; ++i)
	{
		text += "Name: ";
		text += sh->vars[i]->name;
		text += " Size: ";
		text += sh->vars[i]->size;
		text += " Offset: ";
		text += sh->vars[i]->offset;
		text += "\n";
	}
	os::Printer::log(text.c_str());
}

void CD3D11MaterialRenderer::printBuffers(E_SHADER_TYPE type)
{
	SShader* sh = getShader(type);

	if(!sh)
		return;

	core::stringc text = "";
	
	const u32 size = sh->buffers.size();
	
	for(u32 i = 0; i < size; ++i)
	{
		text += "Name: ";
		text += sh->buffers[i]->name;
		text += " Size: ";
		text += sh->buffers[i]->size;
		text += "\n";
	}

	os::Printer::log(text.c_str());
}


bool CD3D11MaterialRenderer::init(const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
								  const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget, 
								  const c8* geometryShaderProgram, const c8* geometryShaderEntryPointName, E_GEOMETRY_SHADER_TYPE gsCompileTarget,
								  const c8* hullShaderProgram, const c8* hullShaderEntryPointName, E_HULL_SHADER_TYPE hsCompileTarget,
								  const c8* domainShaderProgram, const c8* domainShaderEntryPointName, E_DOMAIN_SHADER_TYPE dsCompileTarget,
								  const c8* computeShaderProgram, const c8* computeShaderEntryPointName, E_COMPUTE_SHADER_TYPE csCompileTarget)
{
	if (vsCompileTarget < 0 || vsCompileTarget > EVST_COUNT)
	{
		os::Printer::log("Invalid HLSL vertex shader compilation target.", ELL_ERROR);
		return false;
	}

	UINT flags = 0;
	if (vsCompileTarget >= EVST_VS_4_0 && psCompileTarget >= EPST_PS_4_0)
		flags |= D3D10_SHADER_ENABLE_STRICTNESS;
	else
	{
		flags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
		vsCompileTarget = EVST_VS_4_0;
		psCompileTarget = EPST_PS_4_0;
	}

#ifdef _DEBUG
	// These values allow use of PIX and shader debuggers
	flags |= D3D10_SHADER_DEBUG;
	flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#else
	// These flags allow maximum performance
	flags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
#endif

	if(vertexShaderProgram == pixelShaderProgram)
		sameFile = true;

	if (!createShader(vertexShaderProgram, vertexShaderEntryPointName, VERTEX_SHADER_TYPE_NAMES[vsCompileTarget], flags, EST_VERTEX_SHADER))
		return false;

	if (!createShader(pixelShaderProgram, pixelShaderEntryPointName, PIXEL_SHADER_TYPE_NAMES[psCompileTarget], flags, EST_PIXEL_SHADER))
		return false;

	if (!createShader(geometryShaderProgram, geometryShaderEntryPointName, GEOMETRY_SHADER_TYPE_NAMES[gsCompileTarget], flags, EST_GEOMETRY_SHADER))
		return false;

	if (!createShader(domainShaderProgram, domainShaderEntryPointName, DOMAIN_SHADER_TYPE_NAMES[dsCompileTarget], flags, EST_DOMAIN_SHADER))
		return false;

	if (!createShader(hullShaderProgram, hullShaderEntryPointName, HULL_SHADER_TYPE_NAMES[hsCompileTarget], flags, EST_HULL_SHADER))
		return false;

	if (!createShader(computeShaderProgram, computeShaderEntryPointName, COMPUTE_SHADER_TYPE_NAMES[csCompileTarget], flags, EST_COMPUTE_SHADER))
		return false;

	return true;
}

bool CD3D11MaterialRenderer::stubD3DXCompileShader(LPCVOID pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
												   LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode)
{	
	static bool LoadFailed = false;
	static pD3DCompile pFn = 0;

	if(LoadFailed)
		return false;

	if (!pFn && !LoadFailed)
	{
		HINSTANCE D3DLibrary = LoadLibrary(D3DCOMPILER_DLL);

		if (!D3DLibrary)
		{
			LoadFailed = true;
			os::Printer::log("Could not load library", D3DCOMPILER_DLL, ELL_ERROR);
			return false;
		}

		pFn = (pD3DCompile)GetProcAddress(D3DLibrary, "D3DCompile");

		if (!pFn)
		{
			LoadFailed = true;
			os::Printer::log("Could not load shader function D3DCompile from dll, shaders disabled",
				D3DCOMPILER_DLL, ELL_ERROR);

			return false;
		}
	}

	ID3D10Blob* ppErrorMsgs = 0;

	HRESULT result = (*pFn)(pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, &ppErrorMsgs);

	if (FAILED(result))
	{
		core::stringc errorMsg = "Could not compile shader";
		if (ppErrorMsgs)
		{
			errorMsg += ": ";
			errorMsg += static_cast<const char*>(ppErrorMsgs->GetBufferPointer());

			ppErrorMsgs->Release();
		}
		os::Printer::log(errorMsg.c_str(), ELL_ERROR);

		return false;
	}
#ifdef _DEBUG
	else if (ppErrorMsgs)
	{
		core::stringc errorMsg = "Shader compilation warning: ";
		errorMsg += static_cast<const char*>(ppErrorMsgs->GetBufferPointer());

		ppErrorMsgs->Release();
		ppErrorMsgs = NULL;

		os::Printer::log(errorMsg.c_str(), ELL_WARNING);
	}
#endif

	if(ppErrorMsgs)
		ppErrorMsgs->Release();

	return true;
}

bool CD3D11MaterialRenderer::createConstantBuffer(ID3D10Blob* code, E_SHADER_TYPE type)
{
	// D3DXCompile
	typedef HRESULT (WINAPI *D3DX11ReflectFunc)(LPCVOID pSrcData, SIZE_T SrcDataSize, REFIID pInterface, void** ppReflector);

	static D3DX11ReflectFunc pFn = 0;
	static bool LoadFailed = false;

	if(LoadFailed)
		return false;

	if (!pFn && !LoadFailed)
	{
		HINSTANCE D3DLibrary = LoadLibrary(D3DCOMPILER_DLL);

		if (!D3DLibrary)
		{
			LoadFailed = true;

			os::Printer::log("Could not load library", D3DCOMPILER_DLL, ELL_ERROR);
			return false;
		}

		pFn = (D3DX11ReflectFunc)GetProcAddress(D3DLibrary, "D3DReflect");

		if (!pFn)
		{
			LoadFailed = true;

			os::Printer::log("Could not load shader function D3DReflect from dll",
				D3DCOMPILER_DLL, ELL_ERROR);

			return false;
		}
	}

	ID3D11ShaderReflection* pReflector = NULL; 

	HRESULT result = (*pFn)(code->GetBufferPointer(), code->GetBufferSize(), 
		IID_ID3D11ShaderReflection, (void**) &pReflector);

	if(FAILED(result))
	{
		os::Printer::log("Could not reflect constant buffer in shader.", ELL_ERROR);

		return false;
	}

	D3D11_SHADER_DESC shaderDesc;
	pReflector->GetDesc(&shaderDesc);

	SShader* sh = getShader(type);

	const u32 size = shaderDesc.ConstantBuffers;

	for(u32 i = 0; i < size; ++i)
	{
		ID3D11ShaderReflectionConstantBuffer* buffer = pReflector->GetConstantBufferByIndex(i);

		D3D11_SHADER_BUFFER_DESC bufferDesc;
		buffer->GetDesc(&bufferDesc);

		SShaderBuffer* sBuffer = NULL;

		// take the same buffer from the vertex shader if vs and ps are in the same file
		if(sameFile && type != EST_VERTEX_SHADER)
		{
			SShader* shader = getShader(EST_VERTEX_SHADER);
	
			for(u32 j = 0; j < shader->buffers.size(); j++)
			{
				if(shader->buffers[j]->name == bufferDesc.Name)
				{
					sBuffer = shader->buffers[j];
					sBuffer->AddRef();
					break;
				}
			}
		}

		if(!sBuffer)
		{
			sBuffer = new SShaderBuffer();
			sBuffer->name = bufferDesc.Name;
			sBuffer->size = bufferDesc.Size;
		}

		for(u32 j = 0; j < bufferDesc.Variables; j++)
		{
			ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(j);

			D3D11_SHADER_VARIABLE_DESC varDesc;
			var->GetDesc(&varDesc);

			D3D11_SHADER_TYPE_DESC typeDesc;
			var->GetType()->GetDesc(&typeDesc);

			SShaderVariable* sv = new SShaderVariable();
			sv->name = varDesc.Name;
			sv->buffer = sBuffer;
			sv->offset = varDesc.StartOffset;
			sv->size = varDesc.Size;
			sv->baseType = typeDesc.Type;
			sv->classType = typeDesc.Class;

			sh->vars.push_back(sv);
		}

		if(!sBuffer->data)
		{	
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sBuffer->size;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			// Create the buffer.
			result = Device->CreateBuffer(&cbDesc, NULL, &sBuffer->data);

			if(FAILED(result))
			{
				os::Printer::log("Could not create constant buffer", sBuffer->name, ELL_ERROR);

				delete sBuffer;

				continue;
			}

			sBuffer->cData = malloc(sBuffer->size);
		}

		sh->buffers.push_back(sBuffer);
	}

	pReflector->Release();

	return true;
}

} // end namespace video
} // end namespace irr

#endif
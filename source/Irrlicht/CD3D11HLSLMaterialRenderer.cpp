	// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#include "CD3D11HLSLMaterialRenderer.h"
#include "CD3D11VertexDeclaration.h"
#include "IShaderConstantSetCallBack.h"
#include "IVideoDriver.h"
#include "os.h"
#include "irrString.h"
#include "IFileSystem.h"
#include "irrMap.h"
#include "CD3D11Driver.h"

#include <d3dCompiler.h>


class IncludeFX :public ID3DInclude 
{ 
public:
	IncludeFX(irr::io::IFileSystem* fileSystem)
	{
		FileSystem = fileSystem;
	} 

	virtual HRESULT __stdcall Close( THIS_ LPCVOID pData ) 
	{
		irr::core::map<LPCVOID, irr::io::IReadFile*>::Node* node = Include.find(pData);

		if(node)
		{
			node->getValue()->drop();
				
			Include.remove(node);
		}

		if(pData)
			delete[] pData;

		return S_OK;
	}

	virtual HRESULT __stdcall Open( THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes ) 
	{
		irr::io::IReadFile* file = FileSystem->createAndOpenFile(pFileName);
		if (!file)
		{
			irr::os::Printer::log("Could not open Include shader program file",
				pFileName, irr::ELL_WARNING);
			return S_FALSE;

		}
		irr::u32 mBytes = file->getSize();
		pBytes = &mBytes;
		
		irr::c8* retInclude = new irr::c8[mBytes+1];
		file->read(retInclude, mBytes);
		retInclude[mBytes] = 0;
		ppData = (LPCVOID*)retInclude;

		Include.insert(ppData, file);
	
		return S_OK;
	}

private:
	irr::core::map<LPCVOID, irr::io::IReadFile*> Include;
	irr::io::IFileSystem* FileSystem;
};


#ifndef _IRR_D3D_NO_SHADER_DEBUGGING
#include <stdio.h>
#endif


namespace irr
{
namespace video
{

//! Public constructor
CD3D11HLSLMaterialRenderer::CD3D11HLSLMaterialRenderer(ID3D11Device* device, 
		video::IVideoDriver* driver, s32& outMaterialTypeNr,
		const c8* vertexShaderProgram, const c8* vertexShaderEntryPointName, E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8* pixelShaderProgram, const c8* pixelShaderEntryPointName, E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8* geometryShaderProgram, const c8* geometryShaderEntryPointName, E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 verticesOut, E_VERTEX_TYPE vertexTypeOut,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData, io::IFileSystem* fileSystem)
	: CD3D11ShaderMaterialRenderer(device, driver, callback, baseMaterial, userData), 
	Effect(0), isStreamOutput(false)
{
	#ifdef _DEBUG
	setDebugName("CD3D11HLSLMaterialRenderer");
	#endif

	if (CallBack)
		CallBack->grab();

	outMaterialTypeNr = -1;
	ZeroMemory(&PassDescription, sizeof(D3DX11_PASS_DESC));

	Includer = new IncludeFX(fileSystem);

	// Create effect
	if (!init(vertexShaderProgram, vertexShaderEntryPointName, vsCompileTarget,
		 pixelShaderProgram, pixelShaderEntryPointName, psCompileTarget,
		 geometryShaderProgram, geometryShaderEntryPointName, gsCompileTarget,
		 inType, outType, verticesOut, vertexTypeOut))
	{
		if (BaseMaterial)
			BaseMaterial->drop();
		BaseMaterial = NULL;

		if (CallBack)
			CallBack->drop();
		CallBack = NULL;

		if(Effect)
			Effect->Release();
		Effect = NULL;

		if(Context)
			Context->Release();
		Context = NULL;

		if(Device)
			Device->Release();
		Device = NULL;

		if(Includer)
			delete Includer;
		Includer = NULL;
		return;
	}

	// register myself as new material
	outMaterialTypeNr = Driver->addMaterialRenderer(this);
}

CD3D11HLSLMaterialRenderer::~CD3D11HLSLMaterialRenderer()
{
	if(Effect)
		Effect->Release();
	Effect = NULL;

	if(Includer)
		delete Includer;
	Includer = NULL;
}

bool CD3D11HLSLMaterialRenderer::setVariable(s32 index, const f32* floats, int count)
{
	if (!Effect || index < 0)
		return false;

	ID3DX11EffectVariable* var = Effect->GetVariableByIndex(index);
	if( var->IsValid() )
	{
		switch( count )
		{
		case 16:
			var->AsMatrix()->SetMatrix((f32*)floats);	
			break;
		case 3:
		case 4:
			var->AsVector()->SetFloatVector((f32*)floats);
			break;
		case 1:
			var->AsScalar()->SetFloat((f32)*floats);
			break;
		default:
			if (count % 16 == 0)
				var->AsMatrix()->SetMatrixArray((f32*)floats,0,count);
			else if (count % 4 == 0 || count % 3 == 0)
				var->AsVector()->SetFloatVectorArray((f32*)floats,0,count);
			else
				var->AsScalar()->SetFloatArray((f32*)floats,0,count);

			break;
		};

		return true;
	}

	return false;
}

bool CD3D11HLSLMaterialRenderer::setVariable(s32 index, const s32* ints, int count)
{
	if (!Effect || index < 0)
		return false;

	ID3DX11EffectVariable* var = Effect->GetVariableByIndex(index);
	if( var->IsValid() )
	{
		switch( count )
		{
		case 4:
			var->AsVector()->SetIntVectorArray((s32*)ints,0,4);
			break;
		case 1:
			var->AsScalar()->SetInt((s32)*ints);
			break;
		default:
			var->AsScalar()->SetIntArray((s32*)ints, 0, count);
			break;
		};

		return true;
	}

	return false;
}

irr::s32 CD3D11HLSLMaterialRenderer::getVariableID( bool vertex, const c8* name )
{
	if (!Effect)
		return -1;

	D3DX11_EFFECT_DESC desc;

	Effect->GetDesc(&desc);

	for(u32 i = 0; i < desc.GlobalVariables; i++)
	{
		D3DX11_EFFECT_VARIABLE_DESC desc2;

		Effect->GetVariableByIndex(i)->GetDesc(&desc2);

		if(strcmp(desc2.Name, name) == 0)
			return i;
	}

	core::stringc s = "HLSL Variable to get ID not found: '";
	s += name;
	s += "'. Available variables are:";
	os::Printer::log(s.c_str(), ELL_WARNING);
	printHLSLVariables();

	return -1;
}

bool CD3D11HLSLMaterialRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	if (!Effect)
		return false;

	CD3D11ShaderMaterialRenderer::OnRender(service, vtxtype);

	// Apply pass
	Technique->GetPassByIndex(0)->Apply( 0, Context );

	return true;
}

//! get shader signature
void* CD3D11HLSLMaterialRenderer::getShaderByteCode() const
{
	return PassDescription.pIAInputSignature;
}

//! get shader signature size
u32 CD3D11HLSLMaterialRenderer::getShaderByteCodeSize() const
{
	return PassDescription.IAInputSignatureSize;
}

bool CD3D11HLSLMaterialRenderer::init(const core::stringc vertexShaderProgram,
										const c8* vertexShaderEntryPointName,
										E_VERTEX_SHADER_TYPE vsCompileTarget,
										const core::stringc pixelShaderProgram,
										const c8* pixelShaderEntryPointName,
										E_PIXEL_SHADER_TYPE psCompileTarget,
										const core::stringc geometryShaderProgram,
										const c8* geometryShaderEntryPointName,
										E_GEOMETRY_SHADER_TYPE gsCompileTarget,
										scene::E_PRIMITIVE_TYPE inType,			// Only for OpenGL
										scene::E_PRIMITIVE_TYPE outType,		// Only for OpenGL
										u32 verticesOut,						// Only for OpenGL
										E_VERTEX_TYPE vertexTypeOut)			// Only for Direct3D 11
{
	// now create shaders
	if (vsCompileTarget > EVST_COUNT)
	{
		os::Printer::log("Invalid HLSL vertex shader compilation target", ELL_ERROR);
		return false;
	}

	UINT flags = 0;
	if (vsCompileTarget > EVST_VS_4_0 && psCompileTarget > EPST_PS_4_0)
		flags |= D3D10_SHADER_ENABLE_STRICTNESS;
	else
	{
		flags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
		vsCompileTarget = EVST_VS_4_0;
		psCompileTarget = EPST_PS_4_0;
	}


	// Create effect string
	
	core::stringc effectString = vertexShaderProgram;
	effectString += "\n";

	// Prevent overwriting
	if (geometryShaderProgram != vertexShaderProgram)
	{
		effectString += geometryShaderProgram;
		effectString += "\n";
	}
	if (pixelShaderProgram != geometryShaderProgram &&
		pixelShaderProgram != vertexShaderProgram)
	{
		effectString += pixelShaderProgram;
		effectString += "\n";
	}

	// Preparing technique
	core::stringc techString = "technique11 CustomTech\n";
	techString += "{\n";
	techString += "	pass p0\n";
	techString += "	{\n";

	// Set vertex shader
	techString += "		SetVertexShader( CompileShader( ";
	techString += VERTEX_SHADER_TYPE_NAMES[vsCompileTarget];
	techString += ", ";
	techString += vertexShaderEntryPointName;
	techString += "() ) );\n";

	// Consider stream output for effect if pixel shader is null
	if (!pixelShaderProgram.size())
	{
		isStreamOutput = true;

		// if not exists geometry shader, use stream output with vertex shader
		if (geometryShaderProgram.size())
		{
			techString += "		SetGeometryShader( ConstructGSWithSO( CompileShader( ";
			techString += GEOMETRY_SHADER_TYPE_NAMES[gsCompileTarget];
			techString += ", ";
			techString += geometryShaderEntryPointName;
			techString += "() ), ";
			techString += parseStreamOutputDeclaration( ((CD3D11Driver*)Driver)->getVertexDeclaration(vertexTypeOut) );
			techString += " ) );\n";
		}
		else
		{
			techString += "		SetGeometryShader( ConstructGSWithSO( CompileShader( ";
			techString += VERTEX_SHADER_TYPE_NAMES[vsCompileTarget];
			techString += ", ";
			techString += vertexShaderEntryPointName;
			techString += "() ), ";
			techString += parseStreamOutputDeclaration( ((CD3D11Driver*)Driver)->getVertexDeclaration(vertexTypeOut) );
			techString += " ) );\n";
		}

		// Add NULL pixel shader
		techString += "		SetPixelShader( NULL );\n";
		techString += "	}\n";
	}
	else
	{
		if (geometryShaderProgram.size())
		{
			// Set geometry shader
			techString += "		SetGeometryShader( CompileShader( ";
			techString += GEOMETRY_SHADER_TYPE_NAMES[gsCompileTarget];
			techString += ", ";                                                                                                        
			techString += geometryShaderEntryPointName;
			techString += "() ) );\n";
		}
		else
		{
			// Add NULL geometry shader
			techString += "		SetGeometryShader( NULL );\n";
		}

		// Set pixel shader
		techString += "		SetPixelShader( CompileShader( ";
		techString += PIXEL_SHADER_TYPE_NAMES[psCompileTarget];
		techString += ", ";
		techString += pixelShaderEntryPointName;
		techString += "() ) );\n";
		techString += "	}\n";
	}

	techString += "}";

	effectString += techString;
	effectString += "\n";

#ifdef _DEBUG
	// These values allow use of PIX and shader debuggers
	flags |= D3D10_SHADER_DEBUG;
	flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#else
	// These flags allow maximum performance
		flags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
#endif
	// Compile effect
	HRESULT hr = S_OK;
	ID3DBlob* ppCode = NULL;
	ID3DBlob* ppErrors = NULL;

	hr = D3DCompile( effectString.c_str(), effectString.size(), "",
					 NULL, // macros
					 Includer, // includes
					 NULL, // entrypoint
					 "fx_5_0",
					 flags,
					 0,
					 &ppCode,
					 &ppErrors );
	if (FAILED(hr))
	{
		core::stringc errorMsg = "Error, could not compile custom effect";
		if (ppErrors)
		{
			errorMsg += ": ";
			errorMsg += static_cast<const char*>(ppErrors->GetBufferPointer());
			ppErrors->Release();
		}
		os::Printer::log(errorMsg.c_str(), ELL_ERROR);
		return false;
	}
#ifdef _DEBUG
	else if (ppErrors)
	{
		core::stringc errorMsg = "Effect compilation warning: ";
		if (ppErrors)
		{
			errorMsg += static_cast<const char*>(ppErrors->GetBufferPointer());
			errorMsg += "\n";
			ppErrors->Release();
		}
		os::Printer::log(errorMsg.c_str(), ELL_ERROR);
	}
#endif

	// Create effect
	hr = D3DX11CreateEffectFromMemory( ppCode->GetBufferPointer(), ppCode->GetBufferSize(), 0, Device, &Effect );
	ppCode->Release();
	if (FAILED(hr))
	{
		LPTSTR errorText = NULL;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL,
					  hr,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  (LPTSTR)&errorText,
					  0,
					  NULL);
		if ( NULL != errorText )
		{
			core::stringc errorMsg = "Error, could not create custom effect: ";
			errorMsg += errorText;
			os::Printer::log(errorMsg.c_str(), ELL_ERROR);

			// release memory allocated by FormatMessage()
			LocalFree(errorText);
			errorText = NULL;
		}
		else
			os::Printer::log("Error, could not create custom effect", ELL_ERROR);
		
		return false;
	}

	// Get technique
	Technique = Effect->GetTechniqueByName("CustomTech");
	if (!Technique->IsValid())
	{
		os::Printer::log("Error, could not obtain effect technique", ELL_ERROR);
		return false;
	}

	// Get pass description
	ID3DX11EffectPass* pass = Technique->GetPassByIndex(0);
	if (!pass->IsValid())
	{
		os::Printer::log("Error, could not obtain effect pass", ELL_ERROR);
		return false;
	}
	pass->GetDesc(&PassDescription);

	return true;
}

core::stringc CD3D11HLSLMaterialRenderer::parseStreamOutputDeclaration(CD3D11VertexDeclaration* declaration)
{
	// Parse declaration
	const core::array<D3D11_SO_DECLARATION_ENTRY>& entries = declaration->getStreamOutputDeclaration();
	u32 size = declaration->size();

	core::stringc decl = "\"";

	for(u32 i = 0; i < size; i++)
	{
		// Add semantic index
		//decl += i;
		//decl += ":";

		// Add semantic name
		//if(strcmp("POSITION", entries[i].SemanticName) == 0)
		//	decl += "SV_Position";
		//else
		decl += entries[i].SemanticName;
		if (entries[i].SemanticIndex > 0)
			decl += entries[i].SemanticIndex;

		// Add coords used
		decl += ".";
		switch (entries[i].ComponentCount)
		{
		case 4:
			if(strcmp("COLOR", entries[i].SemanticName) == 0)
				decl += "rgba";
			else
				decl += "xyzw";
			break;

		case 3:
			decl += "xyz";
			break;

		case 2:
			decl += "xy";
			break;

		case 1:
			decl += "x";
			break;
		}

		// Finish element with ";"
		if (i < (size - 1))
			decl += "; ";
	}

	decl += "\"";

	return decl;
}

void CD3D11HLSLMaterialRenderer::printHLSLVariables()
{
	// currently we only support top level parameters.
	// Should be enough for the beginning. (TODO)
	D3DX11_EFFECT_DESC desc;

	Effect->GetDesc(&desc);

	core::stringc s = "";

	for(u32 i = 0; i < desc.GlobalVariables; i++)
	{
		D3DX11_EFFECT_VARIABLE_DESC desc2;

		Effect->GetVariableByIndex(i)->GetDesc(&desc2);
		
		s += "Name: ";
		s += desc2.Name;
		s += "  Index: ";
		s += i;
		s += "\n";
	}

	os::Printer::log(s.c_str());
}

} // end namespace video
} // end namespace irr

#endif
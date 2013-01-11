// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "CD3D11ParallaxMapRenderer.h"
#include "IMaterialRendererServices.h"
#include "IVideoDriver.h"
#include "os.h"
#include "SLight.h"

//#define SHADER_EXTERNAL_DEBUG

#ifdef SHADER_EXTERNAL_DEBUG
#include "CReadFile.h"
#endif

#include <d3dCompiler.h>

namespace irr
{
namespace video
{
	const char PARALLAX_MAP_SHADER[] =
		"// adding constant buffer for transform matrices\n"\
		"cbuffer cbPerFrame : register(c0)\n"\
		"{\n"\
		"   float4x4 g_mWorld;\n"\
		"   float4x4 g_mWorldViewProj;\n"\
		"	float3	 g_lightPos1;\n"\
		"	float4	 g_lightColor1;\n"\
		"	float3	 g_lightPos2;\n"\
		"	float4	 g_lightColor2;\n"\
		"	float4	 g_scaleFactor;\n"\
		"	float4	 g_eyePosition;\n"\
		"};\n"\
		"\n"\
		"cbuffer cbConsts : register(c1)\n"\
		"{\n"\
		"	float4 zero = float4(0, 0, 0, 0);\n"\
		"	float4 positiveHalf = float4(0.5f, 0.5f, 0.5f, 0.5f);\n"\
		"	float4 negativeHalf = float4(-0.5f, -0.5f, -0.5f, -0.5f);\n"\
		"	float4 negativeOne = float4(-1.f, -1.f, -1.f, -1.f);\n"\
		"	float4 positiveTwo = float4(2.f, 2.f, 2.f, 2.f);\n"\
		"};\n"\
		"\n"\
		"// adding textures and samplers\n"\
		"Texture2D g_tex1 : register(t0);\n"\
		"Texture2D g_tex2 : register(t1);\n"\
		"SamplerState g_sampler1 : register(s0);\n"\
		"SamplerState g_sampler2 : register(s1);\n"\
		"\n"\
		"struct VS_INPUT\n"\
		"{\n"\
		"	float4 pos		: POSITION;\n"\
		"	float3 norm		: NORMAL;\n"\
		"	float4 color	: COLOR;\n"\
		"	float2 tex0		: TEXCOORD0;\n"\
		"	float3 tangent	: TEXCOORD1;\n"\
		"	float3 binormal : TEXCOORD2;\n"\
		"};\n"\
		"\n"\
		"struct PS_INPUT\n"\
		"{\n"\
		"	float4 pos				: SV_Position;\n"\
		"	float2 colorMapCoord	: TEXTURE0;\n"\
		"	float2 normalMapCoord	: TEXTURE1;\n"\
		"	float3 lightVector1		: TEXTURE2;\n"\
		"	float4 lightColor1		: COLOR0;\n"\
		"	float3 lightVector2		: TEXTURE3;\n"\
		"	float4 lightColor2		: COLOR1;\n"\
		"	float3 eyePos			: TEXTURE4;\n"\
		"};\n"\
		"\n"\
		"PS_INPUT VS(VS_INPUT input)\n"\
		"{\n"\
		"	PS_INPUT output = (PS_INPUT)0;\n"\
		"\n"\
		"	// transform position to clip space with worldViewProj matrix\n"\
		"	output.pos = mul( input.pos, g_mWorldViewProj );\n"\
		"\n"\
		"	// transform normal, tangent and binormal\n"\
		"	float3x3 tbnMatrix = mul( float3x3( input.binormal, input.tangent , input.norm ), (float3x3)g_mWorld );\n"\
		"\n"\
		"	// transform vertex into world position\n"\
		"	float4 worldPos = mul( input.pos, g_mWorld );\n"\
		"\n"\
		"	float3 lightDir1 = g_lightPos1 - worldPos;\n"\
		"	float3 lightDir2 = g_lightPos2 - worldPos;\n"\
		"\n"\
		"	// transform light vectors with U, V, W\n"\
		"	output.lightVector1 = normalize( mul( tbnMatrix, lightDir1 ) );\n"\
		"	output.lightVector2 = normalize( mul( tbnMatrix, lightDir2 ) );\n"\
		"	output.eyePos = normalize( mul( tbnMatrix, g_eyePosition - worldPos ) );\n"\
		"\n"\
		"	// calculate attenuation of lights\n"\
		"	lightDir1.x = dot( lightDir1, lightDir1 ) * g_lightColor1.w;\n"\
		"	lightDir1 = rsqrt( lightDir1.x );\n"\
		"	output.lightColor1 = float4( lightDir1, 1.f ) * g_lightColor1;\n"\
		"\n"\
		"	lightDir2.x = dot( lightDir2, lightDir2 ) * g_lightColor2.w;\n"\
		"	lightDir2 = rsqrt( lightDir2.x );\n"\
		"	output.lightColor2 = float4( lightDir2, 1.f ) * g_lightColor2;\n"\
		"\n"\
		"	// output texture coordinates\n"\
		"	output.colorMapCoord = input.tex0;\n"\
		"	output.normalMapCoord = input.tex0;\n"\
		"	output.lightColor1.a = input.color.a;\n"\
		"\n"\
		"	return output;\n"\
		"}\n"\
		"\n"\
		"// High-definition pixel-shader\n"\
		"float4 PS(PS_INPUT input) : SV_Target\n"\
		"{\n"\
		"	// sample texture\n"\
		"	float4 normalMap = g_tex2.Sample( g_sampler2, input.normalMapCoord ).bgra;\n"\
		"\n"\
		"	// move normal vectors from -1..1 into 0..1\n"\
		"	float4 normalVec = mad( normalMap, positiveTwo, negativeOne );\n"\
		"\n"\
		"	// scale by height\n"\
		"	normalVec = normalVec.wwww * g_scaleFactor;\n"\
		"\n"\
		"	// move eye vectors from -1..1 into 0..1\n"\
		"	float3 eyeVec = mad( input.eyePos,  positiveTwo, negativeOne );\n"\
		"\n"\
		"	float2 newTexCoord;\n"\
		"	newTexCoord.xy = mad( normalVec, eyeVec, input.colorMapCoord );\n"\
		"\n"\
		"	float4 colorMap = g_tex1.Sample( g_sampler1, newTexCoord ).bgra;\n"\
		"	normalMap = g_tex2.Sample( g_sampler2, newTexCoord ).bgra;\n"\
		"\n"\
		"	normalMap = mad( normalMap, positiveTwo, negativeOne );\n"\
		"	float3 lightVec1 = mad( input.lightVector1, positiveTwo, negativeOne );\n"\
		"	float3 lightVec2 = mad( input.lightVector2, positiveTwo, negativeOne );\n"\
		"\n"\
		"	lightVec1 = dot( lightVec1, normalMap );\n"\
		"	lightVec1 = max( lightVec1, zero);\n"\
		"	lightVec1 = mul( lightVec1, input.lightColor1 );\n"\
		"\n"\
		"	lightVec2 = dot( lightVec2, normalMap );\n"\
		"	lightVec2 = max( lightVec2, zero );\n"\
		"	lightVec2 = mad( lightVec2, input.lightColor2, lightVec1 );\n"\
		"\n"\
		"	colorMap.xyz = colorMap.xyz * lightVec2;\n"\
		"	colorMap.a = input.lightColor1.a;\n"\
		"\n"\
		"	return colorMap;\n"\
		"}\n"\
		"\n"\
		"// Technique for standard vertex type\n"\
		"technique11 ParallaxMapTechnique\n"\
		"{\n"\
		"	pass p0\n"\
		"	{\n"\
		"		SetVertexShader( CompileShader( vs_4_0, VS() ) );\n"\
		"		SetGeometryShader( NULL );\n"\
		"		SetPixelShader( CompileShader( ps_4_0, PS() ) );\n"\
		"	}\n"\
		"}\n";

	CD3D11ParallaxMapRenderer::CD3D11ParallaxMapRenderer(
		ID3D11Device* device, video::IVideoDriver* driver, 
		s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial)
		: CD3D11ShaderMaterialRenderer(device, driver, 0, baseMaterial),
		CurrentScale(0.0f)
	{
		#ifdef _DEBUG
		setDebugName("CD3D11ParallaxMapRenderer");
		#endif
	
		// set this as callback. We could have done this in
		// the initialization list, but some compilers don't like it.
		CallBack = this;

		HRESULT hr = S_OK;
		ZeroMemory(&PassDescription, sizeof(D3DX11_PASS_DESC));

		video::IMaterialRenderer* renderer = driver->getMaterialRenderer(EMT_PARALLAX_MAP_SOLID);
		if(renderer)
		{
			// Reuse effect if already exists
			Effect = ((video::CD3D11ParallaxMapRenderer*)renderer)->Effect;
			if(Effect)
				Effect->AddRef();
		}
		else
		{
			if(!init(PARALLAX_MAP_SHADER))
				return;
		}

		if(Effect)
		{
			Technique = Effect->GetTechniqueByName("ParallaxMapTechnique");
			Technique->GetPassByIndex(0)->GetDesc(&PassDescription);

			WorldMatrix = Effect->GetVariableByName("g_mWorld")->AsMatrix();
			WorldViewProjMatrix = Effect->GetVariableByName("g_mWorldViewProj")->AsMatrix();
			LightPos1 = Effect->GetVariableByName("g_lightPos1")->AsVector();
			LightColor1 = Effect->GetVariableByName("g_lightColor1")->AsVector();
			LightPos2 = Effect->GetVariableByName("g_lightPos2")->AsVector();
			LightColor2 = Effect->GetVariableByName("g_lightColor2")->AsVector();
			ScaleFactor = Effect->GetVariableByName("g_scaleFactor")->AsVector();
			EyePosition = Effect->GetVariableByName("g_eyePosition")->AsVector();

			outMaterialTypeNr = Driver->addMaterialRenderer(this);
		}
	}

	CD3D11ParallaxMapRenderer::~CD3D11ParallaxMapRenderer()
	{
		if (CallBack == this)
			CallBack = 0;

		if(Effect)
			Effect->Release();
	}

	bool CD3D11ParallaxMapRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		if (vtxtype != video::EVT_TANGENTS)
		{
			os::Printer::log("Error: Parallax map renderer only supports vertices of type EVT_TANGENTS", ELL_ERROR);
			return false;
		}

		return CD3D11ShaderMaterialRenderer::OnRender(service, vtxtype);
	}

	//! Returns the render capability of the material.
	s32 CD3D11ParallaxMapRenderer::getRenderCapability() const
	{
		if (Driver->queryFeature(video::EVDF_PIXEL_SHADER_4_0) &&
			Driver->queryFeature(video::EVDF_VERTEX_SHADER_4_0))
			return 0;

		return 1;
	}

	void CD3D11ParallaxMapRenderer::OnSetConstants( IMaterialRendererServices* services, s32 userData )
	{
		// Set matrices
		WorldMatrix->SetMatrix((float*)Driver->getTransform(video::ETS_WORLD).pointer());

		core::matrix4 mat = Driver->getTransform(video::ETS_PROJECTION);
		mat *= Driver->getTransform(video::ETS_VIEW);
		mat *= Driver->getTransform(video::ETS_WORLD);
		WorldViewProjMatrix->SetMatrix((float*)mat.pointer());

		f32 floats[4] = {0,0,0,1};
		core::matrix4 minv = Driver->getTransform(video::ETS_VIEW);
		minv.makeInverse();
		minv.multiplyWith1x4Matrix(floats);
		EyePosition->SetFloatVector(reinterpret_cast<float*>(floats));

		// here we've got to fetch the fixed function lights from the
		// driver and set them as constants
		u32 cnt = Driver->getDynamicLightCount();

		SLight light;

		if(cnt >= 1)
			light = Driver->getDynamicLight(0);	
		else
		{
			light.DiffuseColor.set(0,0,0); // make light dark
			light.Radius = 1.0f;
		}

		light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

		LightPos1->SetFloatVector(reinterpret_cast<float*>(&light.Position));
		LightColor1->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));

		if(cnt >= 2)
			light = Driver->getDynamicLight(1);
		else
		{
			light = SLight();
			light.DiffuseColor.set(0,0,0); // make light dark
			light.Radius = 1.0f;
		}

		light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

		LightPos2->SetFloatVector(reinterpret_cast<float*>(&light.Position));
		LightColor2->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));


		// set scale factor
		f32 factor = 0.02f; // default value
		if (CurrentScale != 0)
			factor = CurrentScale;

		f32 scale[] = {factor, factor, factor, 0};

		ScaleFactor->SetFloatVector(reinterpret_cast<float*>(scale));

		Technique->GetPassByIndex(0)->Apply(0, Context);
	}

	void* CD3D11ParallaxMapRenderer::getShaderByteCode() const
	{
		return PassDescription.pIAInputSignature;
	}

	irr::u32 CD3D11ParallaxMapRenderer::getShaderByteCodeSize() const
	{
		return PassDescription.IAInputSignatureSize;
	}

	bool CD3D11ParallaxMapRenderer::init( const char* shader )
	{
		// Create effect if this is first
		UINT flags = 0;
		//flags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
#ifdef _DEBUG
		// These values allow use of PIX and shader debuggers
		flags |= D3D10_SHADER_DEBUG;
		flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#else
		// These flags allow maximum performance
		flags |= D3D10_SHADER_ENABLE_STRICTNESS;
		flags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
#endif
		ID3DBlob* ppCode = NULL;
		ID3DBlob* ppErrors = NULL;


		HRESULT hr = D3DCompile(PARALLAX_MAP_SHADER, strlen(PARALLAX_MAP_SHADER), "", NULL, NULL, NULL, "fx_5_0", flags, 2, &ppCode, &ppErrors );
		if (FAILED(hr))
		{
			core::stringc errorMsg = "Error, could not compile parallax map effect";
			if (ppErrors)
			{
				errorMsg += ": ";
				errorMsg += static_cast<const char*>(ppErrors->GetBufferPointer());
				ppErrors->Release();
			}
			os::Printer::log(errorMsg.c_str(), ELL_ERROR);
			return false;
		}

		hr = D3DX11CreateEffectFromMemory( ppCode->GetBufferPointer(), ppCode->GetBufferSize(), flags, Device, &Effect );

		if (FAILED(hr))
		{
			os::Printer::log("Error, could not create normal map effect", ELL_ERROR);
			return false;
		}

		return true;
	}

	void CD3D11ParallaxMapRenderer::OnSetMaterial( const SMaterial& material )
	{
		CurrentScale = material.MaterialTypeParam;

		CurrentMaterial = material;
	}

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_11_
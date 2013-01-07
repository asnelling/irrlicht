// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "CD3D11NormalMapRenderer.h"
#include "IVideoDriver.h"
#include "IMaterialRendererServices.h"
#include "os.h"
#include "SLight.h"

namespace irr
{
namespace video  
{
	const char NORMAL_MAP_SHADER[] = 
		"// adding constant buffer for transform matrices\n"\
		"cbuffer cbPerFrame : register(c0)\n"\
		"{\n"\
		"   float4x4 g_mWorld;\n"\
		"   float4x4 g_mWorldViewProj;\n"\
		"	float3	 g_lightPos1;\n"\
		"	float4	 g_lightColor1;\n"\
		"	float3	 g_lightPos2;\n"\
		"	float4	 g_lightColor2;\n"\
		"};\n"\
		"\n"\
		"cbuffer cbConsts : register(c1)\n"\
		"{\n"\
		"	float4 zeros = float4(0, 0, 0, 0);\n"\
		"	float3 positiveHalf = float3(0.5, 0.5, 0.5);\n"\
		"	float4 minusHalf = float4(-0.5, -0.5, -0.5, -0.5);\n"\
		"	float3 ones = float3(1, 1, 1);\n"\
		"	float3 twos = float3(2, 2, 2);\n"\
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
		"};\n"\
		"\n"\
		"PS_INPUT VS(VS_INPUT input)\n"\
		"{\n"\
		"	PS_INPUT output = (PS_INPUT)0;\n"\
		"\n"\
		"	// transform position to clip space\n"\
		"	output.pos = mul( input.pos, g_mWorldViewProj );\n"\
		"\n"\
		"// transform normal, tangent and binormal\n"\
		"	float3 tangent = mul( input.tangent, (float3x3)g_mWorld );\n"\
		"	float3 binormal = mul( input.binormal, (float3x3)g_mWorld );\n"\
		"	float3 normal = mul( input.norm, (float3x3)g_mWorld );\n"\
		"\n"\
		"	// transform vertex into world position\n"\
		"	float3 worldPos = mul( input.pos, g_mWorld ).xyz;\n"\
		"	float3 lightDir1 = worldPos - g_lightPos1;\n"\
		"	float3 lightDir2 = worldPos - g_lightPos2;\n"\
		"\n"\
		"	// transform light vectors with U, V, W\n"\
		"	float3x3 TBN = float3x3( tangent, binormal, normal );\n"\
		"	float3 lightVector1 = normalize( mul( lightDir1, TBN ) );\n"\
		"	float3 lightVector2 = normalize( mul( lightDir2, TBN ) );\n"\
		"\n"\
		"	// move light vectors from -1..1 into 0..1\n"\
		"	output.lightVector1 = lightVector1 * positiveHalf + positiveHalf;\n"\
		"	output.lightVector2 = lightVector2 * positiveHalf + positiveHalf;\n"\
		"\n"\
		"	// calculate attenuation of lights\n"\
		"	lightDir1.x = dot( lightDir1, lightDir1 ) * g_lightColor1.a;\n"\
		"	lightDir1 = rsqrt(lightDir1.x);\n"\
		"	output.lightColor1 = mul( lightDir1, g_lightColor1);\n"\
		"\n"\
		"	lightDir2.x = dot( lightDir2, lightDir2 ) * g_lightColor2.a;\n"\
		"	lightDir2 = rsqrt(lightDir2.x);\n"\
		"	output.lightColor2 = mul( lightDir2, g_lightColor2);\n"\
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
		"	// sample textures\n"\
		"	float4 colorMap = g_tex1.Sample( g_sampler1, input.colorMapCoord ).argb;\n"\
		"	float4 normalMap = g_tex2.Sample( g_sampler1, input.normalMapCoord ).argb;\n"\
		"\n"\
		"	normalMap = normalize(normalMap + minusHalf);"
		"	float4 lightVector1 = normalize(float4(input.lightVector1, 1.f) + minusHalf);"
		"	float4 lightVector2 = normalize(float4(input.lightVector2, 1.f) + minusHalf);"
		""
		"	lightVector1 = dot(lightVector1, normalMap);"
		"	lightVector1 = max(lightVector1, zeros);"
		"	lightVector1 = mul(lightVector1, input.lightColor1);"
		""
		"	lightVector2 = dot(lightVector2, normalMap);"
		"	lightVector2 = max(lightVector2, zeros);"
		""
		"	lightVector1 = mad(lightVector2, input.lightColor1, lightVector1);"
		""
		"	lightVector1 = lightVector1 * colorMap;"
		""
		"	lightVector1.a = input.lightColor1.a;"
		"	return lightVector1;"
		"}\n"\
		"\n"\
		"// Technique for standard vertex type\n"\
		"technique11 NormalMapTechnique\n"\
		"{\n"\
		"	pass p0\n"\
		"	{\n"\
		"		SetVertexShader( CompileShader( vs_4_0, VS() ) );\n"\
		"		SetGeometryShader( NULL );\n"\
		"		SetPixelShader( CompileShader( ps_4_0, PS() ) );\n"\
		"	}\n"\
		"}\n";

	CD3D11NormalMapRenderer::CD3D11NormalMapRenderer(ID3D11Device* device, video::IVideoDriver* driver, s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial)
		: CD3D11MaterialRenderer(device, driver, baseMaterial), 
		Effect(0), Technique(0), 
		WorldMatrix(0), WorldViewProjMatrix(0), 
		LightPos1(0), LightColor1(0), LightPos2(0), LightColor2(0)
	{
#ifdef _DEBUG
		setDebugName("CD3D11NormalMapRenderer");
#endif
		HRESULT hr = S_OK;
		ZeroMemory(&PassDescription, sizeof(D3DX11_PASS_DESC));

		video::IMaterialRenderer* renderer = driver->getMaterialRenderer(EMT_NORMAL_MAP_SOLID);
		if(renderer)
		{
			// Reuse effect if already exists
			Effect = ((video::CD3D11NormalMapRenderer*)renderer)->Effect;
			if(Effect)
				Effect->AddRef();
		}
		else
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

			HRESULT hr = D3DCompile(NORMAL_MAP_SHADER, strlen(NORMAL_MAP_SHADER), "", NULL, NULL, NULL, "fx_5_0", flags, 2, &ppCode, &ppErrors );
			if (FAILED(hr))
			{
				core::stringc errorMsg = "Error, could not compile normal map effect";
				if (ppErrors)
				{
					errorMsg += ": ";
					errorMsg += static_cast<const char*>(ppErrors->GetBufferPointer());
					ppErrors->Release();
				}
				os::Printer::log(errorMsg.c_str(), ELL_ERROR);
				return;
			}

			hr = D3DX11CreateEffectFromMemory( ppCode->GetBufferPointer(), ppCode->GetBufferSize(), flags, Device, &Effect );

			if (FAILED(hr))
			{
				os::Printer::log("Error, could not create normal map effect", ELL_ERROR);
				return;
			}
		}

		if(Effect)
		{
			Technique = Effect->GetTechniqueByName("NormalMapTechnique");
			Technique->GetPassByIndex(0)->GetDesc(&PassDescription);
			WorldMatrix = Effect->GetVariableByName("g_mWorld")->AsMatrix();
			WorldViewProjMatrix = Effect->GetVariableByName("g_mWorldViewProj")->AsMatrix();
			LightPos1 = Effect->GetVariableByName("g_lightPos1")->AsVector();
			LightColor1 = Effect->GetVariableByName("g_lightColor1")->AsVector();
			LightPos2 = Effect->GetVariableByName("g_lightPos2")->AsVector();
			LightColor2 = Effect->GetVariableByName("g_lightColor2")->AsVector();

			outMaterialTypeNr = Driver->addMaterialRenderer(this);
		}
	}

	CD3D11NormalMapRenderer::~CD3D11NormalMapRenderer()
	{
		SAFE_RELEASE(Effect);
	}

	bool CD3D11NormalMapRenderer::setVariable(const c8* name, const f32* floats, int count)
	{
		os::Printer::log("Could not set variable on normal map material renderer");
		return false;
	}

	void CD3D11NormalMapRenderer::OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
						bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		BaseRenderer->OnSetMaterial(material, lastMaterial, resetAllRenderstates, services);
	}

	bool CD3D11NormalMapRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		if (vtxtype != video::EVT_TANGENTS)
		{
			os::Printer::log("Error: Normal map renderer only supports vertices of type EVT_TANGENTS", ELL_ERROR);
			return false;
		}

		// Set matrices
		WorldMatrix->SetMatrix((float*)Driver->getTransform(video::ETS_WORLD).pointer());

		core::matrix4 mat = Driver->getTransform(video::ETS_PROJECTION);
		mat *= Driver->getTransform(video::ETS_VIEW);
		mat *= Driver->getTransform(video::ETS_WORLD);
		WorldViewProjMatrix->SetMatrix((float*)mat.pointer());

		// here we've got to fetch the fixed function lights from the
		// driver and set them as constants
		u32 cnt = Driver->getDynamicLightCount();

		if(cnt >= 2)
		{
			SLight light = Driver->getDynamicLight(0);	
			light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

			LightPos1->SetFloatVector(reinterpret_cast<float*>(&light.Position));
			LightColor1->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));

			light = Driver->getDynamicLight(1);	
			light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation
			LightPos2->SetFloatVector(reinterpret_cast<float*>(&light.Position));
			LightColor2->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));
		}
		else
		{
			SLight light;
			light.DiffuseColor.set(0,0,0); // make light dark
			light.Radius = 1.0f;

			light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

			LightPos1->SetFloatVector(reinterpret_cast<float*>(&light.Position.X));
			LightColor1->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));
			LightPos2->SetFloatVector(reinterpret_cast<float*>(&light.Position.X));
			LightColor2->SetFloatVector(reinterpret_cast<float*>(&light.DiffuseColor));
		}

		// Apply effect
		Technique->GetPassByIndex(0)->Apply(0, ImmediateContext );

		return true;
	}

	void CD3D11NormalMapRenderer::OnUnsetMaterial()
	{
		BaseRenderer->OnUnsetMaterial();
	}

	//! Returns the render capability of the material.
	s32 CD3D11NormalMapRenderer::getRenderCapability() const
	{
		return 0;
	}

	//! get shader signature
	void* CD3D11NormalMapRenderer::getShaderByteCode() const
	{
		return PassDescription.pIAInputSignature;
	}

	//! get shader signature size
	u32 CD3D11NormalMapRenderer::getShaderByteCodeSize() const
	{
		return PassDescription.IAInputSignatureSize;
	}

	bool CD3D11NormalMapRenderer::isTransparent() const
	{
		return BaseRenderer->isTransparent();
	}
}
}

#endif
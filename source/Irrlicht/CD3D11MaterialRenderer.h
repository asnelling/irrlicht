
#ifndef __C_D3D11_FIXED_FUNCTION_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D11_FIXED_FUNCTION_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#include "CD3D11Driver.h"
#include "CD3D11ShaderMaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"

#include "os.h"
#include "irrString.h"
#include "CReadFile.h"

#include <d3dx11effect.h>
#include <D3Dcompiler.h>

namespace irr
{
namespace video
{

namespace
{

struct SCENE_MATERIAL
{
	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;
	D3DXVECTOR4 Emissive;
	float		Shininess;
	int			Type;	// video::E_MATERIAL_TYPE
};

struct SCENE_LIGHT
{
	D3DXVECTOR4 Position;
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;
	D3DXVECTOR4 Ambient;
	D3DXVECTOR4 Atten;
};

D3DXMATRIX UnitMatrixD3D11;
D3DXMATRIX SphereMapMatrixD3D11;

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
}

const char FIXED_FUNCTION_SHADER[] =
	"// Constants\n"\
	"#define MAX_LIGHTS 								8\n"\
	"#define MAX_TEXTURES 								8\n"\
	"\n"\
	"#define FOGMODE_NONE    							0\n"\
	"#define FOGMODE_LINEAR  							1\n"\
	"#define FOGMODE_EXP     							2\n"\
	"#define FOGMODE_EXP2    							3\n"\
	"#define E 										2.71828\n"\
	"#define USE_ALPHA_TEST 							0\n"\
	"#define USE_CLIPPLANES 							0	// disable clip planes by now\n"\
	"#define USE_FOG 									0	// disable fog by now\n"\
	"\n"\
	"// Material types\n"\
	"#define EMT_SOLID									0\n"\
	"#define EMT_SOLID_2_LAYER							1\n"\
	"#define EMT_LIGHTMAP								2\n"\
	"#define EMT_LIGHTMAP_ADD							3\n"\
	"#define EMT_LIGHTMAP_M2							4\n"\
	"#define EMT_LIGHTMAP_M4							5\n"\
	"#define EMT_LIGHTMAP_LIGHTING						6\n"\
	"#define EMT_LIGHTMAP_LIGHTINM2						7\n"\
	"#define EMT_LIGHTMAP_LIGHTINM4						8\n"\
	"#define EMT_DETAIL_MAP								9\n"\
	"#define EMT_SPHERE_MAP								10\n"\
	"#define EMT_REFLECTION_2_LAYER						11\n"\
	"#define EMT_TRANSPARENT_ADD_COLOR					12\n"\
	"#define EMT_TRANSPARENT_ALPHA_CHANNEL				13\n"\
	"#define EMT_TRANSPARENT_ALPHA_CHANNEL_REF			14\n"\
	"#define EMT_TRANSPARENT_VERTEX_ALPHA				15\n"\
	"#define EMT_TRANSPARENT_REFLECTION_2_LAYER			16\n"\
	"\n"\
	"// Normal mapping\n"\
	"#define EMT_NORMAL_MAP_SOLID						17\n"\
	"#define EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR		18\n"\
	"#define EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA	19\n"\
	"\n"\
	"// Parllax mapping\n"\
	"#define EMT_PARALLAX_MAP_SOLID						20\n"\
	"#define EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR		21\n"\
	"#define EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA	22\n"\
	"\n"\
	"#define EMT_ONETEXTURE_BLEND						23\n"\
	"\n"\
	"// adding needed structs\n"\
	"struct ColorsOutput\n"\
	"{\n"\
	"	float4 diffuse;\n"\
	"	float4 specular;\n"\
	"};\n"\
	"\n"\
	"struct Light\n"\
	"{\n"\
	"	float4 position;\n"\
	"	float4 diffuse;\n"\
	"	float4 specular;\n"\
	"	float4 ambient;\n"\
	"	float4 atten;\n"\
	"};\n"\
	"\n"\
	"struct Material\n"\
	"{\n"\
	"	float4 ambient;\n"\
	"	float4 diffuse;\n"\
	"	float4 specular;\n"\
	"	float4 emissive;\n"\
	"	float shininess;\n"\
	"	int type;\n"\
	"};\n"\
	"\n"\
	"struct VS_INPUT\n"\
	"{\n"\
	"	float4 pos		: POSITION;\n"\
	"	float3 norm		: NORMAL;\n"\
	"	float4 color	: COLOR;\n"\
	"	float2 tex0		: TEXCOORD0;\n"\
	"};\n"\
	"\n"\
	"struct VS_INPUT_2COORDS\n"\
	"{\n"\
	"	float4 pos		: POSITION;\n"\
	"	float3 norm		: NORMAL;\n"\
	"	float4 color	: COLOR;\n"\
	"	float2 tex0		: TEXCOORD0;\n"\
	"	float2 tex1		: TEXCOORD1;\n"\
	"};\n"\
	"\n"\
	"struct VS_INPUT_TANGENTS\n"\
	"{\n"\
	"	float4 pos		: POSITION;\n"\
	"	float3 norm		: NORMAL;\n"\
	"	float4 color	: COLOR;\n"\
	"	float2 tex0		: TEXCOORD0;\n"\
	"	float3 tangent	: TEXCOORD1;\n"\
	"	float3 binormal : TEXCOORD2;\n"\
	"};\n"\
	"\n"\
	"struct VS_OUTPUT\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float3 wPos		: TEXTURE1;         //world space pos\n"\
	"	float3 wNorm	: TEXTURE2;        //world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist	: FOGDISTANCE;\n"\
	"	float3 planeDist: SV_ClipDistance0;\n"\
	"};\n"\
	"\n"\
	"struct VS_OUTPUT_2COORDS\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float2 tex1		: TEXTURE1;\n"\
	"	float3 wPos		: TEXTURE2;         //world space pos\n"\
	"	float3 wNorm	: TEXTURE3;        //world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist	: FOGDISTANCE;\n"\
	"	float3 planeDist: SV_ClipDistance0;\n"\
	"};\n"\
	"\n"\
	"struct VS_OUTPUT_TANGENTS\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float3 tangent	: TANGENT0;\n"\
	"	float3 binormal : BINORMAL0;\n"\
	"	float3 wPos		: TEXTURE1;         //world space pos\n"\
	"	float3 wNorm	: TEXTURE2;        //world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist	: FOGDISTANCE;\n"\
	"	float3 planeDist: SV_ClipDistance0;\n"\
	"};\n"\
	"\n"\
	"struct PS_INPUT\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float3 wPos		: TEXTURE1;         //world space pos\n"\
	"	float3 wNorm	: TEXTURE2;        //world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist 	: FOGDISTANCE;\n"\
	"};\n"\
	"\n"\
	"struct PS_INPUT_2COORDS\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float2 tex1		: TEXTURE1;\n"\
	"	float3 wPos		: TEXTURE2;         //world space pos\n"\
	"	float3 wNorm	: TEXTURE3;        //world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist 	: FOGDISTANCE;\n"\
	"};\n"\
	"\n"\
	"struct PS_INPUT_TANGENTS\n"\
	"{\n"\
	"	float4 pos		: SV_Position;\n"\
	"	float2 tex0		: TEXTURE0;\n"\
	"	float3 tangent	: TANGENT0;\n"\
	"	float3 binormal	: BINORMAL0;\n"\
	"	float3 wPos		: TEXTURE1;			//world space pos\n"\
	"	float3 wNorm	: TEXTURE2;			//world space normal\n"\
	"	float4 colorD	: COLOR0;\n"\
	"	float4 colorS	: COLOR1;\n"\
	"	float fogDist	: FOGDISTANCE;\n"\
	"};\n"\
	"\n"\
	"// adding constant buffer for transform matrices\n"\
	"cbuffer cbPerFrame : register(c0)\n"\
	"{\n"\
	"	float4x4 mWorld;\n"\
	"	float4x4 mView;\n"\
	"	float4x4 mProj;\n"\
	"	float4x4 mTexture;\n"\
	"};\n"\
	"\n"\
	"// adding constant buffer for fog, clip planes and point draw\n"\
	"cbuffer cbPerTechnique : register(c1)\n"\
	"{\n"\
	"	bool enableAlpha = false;\n"\
	"\n"\
	"   bool enableLighting = false;\n"\
	"\n"\
	"   bool enableClipping = false;\n"\
	"\n"\
	"	bool enableFog = false;\n"\
	"\n"\
	"   bool enablePointScale = false;\n"\
	"   float pointScaleA;\n"\
	"   float pointScaleB;\n"\
	"   float pointScaleC;\n"\
	"   float pointSize;   \n"\
	"\n"\
	"	int fogMode = FOGMODE_NONE;\n"\
	"   float fogStart;\n"\
	"   float fogEnd;\n"\
	"   float fogDensity;\n"\
	"   float4 fogColor;\n"\
	"};\n"\
	"\n"\
	"// adding constant buffer for lightning information\n"\
	"cbuffer cbLights : register(c2)\n"\
	"{\n"\
	"	int lightCount=0; // Number of lights enabled\n"\
	"	float4 clipPlanes[3];\n"\
	"	Light lights[MAX_LIGHTS];\n"\
	"	Material material;\n"\
	"};\n"\
	"\n"\
	"// adding function to calculate lightning\n"\
	"ColorsOutput calcLighting( float3 worldNormal, float3 worldPos, float3 cameraPos, float4 vertexColour)\n"\
	"{\n"\
	"	ColorsOutput output = (ColorsOutput)0;\n"\
	"\n"\
	"	const int nLights = min(MAX_LIGHTS, lightCount); // Clamp to MAX_LIGHTS\n"\
	"\n"\
	"	for(int i=0; i<nLights; i++)\n"\
	"	{\n"\
	"		float3 toLight = lights[i].position.xyz - worldPos;\n"\
	"		float lightDist = length( toLight );\n"\
	"		float fAtten = 1.0/dot( lights[i].atten, float4(1,lightDist,lightDist*lightDist,0) );\n"\
	"		float3 lightDir = normalize( toLight );\n"\
	"		float3 halfAngle = normalize( normalize(-cameraPos) + lightDir );\n"\
	"\n"\
	"		float4 _Ambient = lights[i].ambient * material.ambient;\n"\
	"		float4 _Diffuse = lights[i].diffuse * material.diffuse * vertexColour;\n"\
	"		float4 _Specular = lights[i].specular * material.specular;\n"\
	"		float4 _Emissive = material.emissive;\n"\
	"\n"\
	"		output.diffuse += max(0,dot( lightDir, worldNormal ) * _Diffuse * fAtten) + _Ambient + _Emissive;\n"\
	"		output.specular += max(0,pow( dot( halfAngle, worldNormal ), 64 ) * _Specular * fAtten );\n"\
	"	}\n"\
	"\n"\
	"	return output;\n"\
	"}\n"\
	"\n"\
	"// adding function to calculate fog\n"\
	"float calcFogFactor( float d )\n"\
	"{\n"\
	"	float fogCoeff = 1.0;\n"\
	"\n"\
	"	if(!enableFog)\n"\
	"		return fogCoeff;\n"\
	"\n"\
	"	[branch]\n"\
	"	switch( fogMode )\n"\
	"	{\n"\
	"	case FOGMODE_LINEAR:\n"\
	"		fogCoeff = (fogEnd - d)/(fogEnd - fogStart);\n"\
	"		break;\n"\
	"	case FOGMODE_EXP:\n"\
	"		fogCoeff = 1.0 / pow( E, d*fogDensity );\n"\
	"		break;\n"\
	"	case FOGMODE_EXP2:\n"\
	"		fogCoeff = 1.0 / pow( E, d*d*fogDensity*fogDensity );\n"\
	"		break;\n"\
	"	};\n"\
	"\n"\
	"    return clamp( fogCoeff, 0, 1 );\n"\
	"}\n"\
	"\n"\
	"// adding vertex shader code\n"\
	"VS_OUTPUT standardVS( VS_INPUT input )\n"\
	"{\n"\
	"	VS_OUTPUT output = (VS_OUTPUT)0;\n"\
	"\n"\
	"	//output our final position in clipspace\n"\
	"	float4 worldPos = mul( input.pos, mWorld );\n"\
	"	float4 cameraPos = mul( worldPos, mView ); //Save cameraPos for fog calculations\n"\
	"	output.pos = mul( cameraPos, mProj );\n"\
	"\n"\
	"	output.wPos = worldPos;\n"\
	"\n"\
	"	//save the fog distance for later\n"\
	"	output.fogDist = distance(output.pos, cameraPos);\n"\
	"\n"\
	"   //find our clipping planes (fixed function clipping is done in world space)\n"\
	"	[branch]\n"\
	"	if( enableClipping )\n"\
	"   {\n"\
	"		worldPos.w = 1;\n"\
	"\n"\
	"		//calc the distance from the 3 clipping planes\n"\
	"       output.planeDist.x = dot( worldPos, clipPlanes[0] );\n"\
	"       output.planeDist.y = dot( worldPos, clipPlanes[1] );\n"\
	"       output.planeDist.z = dot( worldPos, clipPlanes[2] );\n"\
	"   }\n"\
	"	else\n"\
	"   {\n"\
	"       output.planeDist.x = 1;\n"\
	"       output.planeDist.y = 1;\n"\
	"       output.planeDist.z = 1;\n"\
	"	}\n"\
	"\n"\
	"	float4 diffuseSwizzle = input.color.bgra; // swizzle the input rgba channels into D3D11 order\n"\
	"\n"\
	"	[branch]\n"\
	"	if( enableLighting )\n"\
	"	{\n"\
	"		float3 worldNormal = normalize( mul( input.norm, (float3x3)mWorld ) );\n"\
	"		output.wNorm = worldNormal;\n"\
	"		ColorsOutput cOut = calcLighting( worldNormal, worldPos, cameraPos, diffuseSwizzle );\n"\
	"		output.colorD = cOut.diffuse;\n"\
	"		output.colorS = cOut.specular;\n"\
	"	}\n"\
	"	else\n"\
	"	{\n"\
	"		output.colorD = diffuseSwizzle;\n"\
	"	}\n"\
	"\n"\
	"	// propagate texture coordinate\n"\
	"	output.tex0 = input.tex0;\n"\
	"	return output;\n"\
	"}\n"\
	"\n"\
	"// adding vertex shader code\n"\
	"VS_OUTPUT_2COORDS coords2TVS( VS_INPUT_2COORDS input )\n"\
	"{\n"\
	"	VS_OUTPUT_2COORDS output = (VS_OUTPUT_2COORDS)0;\n"\
	"\n"\
	"	//output our final position in clipspace\n"\
	"	float4 worldPos = mul( input.pos, mWorld );\n"\
	"	float4 cameraPos = mul( worldPos, mView ); //Save cameraPos for fog calculations\n"\
	"	output.pos = mul( cameraPos, mProj );\n"\
	"\n"\
	"	output.wPos = worldPos;\n"\
	"\n"\
	"	//save the fog distance for later\n"\
	"	output.fogDist = distance(output.pos, cameraPos);\n"\
	"\n"\
	"   //find our clipping planes (fixed function clipping is done in world space)\n"\
	"	[branch]\n"\
	"   if( enableClipping )\n"\
	"   {\n"\
	"       worldPos.w = 1;\n"\
	"\n"\
	"       //calc the distance from the 3 clipping planes\n"\
	"       output.planeDist.x = dot( worldPos, clipPlanes[0] );\n"\
	"       output.planeDist.y = dot( worldPos, clipPlanes[1] );\n"\
	"       output.planeDist.z = dot( worldPos, clipPlanes[2] );\n"\
	"   }\n"\
	"	else\n"\
	"   {\n"\
	"       output.planeDist.x = 1;\n"\
	"       output.planeDist.y = 1;\n"\
	"       output.planeDist.z = 1;\n"\
	"   }\n"\
	"\n"\
	"	float4 diffuseSwizzle = input.color.bgra; // swizzle the input rgba channels into D3D10 order\n"\
	"\n"\
	"	[branch]\n"\
	"	if( enableLighting )\n"\
	"	{\n"\
	"		float3 worldNormal = normalize( mul( input.norm, (float3x3)mWorld ) );\n"\
	"		output.wNorm = worldNormal;\n"\
	"		ColorsOutput cOut = calcLighting( worldNormal, worldPos, cameraPos, diffuseSwizzle );\n"\
	"		output.colorD = cOut.diffuse;\n"\
	"		output.colorS = cOut.specular;\n"\
	"	}\n"\
	"	else\n"\
	"	{\n"\
	"		output.colorD = diffuseSwizzle;\n"\
	"	}\n"\
	"\n"\
	"	// propagate texture coordinate\n"\
	"	output.tex0 = input.tex0;\n"\
	"	output.tex1 = input.tex1;\n"\
	"	return output;\n"\
	"}\n"\
	"\n"\
	"// adding vertex shader code\n"\
	"VS_OUTPUT_TANGENTS tangentsVS( VS_INPUT_TANGENTS input )\n"\
	"{\n"\
	"	VS_OUTPUT_TANGENTS output = (VS_OUTPUT_TANGENTS)0;\n"\
	"\n"\
	"	//output our final position in clipspace\n"\
	"	float4 worldPos = mul( input.pos, mWorld );\n"\
	"	float4 cameraPos = mul( worldPos, mView ); //Save cameraPos for fog calculations\n"\
	"	output.pos = mul( cameraPos, mProj );\n"\
	"\n"\
	"	output.wPos = worldPos;\n"\
	"\n"\
	"	//save the fog distance for later\n"\
	"	output.fogDist = distance(output.pos, cameraPos);\n"\
	"\n"\
	"   //find our clipping planes (fixed function clipping is done in world space)\n"\
	"	[branch]\n"\
	"   if( enableClipping )\n"\
	"   {\n"\
	"       worldPos.w = 1;\n"\
	"\n"\
	"       //calc the distance from the 3 clipping planes\n"\
	"       output.planeDist.x = dot( worldPos, clipPlanes[0] );\n"\
	"       output.planeDist.y = dot( worldPos, clipPlanes[1] );\n"\
	"       output.planeDist.z = dot( worldPos, clipPlanes[2] );\n"\
	"   }\n"\
	"	else\n"\
	"   {\n"\
	"       output.planeDist.x = 1;\n"\
	"       output.planeDist.y = 1;\n"\
	"       output.planeDist.z = 1;\n"\
	"   }\n"\
	"\n"\
	"	float4 diffuseSwizzle = input.color.bgra; // swizzle the input rgba channels into D3D10 order\n"\
	"\n"\
	"	[branch]\n"\
	"	if( enableLighting )\n"\
	"	{\n"\
	"		float3 worldNormal = normalize( mul( input.norm, (float3x3)mWorld ) );\n"\
	"		output.wNorm = worldNormal;\n"\
	"		ColorsOutput cOut = calcLighting( worldNormal, worldPos, cameraPos, diffuseSwizzle );\n"\
	"		output.colorD = cOut.diffuse;\n"\
	"		output.colorS = cOut.specular;\n"\
	"	}\n"\
	"	else\n"\
	"	{ \n"\
	"		output.colorD = diffuseSwizzle;\n"\
	"	}\n"\
	"\n"\
	"	// propagate texture coordinate\n"\
	"	output.tex0 = input.tex0;\n"\
	"	output.tangent = input.tangent;\n"\
	"	output.binormal = input.binormal;\n"\
	"	return output;\n"\
	"}\n"\
	"\n"\
	"// adding textures and samplers\n"\
	"Texture2D tex1 : register(t0);\n"\
	"Texture2D tex2 : register(t1);\n"\
	"\n"\
	"SamplerState sampler1 : register(s0);\n"\
	"SamplerState sampler2 : register(s1);\n"\
	"\n"\
	"// adding pixel shader\n"\
	"float4 standardPS( PS_INPUT input ) : SV_Target\n"\
	"{\n"\
	"	float4 normalColor = float4(0, 0, 0, 1);\n"\
	"	float4 tex1C = tex1.Sample( sampler1, input.tex0 ).bgra;\n"\
	"	float4 tex2C = tex2.Sample( sampler2, input.tex0 ).bgra;\n"\
	"\n"\
	"	// check material type, and add color operation correctly\n"\
	"	[branch]\n"\
	"	switch(material.type)\n"\
	"	{\n"\
	"	case EMT_SOLID:\n"\
	"		normalColor = (tex1C * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"   case EMT_SOLID_2_LAYER:\n"\
	"		normalColor = lerp(tex1C, tex2C, tex1C.a) * input.colorD + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP:\n"\
	"		normalColor = tex1C * tex2C;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_ADD:\n"\
	"		normalColor = tex1C + tex2C;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_M2:\n"\
	"		normalColor = (tex1C * tex2C) * 2;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_M4:\n"\
	"		normalColor = (tex1C * tex2C) * 4;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTING:\n"\
	"		normalColor = tex1C * tex2C;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTINM2:\n"\
	"		normalColor = (tex1C * tex2C) * 2;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTINM4:\n"\
	"		normalColor = (tex1C * tex2C) * 4;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_DETAIL_MAP:\n"\
	"		normalColor = (tex1C + tex2C) - 0.5;\n"\
	"		break;\n"\
	"	case EMT_SPHERE_MAP:							// TODO\n"\
	"		normalColor = tex1C;//tex1C * normalize(reflect(-normalize(input.pos - input.wPos), normalize(input.wNorm)));\n"\
	"		//normalColor = (tex1C * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_REFLECTION_2_LAYER:					// TODO\n"\
	"		normalColor = (tex1C * tex2C);\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_ADD_COLOR:					// TODO\n"\
	"		normalColor = (tex1C * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_VERTEX_ALPHA:\n"\
	"		//normalColor = tex1C + input.colorS;\n"\
	"		normalColor = float4(tex1C.bgr,input.colorS.a );\n"\
	"		//normalColor = (float4(tex1C.rgb, input.colorD.a) /** input.colorD*/) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_ALPHA_CHANNEL:\n"\
	"		normalColor =  tex1C * input.colorD + input.colorS;\n"\
	"		normalColor.a =  tex1C.a;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_REFLECTION_2_LAYER:		// TODO\n"\
	"		normalColor = tex1C + input.colorS;;\n"\
	"		break;\n"\
	"	case EMT_ONETEXTURE_BLEND:						// TODO\n"\
	"		normalColor = tex1C /* input.colorD*/ + input.colorS;\n"\
	"		break;\n"\
	"	default:\n"\
	"		normalColor = tex1C;\n"\
	"		break;\n"\
	"	};\n"\
	"\n"\
	"	// fog calculation\n"\
	"	float fog = calcFogFactor( input.fogDist );\n"\
	"	normalColor = fog * normalColor + (1.0 - fog)*fogColor;\n"\
	"\n"\
	"	// return color in pixel shader\n"\
	"	return normalColor;\n"\
	"}\n"\
	"\n"\
	"float4 coords2TPS( PS_INPUT_2COORDS input ) : SV_Target\n"\
	"{\n"\
	"	float4 normalColor = float4(0, 0, 0, 1);\n"\
	"	float4 tex1C = tex1.Sample( sampler1, input.tex0 ).bgra;\n"\
	"	float4 tex2C = tex2.Sample( sampler2, input.tex1 ).bgra;\n"\
	"\n"\
	"	// check material type, and add color operation correctly\n"\
	"	[branch]\n"\
	"	switch(material.type)\n"\
	"	{\n"\
	"	case EMT_SOLID:\n"\
	"		normalColor = (tex1C * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_SOLID_2_LAYER:\n"\
	"		normalColor = lerp(tex1C, tex2C, tex1C.a) * input.colorD + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP:\n"\
	"		normalColor = tex1C * tex2C;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_ADD:\n"\
	"		normalColor = tex1C + tex2C;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_M2:\n"\
	"		normalColor = (tex1C * tex2C) * 2;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_M4:\n"\
	"		normalColor = (tex1C * tex2C) * 4;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTING:\n"\
	"		normalColor = tex1C * tex2C;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTINM2:\n"\
	"		normalColor = (tex1C * tex2C) * 2;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_LIGHTMAP_LIGHTINM4:\n"\
	"		normalColor = (tex1C * tex2C) * 4;\n"\
	"		normalColor = (normalColor * input.colorD) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_DETAIL_MAP:\n"\
	"		normalColor = (tex1C + tex2C) - 0.5;\n"\
	"		break;\n"\
	"	case EMT_SPHERE_MAP:							// TODO\n"\
	"	case EMT_REFLECTION_2_LAYER:					// TODO\n"\
	"	case EMT_TRANSPARENT_ADD_COLOR:					// TODO\n"\
	"		normalColor = (tex1C /** input.colorD*/) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_ALPHA_CHANNEL:				// TODO\n"\
	"	case EMT_TRANSPARENT_ALPHA_CHANNEL_REF:			// TODO\n"\
	"	case EMT_TRANSPARENT_VERTEX_ALPHA:\n"\
	"		normalColor = (tex1C /** input.colorD*/) + input.colorS;\n"\
	"		//normalColor = (float4(tex1C.rgb, input.colorD.a) /** input.colorD*/) + input.colorS;\n"\
	"		break;\n"\
	"	case EMT_TRANSPARENT_REFLECTION_2_LAYER:		// TODO\n"\
	"	case EMT_ONETEXTURE_BLEND:						// TODO\n"\
	"	default:\n"\
	"		normalColor = tex1C;\n"\
	"		break;\n"\
	"	}\n"\
	"\n"\
	"	// fog calculation\n"\
	"	float fog = calcFogFactor( input.fogDist );\n"\
	"	normalColor = fog * normalColor + (1.0 - fog)*fogColor;\n"\
	"\n"\
	"	// return color in pixel shader\n"\
	"	return normalColor;\n"\
	"}\n"\
	"\n"\
	"float4 tangentsPS( PS_INPUT_TANGENTS input ) : SV_Target\n"\
	"{\n"\
	"	float4 normalColor = float4(0, 0, 0, 1);\n"\
	"	float4 tex1C = tex1.Sample( sampler1, input.tex0 ).bgra;\n"\
	"\n"\
	"	// Only use EMT_SOLID for now\n"\
	"	normalColor = (tex1C * input.colorD) + input.colorS;\n"\
	"\n"\
	"	// fog calculation\n"\
	"	float fog = calcFogFactor( input.fogDist );\n"\
	"	normalColor = fog * normalColor + (1.0 - fog)*fogColor;\n"\
	"\n"\
	"	// return color in pixel shader\n"\
	"	return normalColor;\n"\
	"}\n"\
	"\n"\
	"// Technique for standard vertex type\n"\
	"technique11 standardTech\n"\
	"{\n"\
	"	pass p0\n"\
	"	{\n"\
	"		SetVertexShader( CompileShader( vs_4_0, standardVS() ) );\n"\
	"		SetGeometryShader( NULL );\n"\
	"		SetPixelShader( CompileShader( ps_4_0, standardPS() ) );\n"\
	"	}\n"\
	"}\n"\
	"\n"\
	"// Technique for 2 coords vertex type\n"\
	"technique11 coords2TTech\n"\
	"{\n"\
	"	pass p0\n"\
	"	{\n"\
	"		SetVertexShader( CompileShader( vs_4_0, coords2TVS() ) );\n"\
	"		SetGeometryShader( NULL );\n"\
	"		SetPixelShader( CompileShader( ps_4_0, coords2TPS() ) );\n"\
	"	}\n"\
	"}\n"\
	"\n"\
	"// Technique for tangents vertex type\n"\
	"technique11 tangentsTech\n"\
	"{\n"\
	"	pass p0\n"\
	"	{\n"\
	"		SetVertexShader( CompileShader( vs_4_0, tangentsVS() ) );\n"\
	"		SetGeometryShader( NULL );\n"\
	"		SetPixelShader( CompileShader( ps_4_0, tangentsPS() ) );\n"\
	"	}\n"\
	"}\n";

class CD3D11MaterialRenderer : public CD3D11ShaderMaterialRenderer, IShaderConstantSetCallBack
{
public:
	//! Constructor for fixed function effect
	CD3D11MaterialRenderer(ID3D11Device* device, IVideoDriver* driver)
		: CD3D11ShaderMaterialRenderer(device, driver, this, 0),
		Effect(0), StandardTechnique(0), Coords2TTechnique(0), TangentsTechnique(0), CurrentPass(0),
		WorldMatrix(0), ViewMatrix(0), ProjectionMatrix(0), TextureMatrix0(0),
		FogMode(0), FogStart(0), FogEnd(0), FogDensity(0), FogColor(0),
		EnableLightning(0),  LightCount(0), SceneLights(0),
		SceneMaterial(0), EnableClipping(0), ClipPlanes(0),
		EnablePointScale(0), PointScaleA(0), PointScaleB(0), PointScaleC(0), PointSize(0),
		Sampler1(0), Sampler2(0), Texture1(0), Texture2(0)
	{
		IMaterialRenderer* solidRend = Driver->getMaterialRenderer(EMT_SOLID);
		if (solidRend)
		{
			Effect = static_cast<CD3D11MaterialRenderer*>(solidRend)->Effect;

			Effect->AddRef();	
		}
		else
		{
			if (!init(FIXED_FUNCTION_SHADER))
				return;
		}

		// Get technique
		StandardTechnique = Effect->GetTechniqueByName("standardTech");
		Coords2TTechnique = Effect->GetTechniqueByName("coords2TTech");
		TangentsTechnique = Effect->GetTechniqueByName("tangentsTech");

		WorldMatrix = Effect->GetVariableByName("mWorld")->AsMatrix();
		ViewMatrix = Effect->GetVariableByName("mView")->AsMatrix();
		ProjectionMatrix = Effect->GetVariableByName("mProj")->AsMatrix();
		TextureMatrix0 = Effect->GetVariableByName("mTexture")->AsMatrix();

		EnableLightning = Effect->GetVariableByName("enableLighting")->AsScalar();
		LightCount = Effect->GetVariableByName("lightCount")->AsScalar();
	
		EnableFog = Effect->GetVariableByName("enableFog")->AsScalar();
		FogMode = Effect->GetVariableByName("fogMode")->AsScalar();
		FogStart = Effect->GetVariableByName("fogStart")->AsScalar();
		FogEnd = Effect->GetVariableByName("fogEnd")->AsScalar();
		FogDensity = Effect->GetVariableByName("fogDensity")->AsScalar();
		FogColor = Effect->GetVariableByName("fogColor")->AsVector();

		EnableClipping = Effect->GetVariableByName("enableClipping")->AsScalar();
		ClipPlanes = Effect->GetVariableByName("clipPlanes")->AsVector();
		SceneLights = Effect->GetVariableByName("lights");
		SceneMaterial = Effect->GetVariableByName("material");

		EnablePointScale = Effect->GetVariableByName("enablePointScale")->AsScalar();
		PointScaleA = Effect->GetVariableByName("pointScaleA")->AsScalar();
		PointScaleB = Effect->GetVariableByName("pointScaleB")->AsScalar();
		PointScaleC = Effect->GetVariableByName("pointScaleC")->AsScalar();
		PointSize = Effect->GetVariableByName("pointSize")->AsScalar();

		Texture1 = Effect->GetVariableByName("tex1")->AsShaderResource();
		Texture2 = Effect->GetVariableByName("tex2")->AsShaderResource();
	
		Sampler1 = Effect->GetVariableByName("sampler1")->AsSampler();
		Sampler2 = Effect->GetVariableByName("sampler2")->AsSampler();
	}

	~CD3D11MaterialRenderer()
	{
		if(Effect)
			Effect->Release();

		if(CallBack == this)
			CallBack = NULL;
	}

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		switch(vtxtype)
		{
		case EVT_2TCOORDS:
			CurrentPass = Coords2TTechnique->GetPassByIndex(0);
			break;
		case EVT_TANGENTS:
			CurrentPass = TangentsTechnique->GetPassByIndex(0);
			break;
		case EVT_STANDARD:
			CurrentPass = StandardTechnique->GetPassByIndex(0);
			break;
		default:
			os::Printer::log("Vertex type not supported by this Material Renderer", ELL_ERROR);
			CurrentPass = 0;
			return false;
		};

		return CD3D11ShaderMaterialRenderer::OnRender(service, vtxtype);
	}

	virtual void OnSetConstants(IMaterialRendererServices* service, s32 userData)
	{
		if (!Effect)
			return;

		// set fog
		SColor fogColor;
		video::E_FOG_TYPE fogMode;
		float fogStart = 0.0f;
		float fogEnd = 0.0f;
		float fogDensity = 0.0f;
		bool pixelFog = true;
		bool rangeFog = true;

		EnableFog->SetBool(CurrentMaterial.FogEnable);

		Driver->getFog( fogColor, fogMode, fogStart, fogEnd, fogDensity, pixelFog, rangeFog );
		SColorf fogColorF( fogColor );
		FogColor->SetFloatVector( &(fogColorF.r) );
		FogMode->SetInt( fogMode );
		FogStart->SetFloat( fogStart );
		FogEnd->SetFloat( fogEnd );
		FogDensity->SetFloat( fogDensity );

		// clipping
		EnableClipping->SetBool( true );	

		// set material
		SCENE_MATERIAL mtl;
		ZeroMemory(&mtl, sizeof(SCENE_MATERIAL));
		mtl.Ambient = colorToD3DXVECTOR4( CurrentMaterial.AmbientColor );
		mtl.Diffuse = colorToD3DXVECTOR4( CurrentMaterial.DiffuseColor );
		mtl.Specular = colorToD3DXVECTOR4( CurrentMaterial.SpecularColor );
		mtl.Emissive = colorToD3DXVECTOR4( CurrentMaterial.EmissiveColor );
		mtl.Type = CurrentMaterial.MaterialType;
		mtl.Shininess = CurrentMaterial.Shininess;
	
		SceneMaterial->SetRawValue( &mtl, 0, sizeof(SCENE_MATERIAL) );

		// set lightning
		bool light = CurrentMaterial.Lighting;
		
		EnableLightning->SetBool(light);

		if(light)
		{
			int count = Driver->getDynamicLightCount();

			LightCount->SetInt(count);
			if (count)
			{
				SCENE_LIGHT* lights = new SCENE_LIGHT[count];		
				ZeroMemory( lights, count*sizeof(SCENE_LIGHT) );

				for (int i=0; i<count; i++)
				{
					SCENE_LIGHT& l = lights[i];
					SLight dl = Driver->getDynamicLight(i);

					dl.Position.getAs4Values(l.Position);
					l.Ambient = D3DXVECTOR4(dl.AmbientColor.r, dl.AmbientColor.g, dl.AmbientColor.b,dl.AmbientColor.a);
					l.Diffuse = D3DXVECTOR4(dl.DiffuseColor.r, dl.DiffuseColor.g, dl.DiffuseColor.b,dl.DiffuseColor.a);
					l.Specular = D3DXVECTOR4(dl.SpecularColor.r, dl.SpecularColor.g, dl.SpecularColor.b,dl.SpecularColor.a);
					dl.Attenuation.getAs4Values(l.Atten);
				}
				SceneLights->SetRawValue( lights, 0, count*sizeof(SCENE_LIGHT) );

				delete[] lights;
			}
		}

		// apply transformations
		core::matrix4 mat = Driver->getTransform( video::ETS_PROJECTION );
		ProjectionMatrix->SetMatrix( mat.pointer() );
		mat = Driver->getTransform( video::ETS_VIEW );
		ViewMatrix->SetMatrix( mat.pointer() );
		mat = Driver->getTransform( video::ETS_WORLD );
		WorldMatrix->SetMatrix( mat.pointer() );
	
		// Apply pass
		if(CurrentPass)
			CurrentPass->Apply( 0, Context);
	}

	//! get shader signature
	virtual void* getShaderByteCode() const
	{
		D3DX11_PASS_DESC desc;
		if (CurrentPass)
		{
			CurrentPass->GetDesc( &desc );
			return desc.pIAInputSignature;
		}
	
		return 0;
	}

	//! get shader signature size
	virtual u32 getShaderByteCodeSize() const
	{
		D3DX11_PASS_DESC desc;
		if (CurrentPass)
		{
			CurrentPass->GetDesc( &desc );
			return desc.IAInputSignatureSize;
		}
	
		return 0;
	}

protected:
	// Effects and techniques
	ID3DX11Effect* Effect;
	ID3DX11EffectTechnique* StandardTechnique;
	ID3DX11EffectTechnique* Coords2TTechnique;
	ID3DX11EffectTechnique* TangentsTechnique;
	ID3DX11EffectPass* CurrentPass;

	// Transformation matrix variables
	ID3DX11EffectMatrixVariable* WorldMatrix;
	ID3DX11EffectMatrixVariable* ViewMatrix;
	ID3DX11EffectMatrixVariable* ProjectionMatrix;
	ID3DX11EffectMatrixVariable* TextureMatrix0;

	// Per technique variables
	ID3DX11EffectScalarVariable* EnableLightning;
	ID3DX11EffectScalarVariable* LightCount;
	ID3DX11EffectVariable* SceneLights;
		
	ID3DX11EffectVariable* SceneMaterial;
	ID3DX11EffectScalarVariable* EnableClipping;
	ID3DX11EffectVectorVariable* ClipPlanes;
		
	ID3DX11EffectScalarVariable* EnableFog;
	ID3DX11EffectScalarVariable* FogMode;
	ID3DX11EffectScalarVariable* FogStart;
	ID3DX11EffectScalarVariable* FogEnd;
	ID3DX11EffectScalarVariable* FogDensity;
	ID3DX11EffectVectorVariable* FogColor;
	
	// Samplers
	ID3DX11EffectSamplerVariable* Sampler1;
	ID3DX11EffectSamplerVariable* Sampler2;

	// Shader Resources
	ID3DX11EffectShaderResourceVariable* Texture1;
	ID3DX11EffectShaderResourceVariable* Texture2;

	// Point sprite
	ID3DX11EffectScalarVariable* EnablePointScale;
	ID3DX11EffectScalarVariable* PointScaleA;
	ID3DX11EffectScalarVariable* PointScaleB;
	ID3DX11EffectScalarVariable* PointScaleC;
	ID3DX11EffectScalarVariable* PointSize;

	bool init(const char* shader)
	{
		HRESULT hr = S_OK;

		UINT flags = 0;
		//flags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;
	#ifdef _DEBUG
		// These values allow use of PIX and shader debuggers
		flags |= D3D10_SHADER_DEBUG;
		flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
		//flags |= D3D10_SHADER_WARNINGS_ARE_ERRORS;
	#else
		// These flags allow maximum performance
		flags |= D3D10_SHADER_ENABLE_STRICTNESS;
		flags |= D3D10_SHADER_OPTIMIZATION_LEVEL3;
	#endif

		//Compile effect
		ID3DBlob* ppCode = NULL;
		ID3DBlob* ppErrors = NULL;

		hr = D3DCompile( shader, strlen(shader), "", NULL, NULL, NULL, "fx_5_0", flags, 0, &ppCode, &ppErrors );

		if (FAILED(hr))
		{
			core::stringc errorMsg = "Error, could not compile fixed function effect";
			if (ppErrors)
			{
				errorMsg += ": ";
				errorMsg += static_cast<const char*>(ppErrors->GetBufferPointer());
				ppErrors->Release();
			}
			os::Printer::log(errorMsg.c_str(), ELL_ERROR);
			return false;
		}

		// Create effect
		hr = D3DX11CreateEffectFromMemory( ppCode->GetBufferPointer(), ppCode->GetBufferSize(), flags, Device, &Effect );
		ppCode->Release();
		if (FAILED(hr))
		{
			os::Printer::log("Error, could not create fixed function effect", ELL_ERROR);
			return false;
		}

		return true;
	}

	virtual void OnSetMaterial( const SMaterial& material ) 
	{
		CurrentMaterial = material;
	}

};

class CD3D11MaterialRenderer_SOLID : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_SOLID(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();

		blendDesc.RenderTarget[0].BlendEnable = false;
	}
};

class CD3D11MaterialRenderer_SOLID_2_LAYER : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_SOLID_2_LAYER(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
	}
};

class CD3D11MaterialRenderer_LIGHTMAP : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_LIGHTMAP(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
	}
};
	
class CD3D11MaterialRenderer_DETAIL_MAP : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_DETAIL_MAP(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
		D3D11_SAMPLER_DESC* samplerDescs = static_cast<CD3D11Driver*>(Driver)->getSamplerDescs();
		
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		samplerDescs[0].AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescs[0].AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescs[0].Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDescs[1].AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescs[1].AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescs[1].Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	}
};

class CD3D11MaterialRenderer_SPHERE_MAP : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_SPHERE_MAP(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;
	
		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
	
		// set texture matrix
		TextureMatrix0->SetMatrix(reinterpret_cast<f32*>(&SphereMapMatrixD3D11));
	}

	virtual void OnUnsetMaterial()
	{
		TextureMatrix0->SetMatrix(reinterpret_cast<f32*>(&UnitMatrixD3D11));
	}	
};

class CD3D11MaterialRenderer_REFLECTION_2_LAYER : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_REFLECTION_2_LAYER(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = false;

		// set texture matrix
		TextureMatrix0->SetMatrix(reinterpret_cast<f32*>(&SphereMapMatrixD3D11));
	}

	virtual void OnUnsetMaterial()
	{
		TextureMatrix0->SetMatrix(reinterpret_cast<f32*>(&UnitMatrixD3D11));
	}
};

class CD3D11MaterialRenderer_TRANSPARENT_ADD_COLOR : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_TRANSPARENT_ADD_COLOR(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;
	
		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();	
			
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
 		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		D3D11_DEPTH_STENCIL_DESC& depth = static_cast<CD3D11Driver*>(Driver)->getDepthStencilDesc();
		depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	}

	//! Returns if the material is transparent. The scene management needs to know this
	//! for being able to sort the materials by opaque and transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

class CD3D11MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();

		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;	
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;			
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;		

		//FIXME: flames of torches in example 16 are wrong
	}

	//! Returns if the material is transparent. The scene managment needs to know this
	//! for being able to sort the materials by opaque and transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

class CD3D11MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
	}

	//! Returns if the material is transparent. The scene managment needs to know this
	//! for being able to sort the materials by opaque and transparent.
	virtual bool isTransparent() const
	{
		return false; // this material is not really transparent because it does no blending.
	}
};

class CD3D11MaterialRenderer_TRANSPARENT_VERTEX_ALPHA : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_TRANSPARENT_VERTEX_ALPHA(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;	
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;			
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;		
	}

	//! Returns if the material is transparent. The scene managment needs to know this
	//! for being able to sort the materials by opaque and transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

class CD3D11MaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER : public CD3D11MaterialRenderer
{
public:
	CD3D11MaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER(ID3D11Device* device, IVideoDriver* driver)
	: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
					bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		// Store current material type
		CurrentMaterial = material;

		D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();
			
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;	
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;			
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;	
	}

	//! Returns if the material is transparent. The scene managment needs to know this
	//! for being able to sort the materials by opaque and transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

class CD3D11MaterialRenderer_ONETEXTURE_BLEND : public CD3D11MaterialRenderer
{
public:

	CD3D11MaterialRenderer_ONETEXTURE_BLEND(ID3D11Device* device, IVideoDriver* driver)
		: CD3D11MaterialRenderer(device, driver) {}

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services)
	{
		services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		if (material.MaterialType != lastMaterial.MaterialType ||
			material.MaterialTypeParam != lastMaterial.MaterialTypeParam ||
			resetAllRenderstates)
		{
			D3D11_BLEND_DESC& blendDesc = static_cast<CD3D11Driver*>(Driver)->getBlendDesc();

			E_BLEND_FACTOR srcFact,dstFact;
			E_MODULATE_FUNC modulate;
			u32 alphaSource;
			unpack_textureBlendFunc ( srcFact, dstFact, modulate, alphaSource, material.MaterialTypeParam );

			if (srcFact == EBF_SRC_COLOR && dstFact == EBF_ZERO)
			{
				blendDesc.RenderTarget[0].BlendEnable = FALSE;
			}
			else
			{
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = getD3DBlend ( srcFact );	
				blendDesc.RenderTarget[0].DestBlend = getD3DBlend ( dstFact );	

				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			}
		}
	}

	//! Returns if the material is transparent.
	/** The scene management needs to know this for being able to sort the
	materials by opaque and transparent.
	The return value could be optimized, but we'd need to know the
	MaterialTypeParam for it. */
	virtual bool isTransparent() const
	{
		return true;
	}

	private:
		D3D11_BLEND getD3DBlend ( E_BLEND_FACTOR factor ) const
		{
			D3D11_BLEND r;
			switch ( factor )
			{
				case EBF_ZERO:					r = D3D11_BLEND_ZERO; break;
				case EBF_ONE:					r = D3D11_BLEND_ONE; break;
				case EBF_DST_COLOR:				r = D3D11_BLEND_DEST_COLOR; break;
				case EBF_ONE_MINUS_DST_COLOR:	r = D3D11_BLEND_INV_DEST_COLOR; break;
				case EBF_SRC_COLOR:				r = D3D11_BLEND_SRC_COLOR; break;
				case EBF_ONE_MINUS_SRC_COLOR:	r = D3D11_BLEND_INV_SRC_COLOR; break;
				case EBF_SRC_ALPHA:				r = D3D11_BLEND_SRC_ALPHA; break;
				case EBF_ONE_MINUS_SRC_ALPHA:	r = D3D11_BLEND_INV_SRC_ALPHA; break;
				case EBF_DST_ALPHA:				r = D3D11_BLEND_DEST_ALPHA; break;
				case EBF_ONE_MINUS_DST_ALPHA:	r = D3D11_BLEND_INV_DEST_ALPHA; break;
				case EBF_SRC_ALPHA_SATURATE:	r = D3D11_BLEND_SRC_ALPHA_SAT; break;
			}
			return r;
		}

		bool transparent;

};
}
}

#endif
#endif
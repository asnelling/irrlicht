// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_BURNING_SHADER_H_INCLUDED__
#define __I_BURNING_SHADER_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"
#include "IReferenceCounted.h"
#include "irrMath.h"
#include "IImage.h"
#include "S2DVertex.h"
#include "rect.h"
#include "CDepthBuffer.h"
#include "S4DVertex.h"
#include "irrArray.h"
#include "SLight.h"
#include "SMaterial.h"
#include "os.h"


namespace irr
{

namespace video
{

	struct SBurningShaderLight
	{
		//SLight org;
		bool LightIsOn;

		E_LIGHT_TYPE Type;
		f32 linearAttenuation;
		f32 constantAttenuation;
		f32 quadraticAttenuation;
		sVec4 pos;
		sVec4 pos4;

		sVec4 spotDirection;
		sVec4 spotDirection4;
		f32 spotCosCutoff;
		f32 spotCosInnerCutoff;
		f32 spotExponent;

		sVec3 AmbientColor;
		sVec3 DiffuseColor;
		sVec3 SpecularColor;
	};

	enum eTransformLightFlags
	{
		ENABLED		= 0x01,
		SPECULAR	= 0x08,
		FOG			= 0x10,
		NORMALIZE_NORMALS	= 0x20,
		TEXTURE_TRANSFORM	= 0x40,
	};

	//currently in world space
	struct SBurningShaderLightSpace
	{
		SBurningShaderLightSpace() {enabled = 0;}
		virtual ~SBurningShaderLightSpace() {}
		void reset ()
		{
			Light.set_used ( 0 );
			Global_AmbientLight.set ( 0.f, 0.f, 0.f );
			Flags = 0;
		}
		int enabled;
		core::array<SBurningShaderLight> Light;
		sVec3 Global_AmbientLight;
		sVec4 FogColor;

		sVec4 campos; //Camera Position in World Space
		sVec4 vertex;//world position of vertex
		sVec4 normal; //transformed normal

		u32 Flags; // eTransformLightFlags
	};

	struct SBurningShaderEyeSpace
	{
		SBurningShaderEyeSpace() {}
		virtual ~SBurningShaderEyeSpace() {}
		void reset ()
		{
			Light.set_used ( 0 );
			Global_AmbientLight.set ( 0.f, 0.f, 0.f );

			Flags = 0;
			campos.x = 0.f;
			campos.y = 0.f;
			campos.z = 0.f;
			campos.w = 1.f;
		}
		core::array<SBurningShaderLight> Light;
		sVec3 Global_AmbientLight;
		sVec4 FogColor;

		sVec4 campos; //Camera Position in eye Space
		sVec4 vertex4; //eye coordinate position of vertex
		sVec4 normal3; //transformed normal

		sVec4 vertex3; //eye coordinate position of vertex projected

		u32 Flags; // eTransformLightFlags
	};

	enum eCullFlag
	{
		CULL_FRONT = 1,
		CULL_BACK = 2,
		CULL_INVISIBLE = 4,
		CULL_FRONT_AND_BACK = 8,
	};

	struct SBurningShaderMaterial
	{
		SMaterial org;

		u32 Culling; //eCullFlag

		sVec3 AmbientColor;
		sVec3 DiffuseColor;
		sVec3 SpecularColor;
		sVec3 EmissiveColor;

	};

	enum EBurningFFShader
	{
		ETR_FLAT = 0,
		ETR_FLAT_WIRE,
		ETR_GOURAUD,
		ETR_GOURAUD_WIRE,
		ETR_TEXTURE_FLAT,
		ETR_TEXTURE_FLAT_WIRE,
		ETR_TEXTURE_GOURAUD,
		ETR_TEXTURE_GOURAUD_WIRE,
		ETR_TEXTURE_GOURAUD_NOZ,
		ETR_TEXTURE_GOURAUD_ADD,
		ETR_TEXTURE_GOURAUD_ADD_NO_Z,

		ETR_TEXTURE_GOURAUD_VERTEX_ALPHA,

		ETR_TEXTURE_GOURAUD_LIGHTMAP_M1,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_M2,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_M4,
		ETR_TEXTURE_LIGHTMAP_M4,

		ETR_TEXTURE_GOURAUD_DETAIL_MAP,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD,

		ETR_GOURAUD_NOZ,
		ETR_GOURAUD_ALPHA,
		ETR_GOURAUD_ALPHA_NOZ_NOPERSPECTIVE_CORRECT,

		ETR_TEXTURE_GOURAUD_ALPHA,
		ETR_TEXTURE_GOURAUD_ALPHA_NOZ,
		ETR_TEXTURE_GOURAUD_ALPHA_NOZ_NOPERSPECTIVE_CORRECT,

		ETR_NORMAL_MAP_SOLID,
		ETR_STENCIL_SHADOW,

		ETR_TEXTURE_BLEND,
		ETR_TRANSPARENT_REFLECTION_2_LAYER,
		//ETR_REFERENCE,
		ETR_INVALID,

		ETR2_COUNT
	};


	class CBurningVideoDriver;
	class IBurningShader : public virtual IReferenceCounted
	{
	public:
		IBurningShader(CBurningVideoDriver* driver);

		//! destructor
		virtual ~IBurningShader();

		//! sets a render target
		virtual void setRenderTarget(video::IImage* surface, const core::rect<s32>& viewPort);

		//! sets the Texture
		virtual void setTextureParam( u32 stage, video::CSoftwareTexture2* texture, s32 lodLevel);
		virtual void drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c ) = 0;
		virtual void drawLine ( const s4DVertex *a,const s4DVertex *b);

		void drawWireFrameTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c );

		virtual void setParam ( u32 index, f32 value) {};
		virtual void setZCompareFunc ( u32 func) {};

		virtual void setMaterial ( const SBurningShaderMaterial &material ) {};

		void pushEdgeTest(int wireFrame,int save)
		{
			if ( save ) EdgeTestPass_stack = EdgeTestPass;
			EdgeTestPass = wireFrame ? edge_test_left : edge_test_pass;
		}
		void popEdgeTest() { EdgeTestPass = EdgeTestPass_stack; }
		virtual bool canWireFrame () { return false; }

	protected:

		CBurningVideoDriver *Driver;

		video::CImage* RenderTarget;
		CDepthBuffer* DepthBuffer;
		CStencilBuffer * Stencil;
		tVideoSample ColorMask;

		sInternalTexture IT[ BURNING_MATERIAL_MAX_TEXTURES ];

		static const tFixPointu dithermask[ 4 * 4];

		//draw degenerate triangle as line (left edge) drawTriangle -> holes,drawLine dda/bresenham
		int EdgeTestPass; //edge_test_flag
		int EdgeTestPass_stack;

	};


	IBurningShader* createTriangleRendererTextureGouraud2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M1(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M4(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGTextureLightMap2_M4(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_Add(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureDetailMap2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureVertexAlpha2(CBurningVideoDriver* driver);


	IBurningShader* createTriangleRendererTextureGouraudWire2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraud2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraudNoZ2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraudAlpha2(CBurningVideoDriver* driver);
	IBurningShader* createTRGouraudAlphaNoZ2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraudWire2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureFlat2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureFlatWire2(CBurningVideoDriver* driver);
	IBurningShader* createTRFlat2(CBurningVideoDriver* driver);
	IBurningShader* createTRFlatWire2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudNoZ2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAdd2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAddNoZ2(CBurningVideoDriver* driver);

	IBurningShader* createTRTextureGouraudAlpha(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAlphaNoZ(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureBlend(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureInverseAlphaBlend(CBurningVideoDriver* driver);

	IBurningShader* createTRNormalMap(CBurningVideoDriver* driver);
	IBurningShader* createTRStencilShadow(CBurningVideoDriver* driver);

	IBurningShader* createTriangleRendererReference(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTexture_transparent_reflection_2_layer(CBurningVideoDriver* driver);


} // end namespace video
} // end namespace irr

#endif


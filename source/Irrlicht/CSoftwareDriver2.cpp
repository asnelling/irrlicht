// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "CSoftwareDriver2.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_helper.h"
#include "CSoftwareTexture2.h"
#include "CSoftware2MaterialRenderer.h"
#include "S3DVertex.h"
#include "S4DVertex.h"
#include "CBlit.h"


#define MAT_TEXTURE(tex) ( (video::CSoftwareTexture2*) Material.org.getTexture ( tex ) )


namespace irr
{
namespace video
{

//! constructor
CBurningVideoDriver::CBurningVideoDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, video::IImagePresenter* presenter)
: CNullDriver(io, params.WindowSize), BackBuffer(0), Presenter(presenter),
	WindowId(0), SceneSourceRect(0),
	RenderTargetTexture(0), RenderTargetSurface(0), CurrentShader(0),
	DepthBuffer(0), StencilBuffer ( 0 ),
	CurrentOut(VERTEXCACHE_ELEMENT * 2),
	Temp(VERTEXCACHE_ELEMENT * 2)
{
	#ifdef _DEBUG
	setDebugName("CBurningVideoDriver");
	#endif

	AreaMinDrawSize = 0.0001f;
#ifdef BURNINGVIDEO_RENDERER_FAST
	AreaMinDrawSize = 0.5f;
#endif

	VertexCache_map_source_format();
	memset ( TransformationFlag, 0, sizeof ( TransformationFlag ) );

	// create backbuffer
	BackBuffer = new CImage(BURNINGSHADER_COLOR_FORMAT, params.WindowSize);
	if (BackBuffer)
	{
		BackBuffer->fill(SColor(0));

		// create z buffer
		if ( params.ZBufferBits )
			DepthBuffer = video::createDepthBuffer(BackBuffer->getDimension());

		// create stencil buffer
		if ( params.Stencilbuffer )
			StencilBuffer = video::createStencilBuffer(BackBuffer->getDimension());
	}

	DriverAttributes->setAttribute("MaxIndices", 1<<16);
	DriverAttributes->setAttribute("MaxTextures", BURNING_MATERIAL_MAX_TEXTURES);
	DriverAttributes->setAttribute("MaxTextureSize", SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE ? SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE : 1<<20);
	DriverAttributes->setAttribute("MaxLights", 1024 ); //glsl::gl_MaxLights);
	DriverAttributes->setAttribute("MaxTextureLODBias", 16.f);
	DriverAttributes->setAttribute("Version", 50);

	// create triangle renderers

	irr::memset32 ( BurningShader, 0, sizeof ( BurningShader ) );
	//BurningShader[ETR_FLAT] = createTRFlat2(DepthBuffer);
	//BurningShader[ETR_FLAT_WIRE] = createTRFlatWire2(DepthBuffer);
	BurningShader[ETR_GOURAUD] = createTriangleRendererGouraud2(this);
	BurningShader[ETR_GOURAUD_NOZ] = createTriangleRendererGouraudNoZ2(this);
	BurningShader[ETR_GOURAUD_ALPHA] = createTriangleRendererGouraudAlpha2(this );
	BurningShader[ETR_GOURAUD_ALPHA_NOZ_NOPERSPECTIVE_CORRECT] = createTRGouraudAlphaNoZ2(this ); // 2D
	//BurningShader[ETR_GOURAUD_WIRE] = createTriangleRendererGouraudWire2(DepthBuffer);
	//BurningShader[ETR_TEXTURE_FLAT] = createTriangleRendererTextureFlat2(DepthBuffer);
	//BurningShader[ETR_TEXTURE_FLAT_WIRE] = createTriangleRendererTextureFlatWire2(DepthBuffer);
	BurningShader[ETR_TEXTURE_GOURAUD] = createTriangleRendererTextureGouraud2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M1] = createTriangleRendererTextureLightMap2_M1(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M2] = createTriangleRendererTextureLightMap2_M2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_M4] = createTriangleRendererGTextureLightMap2_M4(this);
	BurningShader[ETR_TEXTURE_LIGHTMAP_M4] = createTriangleRendererTextureLightMap2_M4(this);
	BurningShader[ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD] = createTriangleRendererTextureLightMap2_Add(this);
	BurningShader[ETR_TEXTURE_GOURAUD_DETAIL_MAP] = createTriangleRendererTextureDetailMap2(this);

	BurningShader[ETR_TEXTURE_GOURAUD_WIRE] = createTriangleRendererTextureGouraudWire2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_NOZ] = createTRTextureGouraudNoZ2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_ADD] = createTRTextureGouraudAdd2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_ADD_NO_Z] = createTRTextureGouraudAddNoZ2(this);
	BurningShader[ETR_TEXTURE_GOURAUD_VERTEX_ALPHA] = createTriangleRendererTextureVertexAlpha2 ( this );

	BurningShader[ETR_TEXTURE_GOURAUD_ALPHA] = createTRTextureGouraudAlpha(this );
	BurningShader[ETR_TEXTURE_GOURAUD_ALPHA_NOZ] = createTRTextureGouraudAlphaNoZ( this );

	BurningShader[ETR_NORMAL_MAP_SOLID] = createTRNormalMap ( this );
	BurningShader[ETR_STENCIL_SHADOW] = createTRStencilShadow ( this );
	BurningShader[ETR_TEXTURE_BLEND] = createTRTextureBlend( this );

	BurningShader[ETR_TRANSPARENT_REFLECTION_2_LAYER] = createTriangleRendererTexture_transparent_reflection_2_layer(this);
	BurningShader[ETR_REFERENCE] = createTriangleRendererReference ( this );


	// add the same renderer for all solid types
	CSoftware2MaterialRenderer_SOLID* smr = new CSoftware2MaterialRenderer_SOLID( this);
	CSoftware2MaterialRenderer_TRANSPARENT_ADD_COLOR* tmr = new CSoftware2MaterialRenderer_TRANSPARENT_ADD_COLOR( this);
	CSoftware2MaterialRenderer_UNSUPPORTED * umr = new CSoftware2MaterialRenderer_UNSUPPORTED ( this );

	//!TODO: addMaterialRenderer depends on pushing order....
	addMaterialRenderer ( smr ); // EMT_SOLID
	addMaterialRenderer ( smr ); // EMT_SOLID_2_LAYER,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP,
	addMaterialRenderer ( tmr ); // EMT_LIGHTMAP_ADD,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_M2,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_M4,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING_M2,
	addMaterialRenderer ( smr ); // EMT_LIGHTMAP_LIGHTING_M4,
	addMaterialRenderer ( smr ); // EMT_DETAIL_MAP,
	addMaterialRenderer ( smr ); // EMT_SPHERE_MAP,
	addMaterialRenderer ( smr ); // EMT_REFLECTION_2_LAYER,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ALPHA_CHANNEL,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_ALPHA_CHANNEL_REF,
	addMaterialRenderer ( tmr ); // EMT_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( smr ); // EMT_TRANSPARENT_REFLECTION_2_LAYER,
	addMaterialRenderer ( smr ); // EMT_NORMAL_MAP_SOLID,
	addMaterialRenderer ( tmr ); // EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( smr ); // EMT_PARALLAX_MAP_SOLID,
	addMaterialRenderer ( tmr ); // EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR,
	addMaterialRenderer ( tmr ); // EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA,
	addMaterialRenderer ( tmr ); // EMT_ONETEXTURE_BLEND

	smr->drop ();
	tmr->drop ();
	umr->drop ();

	// select render target
	setRenderTargetImage(BackBuffer);

	//reset Lightspace
	LightSpace.reset ();
	EyeSpace.reset();

	// select the right renderer
	setCurrentShader();
}


//! destructor
CBurningVideoDriver::~CBurningVideoDriver()
{
	// delete Backbuffer
	if (BackBuffer)
		BackBuffer->drop();

	// delete triangle renderers

	for (s32 i=0; i<ETR2_COUNT; ++i)
	{
		if (BurningShader[i])
			BurningShader[i]->drop();
	}

	// delete Additional buffer
	if (StencilBuffer)
		StencilBuffer->drop();

	if (DepthBuffer)
		DepthBuffer->drop();

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	if (RenderTargetSurface)
		RenderTargetSurface->drop();
}


/*!
	selects the right triangle renderer based on the render states.
*/
void CBurningVideoDriver::setCurrentShader()
{
	ITexture *texture0 = Material.org.getTexture(0);
	ITexture *texture1 = Material.org.getTexture(1);

	if ( BURNING_MATERIAL_MAX_TEXTURES < 1 ) texture0 = 0;
	if ( BURNING_MATERIAL_MAX_TEXTURES < 2 ) texture1 = 0;

	bool zMaterialTest = Material.org.ZBuffer != ECFN_DISABLED &&
						Material.org.ZWriteEnable &&
						getWriteZBuffer(Material.org);

	EBurningFFShader shader = zMaterialTest ? ETR_TEXTURE_GOURAUD : ETR_TEXTURE_GOURAUD_NOZ;

	TransformationFlag[ ETS_TEXTURE_0] &= ~(ETF_TEXGEN_CAMERA_SPHERE|ETF_TEXGEN_CAMERA_REFLECTION);
	LightSpace.Flags &= ~TEXTURE_TRANSFORM;
	EyeSpace.Flags &= ~TEXTURE_TRANSFORM;

	switch ( Material.org.MaterialType )
	{
		case EMT_ONETEXTURE_BLEND:
			shader = ETR_TEXTURE_BLEND;
			break;

		case EMT_TRANSPARENT_ALPHA_CHANNEL_REF:
			Material.org.MaterialTypeParam = 0.5f;
			// fall through
		case EMT_TRANSPARENT_ALPHA_CHANNEL:
			if ( texture0 && texture0->hasAlpha () )
			{
				shader = zMaterialTest ? ETR_TEXTURE_GOURAUD_ALPHA : ETR_TEXTURE_GOURAUD_ALPHA_NOZ;
				break;
			}
			else
			{
				shader = ETR_TEXTURE_GOURAUD_VERTEX_ALPHA;
			}
			break;

		case EMT_TRANSPARENT_ADD_COLOR:
			shader = zMaterialTest ? ETR_TEXTURE_GOURAUD_ADD : ETR_TEXTURE_GOURAUD_ADD_NO_Z;
			break;

		case EMT_TRANSPARENT_VERTEX_ALPHA:
			shader = ETR_TEXTURE_GOURAUD_VERTEX_ALPHA;
			break;

		case EMT_LIGHTMAP:
		case EMT_LIGHTMAP_LIGHTING:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M1;
			break;

		case EMT_LIGHTMAP_M2:
		case EMT_LIGHTMAP_LIGHTING_M2:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M2;
			break;

		case EMT_LIGHTMAP_LIGHTING_M4:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M4;
			break;
		case EMT_LIGHTMAP_M4:
			if ( texture1 )
				shader = ETR_TEXTURE_LIGHTMAP_M4;
			break;

		case EMT_LIGHTMAP_ADD:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD;
			break;

		case EMT_DETAIL_MAP:
			if ( texture1 )
				shader = ETR_TEXTURE_GOURAUD_DETAIL_MAP;
			break;

		case EMT_SPHERE_MAP:
			TransformationFlag[ ETS_TEXTURE_0] |= ETF_TEXGEN_CAMERA_SPHERE;
			EyeSpace.Flags |= TEXTURE_TRANSFORM;
			LightSpace.Flags |= TEXTURE_TRANSFORM;
			break;
		case EMT_REFLECTION_2_LAYER:
			if ( texture1 )
			{
				shader = ETR_TEXTURE_GOURAUD_LIGHTMAP_M1;
				TransformationFlag[ ETS_TEXTURE_1] |= ETF_TEXGEN_CAMERA_REFLECTION;
				EyeSpace.Flags |= TEXTURE_TRANSFORM;
				LightSpace.Flags |= TEXTURE_TRANSFORM;
			}
			break;

		case EMT_TRANSPARENT_REFLECTION_2_LAYER:
			if ( texture1 )
			{
				shader = ETR_TRANSPARENT_REFLECTION_2_LAYER;
				TransformationFlag[ ETS_TEXTURE_1] |= ETF_TEXGEN_CAMERA_REFLECTION;
				EyeSpace.Flags |= TEXTURE_TRANSFORM;
				LightSpace.Flags |= TEXTURE_TRANSFORM;
			}
			break;

		case EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA:
		case EMT_NORMAL_MAP_SOLID:
		case EMT_PARALLAX_MAP_SOLID:
		case EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA:
			shader = ETR_NORMAL_MAP_SOLID;
			EyeSpace.Flags |= TEXTURE_TRANSFORM;
			LightSpace.Flags |= TEXTURE_TRANSFORM;
			break;

		default:
			break;

	}

	if ( !texture0 )
	{
		shader = zMaterialTest ? ETR_GOURAUD : ETR_GOURAUD_NOZ;
	}

	if ( Material.org.Wireframe )
	{
		IBurningShader* candidate = BurningShader[shader];
		if ( !candidate || (candidate && !candidate->canWireFrame()))
		{
			shader = ETR_TEXTURE_GOURAUD_WIRE;
		}
	}

	//shader = ETR_REFERENCE;

	// switchToTriangleRenderer
	CurrentShader = BurningShader[shader];
	if ( CurrentShader )
	{
		CurrentShader->setZCompareFunc ( Material.org.ZBuffer );
		CurrentShader->setRenderTarget(RenderTargetSurface, ViewPort);
		CurrentShader->setMaterial ( Material );
		CurrentShader->pushEdgeTest( Material.org.Wireframe,0 );

		switch ( shader )
		{
			case ETR_TEXTURE_GOURAUD_ALPHA:
			case ETR_TEXTURE_GOURAUD_ALPHA_NOZ:
			case ETR_TEXTURE_BLEND:
				CurrentShader->setParam ( 0, Material.org.MaterialTypeParam );
				break;
			default:
			break;
		}
	}

}



//! queries the features of the driver, returns true if feature is available
bool CBurningVideoDriver::queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
{
	if (!FeatureEnabled[feature])
		return false;

	switch (feature)
	{
#ifdef SOFTWARE_DRIVER_2_BILINEAR
	case EVDF_BILINEAR_FILTER:
		return true;
#endif
#ifdef SOFTWARE_DRIVER_2_MIPMAPPING
	case EVDF_MIP_MAP:
		return true;
#endif
	case EVDF_STENCIL_BUFFER:
		return StencilBuffer != 0;

	case EVDF_RENDER_TO_TARGET:
	case EVDF_MULTITEXTURE:
	case EVDF_HARDWARE_TL:
	case EVDF_TEXTURE_NSQUARE:
		return true;

	default:
		return false;
	}
}



//! Create render target.
IRenderTarget* CBurningVideoDriver::addRenderTarget()
{
	CSoftwareRenderTarget2* renderTarget = new CSoftwareRenderTarget2(this);
	RenderTargets.push_back(renderTarget);

	return renderTarget;
}



//matrix multiplication
void CBurningVideoDriver::transform_calc(E_TRANSFORMATION_STATE_BURNING_VIDEO state)
{
	if ( TransformationFlag[state] & ETF_VALID ) return;

	//check
	int ok = 0;
	switch ( state )
	{
		case ETS_PROJ_MODEL_VIEW:
			if ( 0 == (TransformationFlag[ETS_VIEW_PROJECTION] & ETF_VALID) ) transform_calc (ETS_VIEW_PROJECTION);
			ok = TransformationFlag[ETS_WORLD] & TransformationFlag[ETS_VIEW] & TransformationFlag[ETS_PROJECTION] & TransformationFlag[ETS_VIEW_PROJECTION] & ETF_VALID;
			break;
		case ETS_VIEW_PROJECTION:
			ok = TransformationFlag[ETS_VIEW] & TransformationFlag[ETS_PROJECTION] & ETF_VALID;
			break;
		case ETS_VIEW_INVERSE:
			ok = TransformationFlag[ETS_VIEW] & ETF_VALID;
			break;
		case ETS_MODEL_VIEW:
			ok = TransformationFlag[ETS_WORLD] & TransformationFlag[ETS_VIEW] & ETF_VALID;
			break;
		case ETS_NORMAL:
			ok = TransformationFlag[ETS_MODEL_VIEW] & ETF_VALID;
			break;
	}

	if ( !ok )
	{
		char buf[256];
		sprintf(buf,"transform_calc not valid for %d\n",state);
		os::Printer::log(buf, ELL_WARNING);
	}

	switch ( state )
	{
		case ETS_PROJ_MODEL_VIEW:
			if ( TransformationFlag[ETS_WORLD] & ETF_IDENTITY )
			{
				Transformation[state] = Transformation[ETS_VIEW_PROJECTION];
			}
			else
			{
				Transformation[state].setbyproduct_nocheck(Transformation[ETS_VIEW_PROJECTION],Transformation[ETS_WORLD]);
			}
			break;
		case ETS_VIEW_INVERSE:
			Transformation[ ETS_VIEW ].getInverse(Transformation[state]);
			{
				const f32* M = Transformation[state].pointer ();
				LightSpace.campos.x = M[12];
				LightSpace.campos.y = M[13];
				LightSpace.campos.z = M[14];
				LightSpace.campos.w = 1.f;
			}
			break;

		case ETS_VIEW_PROJECTION:
			Transformation[state].setbyproduct_nocheck (Transformation[ETS_PROJECTION],Transformation[ETS_VIEW]);
			break;
		case ETS_MODEL_VIEW:
			if ( TransformationFlag[ETS_WORLD] & ETF_IDENTITY )
			{
				Transformation[state] = Transformation[ETS_VIEW];
			}
			else
			{
				Transformation[state].setbyproduct_nocheck(Transformation[ETS_VIEW],Transformation[ETS_WORLD]);
			}
			break;
		case ETS_NORMAL:
			Transformation[ETS_MODEL_VIEW].transposed_inverse(Transformation[state]);
			break;
	}
	TransformationFlag[state] |= ETF_VALID;
}


//! sets transformation
void CBurningVideoDriver::setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat)
{
	Transformation[state] = mat;
	TransformationFlag[state] |= ETF_VALID;

	//maybe identity (mostly for texturematrix to avoid costly multiplication)
#if defined ( USE_MATRIX_TEST )
	core::setbit_cond ( TransformationFlag[state], mat.getDefinitelyIdentityMatrix(), ETF_IDENTITY );
#else
	const u32* l = (const u32*) mat.pointer();
	const u32* r = (const u32*) core::IdentityMatrix.pointer();
	size_t n = 16;
    for (; n && *l == *r; n--, l++, r++);
	if ( n ) TransformationFlag[state] &= ~ETF_IDENTITY;
	else TransformationFlag[state] |= ETF_IDENTITY;
#endif

	switch ( state )
	{
		case ETS_PROJECTION:
			TransformationFlag[ETS_PROJ_MODEL_VIEW] &= ~ETF_VALID;
			TransformationFlag[ETS_VIEW_PROJECTION] &= ~ETF_VALID;
			break;
		case ETS_VIEW:
			TransformationFlag[ETS_PROJ_MODEL_VIEW] &= ~ETF_VALID;
			TransformationFlag[ETS_VIEW_PROJECTION] &= ~ETF_VALID;
			TransformationFlag[ETS_VIEW_INVERSE] &= ~ETF_VALID;
			TransformationFlag[ETS_MODEL_VIEW] &= ~ETF_VALID;
			TransformationFlag[ETS_NORMAL] &= ~ETF_VALID;
			break;
		case ETS_WORLD:
			TransformationFlag[ETS_PROJ_MODEL_VIEW] &= ~ETF_VALID;
			TransformationFlag[ETS_MODEL_VIEW] &= ~ETF_VALID;
			TransformationFlag[ETS_NORMAL] &= ~ETF_VALID;
			break;
		case ETS_TEXTURE_0:
		case ETS_TEXTURE_1:
		case ETS_TEXTURE_2:
		case ETS_TEXTURE_3:
			if ( 0 == (TransformationFlag[state] & ETF_IDENTITY ) )
			{
				EyeSpace.Flags |= TEXTURE_TRANSFORM;
				LightSpace.Flags |= TEXTURE_TRANSFORM;
			}
			break;
		default:
			break;
	}

}

bool CBurningVideoDriver::beginScene(u32 clearFlag, SColor clearColor, f32 clearDepth, u32 clearStencil, const SExposedVideoData& videoData, core::rect<s32>* sourceRect)
{
	CNullDriver::beginScene(clearFlag, clearColor, clearDepth, clearStencil, videoData, sourceRect);
	WindowId = videoData.D3D9.HWnd;
	SceneSourceRect = sourceRect;

	clearBuffers(clearFlag, clearColor, clearDepth, clearStencil);

	memset ( TransformationFlag, 0, sizeof ( TransformationFlag ) );
	return true;
}

bool CBurningVideoDriver::endScene()
{
	CNullDriver::endScene();

	return Presenter->present(BackBuffer, WindowId, SceneSourceRect);
}

bool CBurningVideoDriver::setRenderTargetEx(IRenderTarget* target, u32 clearFlag, SColor clearColor, f32 clearDepth, u32 clearStencil)
{
	if (target && target->getDriverType() != EDT_BURNINGSVIDEO)
	{
		os::Printer::log("Fatal Error: Tried to set a render target not owned by this driver.", ELL_ERROR);
		return false;
	}

	if (RenderTargetTexture)
		RenderTargetTexture->drop();

	CSoftwareRenderTarget2* renderTarget = static_cast<CSoftwareRenderTarget2*>(target);
	RenderTargetTexture = (renderTarget) ? renderTarget->getTexture() : 0;

	if (RenderTargetTexture)
	{
		RenderTargetTexture->grab();
		setRenderTargetImage(((CSoftwareTexture2*)RenderTargetTexture)->getTexture());
	}
	else
	{
		setRenderTargetImage(BackBuffer);
	}

	clearBuffers(clearFlag, clearColor, clearDepth, clearStencil);

	return true;
}


//! sets a render target
void CBurningVideoDriver::setRenderTargetImage(video::CImage* image)
{
	if (RenderTargetSurface)
		RenderTargetSurface->drop();

	RenderTargetSurface = image;
	RenderTargetSize.Width = 0;
	RenderTargetSize.Height = 0;

	if (RenderTargetSurface)
	{
		RenderTargetSurface->grab();
		RenderTargetSize = RenderTargetSurface->getDimension();
	}

	setViewPort(core::rect<s32>(0,0,RenderTargetSize.Width,RenderTargetSize.Height));

	if (DepthBuffer)
		DepthBuffer->setSize(RenderTargetSize);

	if (StencilBuffer)
		StencilBuffer->setSize(RenderTargetSize);
}


//--------- Transform from NDC to DC, transform TexCoo ----------------------------------------------


struct tweakBurning
{
	tweakBurning()
	{
		current = 1;
		step = 0.0001f;

		ndc_shrink_x = -0.75f;
		ndc_scale_x = 0.5f;
		ndc_trans_x = -0.5f;

		ndc_shrink_y = -0.75f;
		ndc_scale_y = -0.5f;
		ndc_trans_y = -0.5f;

		tex_w_add = 0.f;
		tex_h_add = 0.f;
		tex_cx_add = 0.f;
		tex_cy_add = 0.f;

	}
	int current;
	f32 step;
	union
	{
		struct{
		f32 step;

		f32 ndc_shrink_x;
		f32 ndc_scale_x;
		f32 ndc_trans_x;

		f32 ndc_shrink_y;
		f32 ndc_scale_y;
		f32 ndc_trans_y;

		f32 tex_w_add;
		f32 tex_cx_add;
		f32 tex_h_add;
		f32 tex_cy_add;
		};
		f32 val[16];
	};
	static const char* const name[16];
	void postEventFromUser(const SEvent& e);
};

const char* const tweakBurning::name[]={"step",
	"ndc_shrink_x","ndc_scale_x","ndc_trans_x",
	"ndc_shrink_y","ndc_scale_y","ndc_trans_y",
	"tex_w_add","tex_cx_add","tex_h_add","tex_cy_add",0};

void tweakBurning::postEventFromUser(const SEvent& e)
{
	int show = 0;
	if ( e.EventType == EET_KEY_INPUT_EVENT )
	{
		switch ( e.KeyInput.Key)
		{
			case KEY_KEY_1: step *= 0.9f; if ( step < 0.00001f ) step = 0.0001f;show = 2;break;
			case KEY_KEY_2: step *= 1.1f;show = 2; break;

			case KEY_KEY_3: if (!e.KeyInput.PressedDown) {current -= 1; if (current < 1 ) current = 10; show = 1;} break;
			case KEY_KEY_4: if (!e.KeyInput.PressedDown) {current += 1; if (current > 10 ) current = 1; show = 1;} break;

			case KEY_KEY_5: val[current] -= e.KeyInput.Shift ? step * 100.f : step; show = 1; break;
			case KEY_KEY_6: val[current] += e.KeyInput.Shift ? step * 100.f : step; show = 1; break;
		}
	}
	if ( show )
	{
		if ( step < 0.0001f ) step = 0.0001f;
		char buf[256];
		if ( show == 2 ) sprintf(buf,"%s %f\n",name[0],val[0]);
		else sprintf(buf,"%s %f\n",name[current],val[current]);
		os::Printer::print(buf);
	}
}
tweakBurning Tweak;

void CBurningVideoDriver::postEventFromUser(const void* sevent)
{
	if (sevent) Tweak.postEventFromUser(*(const SEvent*) sevent);
}

// used to scale <-1,-1><1,1> to viewport
void buildNDCToDCMatrix( core::matrix4& out, const core::rect<s32>& viewport)
{
	//guard band to stay in screen bounds.(empirical). get holes left side otherwise or out of screen
	//TODO: match openGL or D3D.
	//assumption pixel center, top-left rule and rounding error projection deny exact match without additional clipping
	//or triangle render scanline doesn't step on correct texel center
	//or sampler is not on texel center

	f32* m = out.pointer();

	m[0]  = (viewport.getWidth() + Tweak.ndc_shrink_x ) * Tweak.ndc_scale_x;
	m[5]  = (viewport.getHeight() + Tweak.ndc_shrink_y ) * Tweak.ndc_scale_y;
	m[12] = Tweak.ndc_trans_x + ( (viewport.UpperLeftCorner.X + viewport.LowerRightCorner.X ) * 0.5f );
	m[13] = Tweak.ndc_trans_y + ( (viewport.UpperLeftCorner.Y + viewport.LowerRightCorner.Y ) * 0.5f );

	m[0]  = (viewport.getWidth() - 0.75f ) * 0.5f;
	m[1]  = 0.f;
	m[2]  = 0.f;
	m[3]  = 0.f;
	m[4]  = 0.f;
	m[5]  = (viewport.getHeight() - 0.75f ) * -0.5f;
	m[6]  = 0.f;
	m[7]  = 0.f;
	m[8]  = 0.f;
	m[9]  = 0.f;
	m[10] = 1.f;
	m[11] = 0.f;
	m[12] = -0.5f + ( (viewport.UpperLeftCorner.X + viewport.LowerRightCorner.X ) * 0.5f );
	m[13] = -0.5f + ( (viewport.UpperLeftCorner.Y + viewport.LowerRightCorner.Y ) * 0.5f );
	m[14] = 0.f;
	m[15] = 1.f;

}

/*!
	texcoo in current mipmap dimension (CurrentOut.data)
*/
inline void CBurningVideoDriver::select_polygon_mipmap ( s4DVertex *v, u32 vIn, u32 tex, const CSoftwareTexture2_Bound& b ) const
{
#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
	for ( u32 g = 0; g != vIn; g += 2 )
	{
		(v + g + 1 )->Tex[tex].x	= (v + g + 0)->Tex[tex].x * ( v + g + 1 )->Pos.w * (b.w+Tweak.tex_w_add) + (b.cx+Tweak.tex_cx_add);
		(v + g + 1 )->Tex[tex].y	= (v + g + 0)->Tex[tex].y * ( v + g + 1 )->Pos.w * (b.h+Tweak.tex_h_add) + (b.cy+Tweak.tex_cy_add);
	}
#else
	for ( u32 g = 0; g != vIn; g += 2 )
	{
		(v + g + 1 )->Tex[tex].x	= (v + g + 0)->Tex[tex].x * b.w;
		(v + g + 1 )->Tex[tex].y	= (v + g + 0)->Tex[tex].y * b.h;
	}
#endif

}

/*!
	texcoo in current mipmap dimension (face, already clipped)
*/
inline void CBurningVideoDriver::select_polygon_mipmap2 ( s4DVertex* v[], u32 tex, const CSoftwareTexture2_Bound& b ) const
{
#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
	(v[0] + 1 )->Tex[tex].x	= v[0]->Tex[tex].x * ( v[0] + 1 )->Pos.w * (b.w+Tweak.tex_w_add) + (b.cx+Tweak.tex_cx_add);
	(v[0] + 1 )->Tex[tex].y	= v[0]->Tex[tex].y * ( v[0] + 1 )->Pos.w * (b.h+Tweak.tex_h_add) + (b.cy+Tweak.tex_cy_add);

	(v[1] + 1 )->Tex[tex].x	= v[1]->Tex[tex].x * ( v[1] + 1 )->Pos.w * (b.w+Tweak.tex_w_add) + (b.cx+Tweak.tex_cx_add);
	(v[1] + 1 )->Tex[tex].y	= v[1]->Tex[tex].y * ( v[1] + 1 )->Pos.w * (b.h+Tweak.tex_h_add) + (b.cy+Tweak.tex_cy_add);

	(v[2] + 1 )->Tex[tex].x	= v[2]->Tex[tex].x * ( v[2] + 1 )->Pos.w * (b.w+Tweak.tex_w_add) + (b.cx+Tweak.tex_cx_add);
	(v[2] + 1 )->Tex[tex].y	= v[2]->Tex[tex].y * ( v[2] + 1 )->Pos.w * (b.h+Tweak.tex_h_add) + (b.cy+Tweak.tex_cy_add);

#else
	(v[0] + 1 )->Tex[tex].x	= v[0]->Tex[tex].x * b.w;
	(v[0] + 1 )->Tex[tex].y	= v[0]->Tex[tex].y * b.h;

	(v[1] + 1 )->Tex[tex].x	= v[1]->Tex[tex].x * b.w;
	(v[1] + 1 )->Tex[tex].y	= v[1]->Tex[tex].y * b.h;

	(v[2] + 1 )->Tex[tex].x	= v[2]->Tex[tex].x * b.w;
	(v[2] + 1 )->Tex[tex].y	= v[2]->Tex[tex].y * b.h;
#endif

}

//--------- Transform from NCD to DC ----------------------------------------------

//! sets a viewport
void CBurningVideoDriver::setViewPort(const core::rect<s32>& area)
{
	ViewPort = area;

	core::rect<s32> rendert(0,0,RenderTargetSize.Width,RenderTargetSize.Height);
	ViewPort.clipAgainst(rendert);

	//Transformation [ ETS_CLIPSCALE ].buildNDCToDCMatrix ( ViewPort, 1.f );
	buildNDCToDCMatrix( Transformation[ETS_CLIPSCALE],ViewPort);
	TransformationFlag[ETS_CLIPSCALE]  |= ETF_VALID;

	if (CurrentShader)
		CurrentShader->setRenderTarget(RenderTargetSurface, ViewPort);
}

/*
	generic plane clipping in homogenous coordinates
	special case ndc frustum <-w,w>,<-w,w>,<-w,w>
	can be rewritten with compares e.q near plane, a.z < -a.w and b.z < -b.w
*/

const sVec4 CBurningVideoDriver::NDCPlane[6] =
{
	sVec4(  0.f,  0.f, -1.f, -1.f ),	// near
	sVec4(  0.f,  0.f,  1.f, -1.f ),	// far
	sVec4(  1.f,  0.f,  0.f, -1.f ),	// left
	sVec4( -1.f,  0.f,  0.f, -1.f ),	// right
	sVec4(  0.f,  1.f,  0.f, -1.f ),	// bottom
	sVec4(  0.f, -1.f,  0.f, -1.f )		// top
};



/*
	test a vertex if it's inside the standard frustum

	this is the generic one..

	f32 dotPlane;
	for ( u32 i = 0; i!= 6; ++i )
	{
		dotPlane = v->Pos.dotProduct ( NDCPlane[i] );
		core::setbit_cond( flag, dotPlane <= 0.f, 1 << i );
	}

	// this is the base for ndc frustum <-w,w>,<-w,w>,<-w,w>
	core::setbit_cond( flag, ( v->Pos.z - v->Pos.w ) <= 0.f, 1 );
	core::setbit_cond( flag, (-v->Pos.z - v->Pos.w ) <= 0.f, 2 );
	core::setbit_cond( flag, ( v->Pos.x - v->Pos.w ) <= 0.f, 4 );
	core::setbit_cond( flag, (-v->Pos.x - v->Pos.w ) <= 0.f, 8 );
	core::setbit_cond( flag, ( v->Pos.y - v->Pos.w ) <= 0.f, 16 );
	core::setbit_cond( flag, (-v->Pos.y - v->Pos.w ) <= 0.f, 32 );

*/
#ifdef IRRLICHT_FAST_MATH

REALINLINE u32 CBurningVideoDriver::clipToFrustumTest ( const s4DVertex * v  ) const
{
	f32 test[6];
	u32 flag;
	const f32 w = - v->Pos.w;

	// a conditional move is needed....FCOMI ( but we don't have it )
	// so let the fpu calculate and write it back.
	// cpu makes the compare, interleaving

	test[0] =  v->Pos.z + w;
	test[1] = -v->Pos.z + w;
	test[2] =  v->Pos.x + w;
	test[3] = -v->Pos.x + w;
	test[4] =  v->Pos.y + w;
	test[5] = -v->Pos.y + w;

	flag  = (IR ( test[0] )              ) >> 31;
	flag |= (IR ( test[1] ) & 0x80000000 ) >> 30;
	flag |= (IR ( test[2] ) & 0x80000000 ) >> 29;
	flag |= (IR ( test[3] ) & 0x80000000 ) >> 28;
	flag |= (IR ( test[4] ) & 0x80000000 ) >> 27;
	flag |= (IR ( test[5] ) & 0x80000000 ) >> 26;

/*
	flag  = F32_LOWER_EQUAL_0 ( test[0] );
	flag |= F32_LOWER_EQUAL_0 ( test[1] ) << 1;
	flag |= F32_LOWER_EQUAL_0 ( test[2] ) << 2;
	flag |= F32_LOWER_EQUAL_0 ( test[3] ) << 3;
	flag |= F32_LOWER_EQUAL_0 ( test[4] ) << 4;
	flag |= F32_LOWER_EQUAL_0 ( test[5] ) << 5;
*/
	return flag;
}

#else


REALINLINE u32 CBurningVideoDriver::clipToFrustumTest ( const s4DVertex * v  ) const
{
	u32 flag = 0;

	flag |= v->Pos.z <= v->Pos.w ? 1 : 0;
	flag |= -v->Pos.z <= v->Pos.w ? 2 : 0;

	flag |= v->Pos.x <= v->Pos.w ? 4 : 0;
	flag |= -v->Pos.x <= v->Pos.w ? 8 : 0;

	flag |= v->Pos.y <= v->Pos.w ? 16 : 0;
	flag |= -v->Pos.y <= v->Pos.w ? 32 : 0;

/*
	if ( v->Pos.z <= v->Pos.w ) flag |= 1;
	if (-v->Pos.z <= v->Pos.w ) flag |= 2;

	if ( v->Pos.x <= v->Pos.w ) flag |= 4;
	if (-v->Pos.x <= v->Pos.w ) flag |= 8;

	if ( v->Pos.y <= v->Pos.w ) flag |= 16;
	if (-v->Pos.y <= v->Pos.w ) flag |= 32;
*/
/*
	for ( u32 i = 0; i!= 6; ++i )
	{
		core::setbit_cond( flag, v->Pos.dotProduct ( NDCPlane[i] ) <= 0.f, 1 << i );
	}
*/
	return flag;
}

#endif // _MSC_VER

u32 CBurningVideoDriver::clipToHyperPlane ( s4DVertex * dest, const s4DVertex * source, u32 inCount, const sVec4 &plane )
{
	u32 outCount = 0;
	s4DVertex * out = dest;

	const s4DVertex * a;
	const s4DVertex * b = source;

	f32 bDotPlane;

	bDotPlane = b->Pos.dotProduct ( plane );

/*
	for( u32 i = 1; i < inCount + 1; ++i)
	{
#if 0
		a = source + (i%inCount)*2;
#else
		const s32 condition = i - inCount;
		const s32 index = (( ( condition >> 31 ) & ( i ^ condition ) ) ^ condition ) << 1;
		a = source + index;
#endif
*/
	for( u32 i = 0; i < inCount; ++i)
	{
		a = source + (i == inCount-1 ? 0 : (i+1)*2);

		// current point inside
		if ( a->Pos.dotProduct ( plane ) <= 0.f )
		{
			// last point outside
			if ( F32_GREATER_0 ( bDotPlane ) )
			{
				// intersect line segment with plane
				out->interpolate ( *b, *a, bDotPlane / (b->Pos - a->Pos).dotProduct ( plane ) );
				out += 2;
				outCount += 1;
			}

			// copy current to out
			//*out = *a;
			memcpy32_small ( out, a, SIZEOF_SVERTEX * 2 );
			b = out;

			out += 2;
			outCount += 1;
		}
		else
		{
			// current point outside

			if ( F32_LOWER_EQUAL_0 (  bDotPlane ) )
			{
				// previous was inside
				// intersect line segment with plane
				out->interpolate ( *b, *a, bDotPlane / (b->Pos - a->Pos).dotProduct ( plane ) );
				out += 2;
				outCount += 1;
			}
			// pointer
			b = a;
		}

		bDotPlane = b->Pos.dotProduct ( plane );

	}

	return outCount;
}


u32 CBurningVideoDriver::clipToFrustum ( s4DVertex *v0, s4DVertex * v1, const u32 vIn )
{
	u32 vOut = vIn;

	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[0] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[1] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[2] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[3] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v1, v0, vOut, NDCPlane[4] ); if ( vOut < vIn ) return vOut;
	vOut = clipToHyperPlane ( v0, v1, vOut, NDCPlane[5] );
	return vOut;
}

/*!
 Part I:
	apply Clip Scale matrix
	From Normalized Device Coordiante ( NDC ) Space to Device Coordinate Space ( DC )

 Part II:
	Project homogeneous vector
	homogeneous to non-homogenous coordinates ( dividebyW )

	Incoming: ( xw, yw, zw, w, u, v, 1, R, G, B, A )
	Outgoing: ( xw/w, yw/w, zw/w, w/w, u/w, v/w, 1/w, R/w, G/w, B/w, A/w )


	replace w/w by 1/w
*/
inline void CBurningVideoDriver::ndc_2_dc_and_project ( s4DVertex *dest,s4DVertex *source, u32 vIn ) const
{
	u32 g;

	const f32* dc = Transformation [ ETS_CLIPSCALE ].pointer();

	for ( g = 0; g != vIn; g += 2 )
	{
		if ( (dest[g].flag & VERTEX4D_PROJECTED ) == VERTEX4D_PROJECTED )
			continue;

		dest[g].flag = source[g].flag | VERTEX4D_PROJECTED;

		const f32 w = source[g].Pos.w;
		const f32 iw = reciprocal_zero ( w );

		// to device coordinates
		dest[g].Pos.x = iw * ( source[g].Pos.x * dc[ 0] + w * dc[12] );
		dest[g].Pos.y = iw * ( source[g].Pos.y * dc[ 5] + w * dc[13] );

#ifndef SOFTWARE_DRIVER_2_USE_WBUFFER
		dest[g].Pos.z = iw * source[g].Pos.z;
#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
		#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
			dest[g].Color[0] = source[g].Color[0] * iw;
		#else
			dest[g].Color[0] = source[g].Color[0];
		#endif
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
		dest[g].LightTangent[0] = source[g].LightTangent[0] * iw;
#endif
		dest[g].Pos.w = iw;
	}
}


inline void CBurningVideoDriver::ndc_2_dc_and_project2 ( s4DVertex **v, const u32 size ) const
{
	u32 g;

	const f32* dc = Transformation [ ETS_CLIPSCALE ].pointer();

	for ( g = 0; g != size; g += 1 )
	{
		s4DVertex * a = (s4DVertex*) v[g];

		if ( (a[1].flag & VERTEX4D_PROJECTED ) == VERTEX4D_PROJECTED )
			continue;

		a[1].flag = a->flag | VERTEX4D_PROJECTED;

		// project homogenous vertex, store 1/w
		const f32 w = a->Pos.w;
		const f32 iw = reciprocal_zero ( w );

		// to device coordinates
		a[1].Pos.x = iw * ( a->Pos.x * dc[ 0] + w * dc[12] );
		a[1].Pos.y = iw * ( a->Pos.y * dc[ 5] + w * dc[13] );

#ifndef SOFTWARE_DRIVER_2_USE_WBUFFER
		a[1].Pos.z = a->Pos.z * iw;
#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
		#ifdef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
			a[1].Color[0] = a->Color[0] * iw;
		#else
			a[1].Color[0] = a->Color[0];
		#endif
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
		a[1].LightTangent[0] = a[0].LightTangent[0] * iw;
#endif
		a[1].Pos.w = iw;

	}

}


/*!
	crossproduct in projected 2D -> screen area triangle
*/
inline f32 CBurningVideoDriver::screenarea ( const s4DVertex *v ) const
{
	return	( ( v[3].Pos.x - v[1].Pos.x ) * ( v[5].Pos.y - v[1].Pos.y ) ) -
			( ( v[3].Pos.y - v[1].Pos.y ) * ( v[5].Pos.x - v[1].Pos.x ) );
}


/*!
*/
inline f32 CBurningVideoDriver::texelarea ( const s4DVertex *v, int tex ) const
{
	f32 z;

	z =		( (v[2].Tex[tex].x - v[0].Tex[tex].x ) * (v[4].Tex[tex].y - v[0].Tex[tex].y ) )
		 -	( (v[4].Tex[tex].x - v[0].Tex[tex].x ) * (v[2].Tex[tex].y - v[0].Tex[tex].y ) );

	return MAT_TEXTURE ( tex )->getLODFactor ( z );
}

/*!
	crossproduct in projected 2D
*/
inline f32 CBurningVideoDriver::screenarea2 ( s4DVertex* const v[] ) const
{
	return	( (( v[1] + 1 )->Pos.x - (v[0] + 1 )->Pos.x ) * ( (v[2] + 1 )->Pos.y - (v[0] + 1 )->Pos.y ) ) -
			( (( v[1] + 1 )->Pos.y - (v[0] + 1 )->Pos.y ) * ( (v[2] + 1 )->Pos.x - (v[0] + 1 )->Pos.x ) );
}

/*!
*/
inline f32 CBurningVideoDriver::texelarea2 ( s4DVertex* const v[], s32 tex ) const
{
	sVec2 a(v[1]->Tex[tex].x - v[0]->Tex[tex].x,v[1]->Tex[tex].y - v[0]->Tex[tex].y);
	sVec2 b(v[2]->Tex[tex].x - v[0]->Tex[tex].x,v[2]->Tex[tex].y - v[0]->Tex[tex].y);
	f32 area = a.x * b.y - b.x * a.y;
/*
	f32 area;
	area =	( (v[1]->Tex[tex].x - v[0]->Tex[tex].x ) * (v[2]->Tex[tex].y - v[0]->Tex[tex].y ) )
		 -	( (v[2]->Tex[tex].x - v[0]->Tex[tex].x ) * (v[1]->Tex[tex].y - v[0]->Tex[tex].y ) );
*/
	return MAT_TEXTURE ( tex )->getLODFactor ( area );
}



// Vertex Cache
const SVSize vSize_template[] =
{
	{ VERTEX4D_FORMAT_TEXTURE_1 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex), 1 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex2TCoords),2 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1 | VERTEX4D_FORMAT_BUMP_DOT3, sizeof(S3DVertexTangents),2 },
	{ VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1, sizeof(S3DVertex), 2 },	// reflection map
	{ 0, sizeof(f32) * 3, 0 },	// core::vector3df*
};

void CBurningVideoDriver::VertexCache_map_source_format()
{
	u32 s0 = sizeof(s4DVertex);
	u32 s1 = sizeof(s4DVertex_proxy);

	if ( s1 <= SIZEOF_SVERTEX/2 )
	{
		os::Printer::log ( "BurningVideo vertex format unnecessary to large", ELL_WARNING );
	}

	if ( s0 != SIZEOF_SVERTEX )
	{
		os::Printer::log ( "BurningVideo vertex format compile problem", ELL_ERROR );
		_IRR_DEBUG_BREAK_IF(s0 != SIZEOF_SVERTEX);
	}

	vSize[0].Format = VERTEX4D_FORMAT_TEXTURE_1 | VERTEX4D_FORMAT_COLOR_1;
	vSize[0].Pitch = sizeof(S3DVertex);
	vSize[0].TexSize = 1;

	vSize[1].Format = VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1;
	vSize[1].Pitch = sizeof(S3DVertex2TCoords);
	vSize[1].TexSize = 2;

	vSize[2].Format = VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1 | VERTEX4D_FORMAT_BUMP_DOT3;
	vSize[2].Pitch = sizeof(S3DVertexTangents);
	vSize[2].TexSize = 2;

	// reflection map
	vSize[3].Format = VERTEX4D_FORMAT_TEXTURE_2 | VERTEX4D_FORMAT_COLOR_1;
	vSize[3].Pitch = sizeof(S3DVertex);
	vSize[3].TexSize = 2;

	// shadow
	vSize[4].Format = 0;
	vSize[4].Pitch = sizeof(f32) * 3; // core::vector3df*
	vSize[4].TexSize = 0;

	u32 size;

	for ( int i = 0; i < 5; ++i )
	{
		u32& flag = vSize[i].Format;

		if ( vSize[i].TexSize > BURNING_MATERIAL_MAX_TEXTURES )
			vSize[i].TexSize = BURNING_MATERIAL_MAX_TEXTURES;

		size = (flag & VERTEX4D_FORMAT_MASK_TEXTURE) >> 16;
		if ( size > BURNING_MATERIAL_MAX_TEXTURES )
		{
			flag = (flag & ~VERTEX4D_FORMAT_MASK_TEXTURE) | (BURNING_MATERIAL_MAX_TEXTURES << 16);
		}

		size = (flag & VERTEX4D_FORMAT_MASK_COLOR) >> 20;
		if ( size > BURNING_MATERIAL_MAX_COLORS )
		{
			flag = (flag & ~VERTEX4D_FORMAT_MASK_COLOR) | (BURNING_MATERIAL_MAX_COLORS << 20);
		}

		size = (flag & VERTEX4D_FORMAT_MASK_TANGENT) >> 24;
		if ( size > BURNING_MATERIAL_MAX_TANGENT )
		{
			flag = (flag & ~VERTEX4D_FORMAT_MASK_TANGENT) | (BURNING_MATERIAL_MAX_TANGENT << 24);
		}

	}
}



/*!
	fill a cache line with transformed, light and clipp test triangles
*/
void CBurningVideoDriver::VertexCache_fill(const u32 sourceIndex, const u32 destIndex)
{
	u8 * source;
	s4DVertex *dest;

	source = (u8*) VertexCache.vertices + ( sourceIndex * vSize[VertexCache.vType].Pitch );

	// it's a look ahead so we never hit it..
	// but give priority...
	//VertexCache.info[ destIndex ].hit = hitCount;

	// store info
	VertexCache.info[ destIndex ].index = sourceIndex;
	VertexCache.info[ destIndex ].hit = 0;

	// destination Vertex
	dest = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( destIndex << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );

	// transform Model * World * Camera * Projection * NDCSpace matrix
	const S3DVertex *base = ((S3DVertex*) source );
	Transformation [ ETS_PROJ_MODEL_VIEW].transformVect ( &dest->Pos.x, base->Pos );

	//mhm ;-) maybe no goto
	if ( VertexCache.vType == 4 ) goto clipandproject;


#if defined (SOFTWARE_DRIVER_2_LIGHTING) || defined ( SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM )

	// vertex normal in light(world) space
/*
	if ( LightSpace.enabled && (Material.org.Lighting || (LightSpace.Flags & TEXTURE_TRANSFORM)) )
	{
		if ( TransformationFlag[ETS_WORLD] & ETF_IDENTITY )
		{
			LightSpace.normal.set ( base->Normal.X, base->Normal.Y, base->Normal.Z, 1.f );
			LightSpace.vertex.set ( base->Pos.X, base->Pos.Y, base->Pos.Z, 1.f );
		}
		else
		{
			Transformation[ETS_WORLD].rotateVect ( &LightSpace.normal.x, base->Normal );

			// vertex in light space
			//if ( LightSpace.Flags & ( POINTLIGHT | FOG | SPECULAR | VERTEXTRANSFORM) )
				Transformation[ETS_WORLD].transformVect ( &LightSpace.vertex.x, base->Pos );
		}

		if ( LightSpace.Flags & NORMALIZE_NORMALS )
			LightSpace.normal.normalize_xyz();
	}
*/
	// vertex normal in light(eye) space
	if ( Material.org.Lighting || (EyeSpace.Flags & TEXTURE_TRANSFORM) )
	{
		Transformation[ETS_MODEL_VIEW].transformVect ( &EyeSpace.vertex4.x, base->Pos );
		Transformation[ETS_NORMAL].rotateVect(&EyeSpace.normal3.x,base->Normal);
		if ( EyeSpace.Flags & NORMALIZE_NORMALS )
			EyeSpace.normal3.normalize_xyz();

		f32 iw = reciprocal_zero(EyeSpace.vertex4.w);
		EyeSpace.vertex3.x = EyeSpace.vertex4.x * iw;
		EyeSpace.vertex3.y = EyeSpace.vertex4.y * iw;
		EyeSpace.vertex3.z = EyeSpace.vertex4.z * iw;

	}

#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
	// apply lighting model
	#if defined (SOFTWARE_DRIVER_2_LIGHTING)
		if ( Material.org.Lighting )
		{
			lightVertex_eye ( dest, base->Color.color );
			//if ( LightSpace.enabled) lightVertex_world ( dest, base->Color.color );
		}
		else
		{
			dest->Color[0].setA8R8G8B8 ( base->Color.color );
		}
	#else
		dest->Color[0].setA8R8G8B8 ( base->Color.color );
	#endif
#endif

	// Texture Transform
#if !defined ( SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM )
	memcpy32_small ( &dest->Tex[0],&base->TCoords,vSize[VertexCache.vType].TexSize << 3 /* * ( sizeof ( f32 ) * 2 ) */ );
#else

	if ( 0 == (EyeSpace.Flags & TEXTURE_TRANSFORM) )
	{
		memcpy32_small ( &dest->Tex[0],&base->TCoords,vSize[VertexCache.vType].TexSize << 3 /*  * ( sizeof ( f32 ) * 2 ) */);
	}
	else
	{
	/*
			Generate texture coordinates as linear functions so that:
				u = Ux*x + Uy*y + Uz*z + Uw
				v = Vx*x + Vy*y + Vz*z + Vw
			The matrix M for this case is:
				Ux  Vx  0  0
				Uy  Vy  0  0
				Uz  Vz  0  0
				Uw  Vw  0  0
	*/

		u32 t;
		sVec4 u;
		sVec4 n;
		sVec4 r;
		sVec2 srcT;
		f32 dot;

		for ( t = 0; t != vSize[VertexCache.vType].TexSize; ++t )
		{
			const f32* M = Transformation [ ETS_TEXTURE_0 + t ].pointer();

			// texgen
			if ( TransformationFlag[ETS_TEXTURE_0+t] & ETF_TEXGEN_CAMERA_SPHERE )
			{
				u.x = EyeSpace.vertex3.x;
				u.y = EyeSpace.vertex3.y;
				u.z = EyeSpace.vertex3.z;
				u.normalize_xyz();

				//reflect(u,N) u - 2.0 * dot(N, u) * N
				dot = -2.f * EyeSpace.normal3.dot_xyz(u);
				r.x = u.x + dot * EyeSpace.normal3.x;
				r.y = u.y + dot * EyeSpace.normal3.y;
				r.z = u.z + dot * EyeSpace.normal3.z;

				//openGL
/*
				f32 m = 2.f * sqrtf(r.x*r.x+r.y*r.y+(r.z+1.f)*(r.z+1.f));
				dest[0].Tex[t].x = r.x / m + 0.5f;
				dest[0].Tex[t].y = r.y / m + 0.5f;
*/
				//~d3d with spheremap scale
				f32 m = 0.25f / (0.00001f + sqrtf(r.x*r.x+r.y*r.y+r.z*r.z));
				dest[0].Tex[t].x =  r.x * m + 0.5f;
				dest[0].Tex[t].y = -r.y * m + 0.5f;
			}
			else
			if ( TransformationFlag [ ETS_TEXTURE_0 + t ] & ETF_TEXGEN_CAMERA_REFLECTION )
			{
				u.x = EyeSpace.vertex3.x;
				u.y = EyeSpace.vertex3.y;
				u.z = EyeSpace.vertex3.z;
				u.normalize_xyz();

				//reflect(u,N) u - 2.0 * dot(N, u) * N
				dot = -2.f * EyeSpace.normal3.dot_xyz(u);
				r.x = u.x + dot * EyeSpace.normal3.x;
				r.y = u.y + dot * EyeSpace.normal3.y;
				//r.z = u.z + dot * EyeSpace.normal.z;

				//openGL
/*
				dest[0].Tex[t].x = r.x;
				dest[0].Tex[t].y = r.y;
*/
				//~d3d with spheremap scale
				dest[0].Tex[t].x =  r.x;
				dest[0].Tex[t].y =  r.y;
			}
			else
			{
				memcpy32_small ( &srcT,(&base->TCoords) + t,sizeof ( f32 ) * 2 );

				switch ( Material.org.TextureLayer[t].TextureWrapU )
				{
					case ETC_CLAMP:
					case ETC_CLAMP_TO_EDGE:
					case ETC_CLAMP_TO_BORDER:
						dest->Tex[t].x = core::clamp ( (f32) ( M[0] * srcT.x + M[4] * srcT.y + M[8] ), 0.f, 1.f );
						break;
					case ETC_MIRROR:
						dest->Tex[t].x = M[0] * srcT.x + M[4] * srcT.y + M[8];
						if (core::fract(dest->Tex[t].x)>0.5f)
							dest->Tex[t].x=1.f-dest->Tex[t].x;
					break;
					case ETC_MIRROR_CLAMP:
					case ETC_MIRROR_CLAMP_TO_EDGE:
					case ETC_MIRROR_CLAMP_TO_BORDER:
						dest->Tex[t].x = core::clamp ( (f32) ( M[0] * srcT.x + M[4] * srcT.y + M[8] ), 0.f, 1.f );
						if (core::fract(dest->Tex[t].x)>0.5f)
							dest->Tex[t].x=1.f-dest->Tex[t].x;
					break;
					case ETC_REPEAT:
					default:
						dest->Tex[t].x = M[0] * srcT.x + M[4] * srcT.y + M[8];
						break;
				}
				switch ( Material.org.TextureLayer[t].TextureWrapV )
				{
					case ETC_CLAMP:
					case ETC_CLAMP_TO_EDGE:
					case ETC_CLAMP_TO_BORDER:
						dest->Tex[t].y = core::clamp ( (f32) ( M[1] * srcT.x + M[5] * srcT.y + M[9] ), 0.f, 1.f );
						break;
					case ETC_MIRROR:
						dest->Tex[t].y = M[1] * srcT.x + M[5] * srcT.y + M[9];
						if (core::fract(dest->Tex[t].y)>0.5f)
							dest->Tex[t].y=1.f-dest->Tex[t].y;
					break;
					case ETC_MIRROR_CLAMP:
					case ETC_MIRROR_CLAMP_TO_EDGE:
					case ETC_MIRROR_CLAMP_TO_BORDER:
						dest->Tex[t].y = core::clamp ( (f32) ( M[1] * srcT.x + M[5] * srcT.y + M[9] ), 0.f, 1.f );
						if (core::fract(dest->Tex[t].y)>0.5f)
							dest->Tex[t].y=1.f-dest->Tex[t].y;
					break;
					case ETC_REPEAT:
					default:
						dest->Tex[t].y = M[1] * srcT.x + M[5] * srcT.y + M[9];
						break;
				}
			}
		}
	}

#if 0
	// tangent space light vector, emboss
	if ( Lights.size () && ( vSize[VertexCache.vType].Format & VERTEX4D_FORMAT_BUMP_DOT3 ) )
	{
		const S3DVertexTangents *tangent = ((S3DVertexTangents*) source );
		const SBurningShaderLight &light = LightSpace.Light[0];

		sVec4 vp;

		vp.x = light.pos.x - LightSpace.vertex.x;
		vp.y = light.pos.y - LightSpace.vertex.y;
		vp.z = light.pos.z - LightSpace.vertex.z;

		vp.normalize_xyz();

		LightSpace.tangent.x = vp.x * tangent->Tangent.X + vp.y * tangent->Tangent.Y + vp.z * tangent->Tangent.Z;
		LightSpace.tangent.y = vp.x * tangent->Binormal.X + vp.y * tangent->Binormal.Y + vp.z * tangent->Binormal.Z;
		//LightSpace.tangent.z = vp.x * tangent->Normal.X + vp.y * tangent->Normal.Y + vp.z * tangent->Normal.Z;
		LightSpace.tangent.z = 0.f;
		LightSpace.tangent.normalize_xyz();

		f32 scale = 1.f / 128.f;
		if ( Material.org.MaterialTypeParam > 0.f )
			scale = Material.org.MaterialTypeParam;

		// emboss, shift coordinates
		dest->Tex[1].x = dest->Tex[0].x + LightSpace.tangent.x * scale;
		dest->Tex[1].y = dest->Tex[0].y + LightSpace.tangent.y * scale;
		//dest->Tex[1].z = LightSpace.tangent.z * scale;
	}
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
	if ( EyeSpace.Light.size () && ( vSize[VertexCache.vType].Format & VERTEX4D_FORMAT_BUMP_DOT3 ) )
	{
		const S3DVertexTangents *tangent = ((S3DVertexTangents*) source );

		sVec4 vp;

		dest->LightTangent[0].x = 0.f;
		dest->LightTangent[0].y = 0.f;
		dest->LightTangent[0].z = 0.f;
		for ( u32 i = 0; i < 2 && i < EyeSpace.Light.size (); ++i )
		{
			const SBurningShaderLight &light = EyeSpace.Light[i];

			if ( !light.LightIsOn )
				continue;

			vp.x = light.pos.x - EyeSpace.vertex3.x;
			vp.y = light.pos.y - EyeSpace.vertex3.y;
			vp.z = light.pos.z - EyeSpace.vertex3.z;
	
			vp.normalize_xyz();


			// transform by tangent matrix
			sVec3 l;
	#if 1
			l.x = (vp.x * tangent->Tangent.X + vp.y * tangent->Tangent.Y + vp.z * tangent->Tangent.Z );
			l.y = (vp.x * tangent->Binormal.X + vp.y * tangent->Binormal.Y + vp.z * tangent->Binormal.Z );
			l.z = (vp.x * tangent->Normal.X + vp.y * tangent->Normal.Y + vp.z * tangent->Normal.Z );
	#else
			l.x = (vp.x * tangent->Tangent.X + vp.y * tangent->Binormal.X + vp.z * tangent->Normal.X );
			l.y = (vp.x * tangent->Tangent.Y + vp.y * tangent->Binormal.Y + vp.z * tangent->Normal.Y );
			l.z = (vp.x * tangent->Tangent.Z + vp.y * tangent->Binormal.Z + vp.z * tangent->Normal.Z );
	#endif


	/*
			f32 scale = 1.f / 128.f;
			scale /= dest->LightTangent[0].b;

			// emboss, shift coordinates
			dest->Tex[1].x = dest->Tex[0].x + l.r * scale;
			dest->Tex[1].y = dest->Tex[0].y + l.g * scale;
	*/
			dest->Tex[1].x = dest->Tex[0].x;
			dest->Tex[1].y = dest->Tex[0].y;

			// scale bias
			dest->LightTangent[0].x += l.x;
			dest->LightTangent[0].y += l.y;
			dest->LightTangent[0].z += l.z;
		}
		dest->LightTangent[0].setLength ( 0.5f );
		dest->LightTangent[0].x += 0.5f;
		dest->LightTangent[0].y += 0.5f;
		dest->LightTangent[0].z += 0.5f;
	}
#endif //if BURNING_MATERIAL_MAX_TANGENT > 0

#endif

clipandproject:
	dest[0].flag = dest[1].flag = vSize[VertexCache.vType].Format;

	// test vertex
	dest[0].flag |= clipToFrustumTest ( dest);

	// to DC Space, project homogenous vertex
	if ( (dest[0].flag & VERTEX4D_CLIPMASK ) == VERTEX4D_INSIDE )
	{
		ndc_2_dc_and_project2 ( (s4DVertex**) &dest, 1 );
	}

	//return dest;
}

//

REALINLINE s4DVertex * CBurningVideoDriver::VertexCache_getVertex ( const u32 sourceIndex )
{
	for ( s32 i = 0; i < VERTEXCACHE_ELEMENT; ++i )
	{
		if ( VertexCache.info[ i ].index == sourceIndex )
		{
			return (s4DVertex *) ( (u8*) VertexCache.mem.data + ( i << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
		}
	}
	return 0;
}


/*
	Cache based on linear walk indices
	fill blockwise on the next 16(Cache_Size) unique vertices in indexlist
	merge the next 16 vertices with the current
*/
REALINLINE void CBurningVideoDriver::VertexCache_get(s4DVertex ** face)
{
	SCacheInfo info[VERTEXCACHE_ELEMENT];

	// next primitive must be complete in cache
	if (	VertexCache.indicesIndex - VertexCache.indicesRun < 3 &&
			VertexCache.indicesIndex < VertexCache.indexCount
		)
	{
		// rewind to start of primitive
		VertexCache.indicesIndex = VertexCache.indicesRun;

		irr::memset32 ( info, VERTEXCACHE_MISS, sizeof ( info ) );

		// get the next unique vertices cache line
		u32 fillIndex = 0;
		u32 dIndex = 0;
		u32 i = 0;
		u32 sourceIndex = 0;

		while ( VertexCache.indicesIndex < VertexCache.indexCount &&
				fillIndex < VERTEXCACHE_ELEMENT
				)
		{
			switch ( VertexCache.iType )
			{
				case 1:
					sourceIndex =  ((u16*)VertexCache.indices) [ VertexCache.indicesIndex ];
					break;
				case 2:
					sourceIndex =  ((u32*)VertexCache.indices) [ VertexCache.indicesIndex ];
					break;
				case 4:
					sourceIndex = VertexCache.indicesIndex;
					break;
			}

			VertexCache.indicesIndex += 1;

			// if not exist, push back
			s32 exist = 0;
			for ( dIndex = 0;  dIndex < fillIndex; ++dIndex )
			{
				if ( info[ dIndex ].index == sourceIndex )
				{
					exist = 1;
					break;
				}
			}

			if ( 0 == exist )
			{
				info[fillIndex++].index = sourceIndex;
			}
		}

		// clear marks
		for ( i = 0; i!= VERTEXCACHE_ELEMENT; ++i )
		{
			VertexCache.info[i].hit = 0;
		}

		// mark all existing
		for ( i = 0; i!= fillIndex; ++i )
		{
			for ( dIndex = 0;  dIndex < VERTEXCACHE_ELEMENT; ++dIndex )
			{
				if ( VertexCache.info[ dIndex ].index == info[i].index )
				{
					info[i].hit = dIndex;
					VertexCache.info[ dIndex ].hit = 1;
					break;
				}
			}
		}

		// fill new
		for ( i = 0; i!= fillIndex; ++i )
		{
			if ( info[i].hit != VERTEXCACHE_MISS )
				continue;

			for ( dIndex = 0;  dIndex < VERTEXCACHE_ELEMENT; ++dIndex )
			{
				if ( 0 == VertexCache.info[dIndex].hit )
				{
					VertexCache_fill ( info[i].index, dIndex );
					VertexCache.info[dIndex].hit += 1;
					info[i].hit = dIndex;
					break;
				}
			}
		}
	}

	const u32 i0 = core::if_c_a_else_0 ( VertexCache.pType != scene::EPT_TRIANGLE_FAN, VertexCache.indicesRun );

	switch ( VertexCache.iType )
	{
		case 1:
		{
			const u16 *p = (const u16 *) VertexCache.indices;
			face[0] = VertexCache_getVertex ( p[ i0    ] );
			face[1] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 1] );
			face[2] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 2] );
		}
		break;

		case 2:
		{
			const u32 *p = (const u32 *) VertexCache.indices;
			face[0] = VertexCache_getVertex ( p[ i0    ] );
			face[1] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 1] );
			face[2] = VertexCache_getVertex ( p[ VertexCache.indicesRun + 2] );
		}
		break;

		case 4:
			face[0] = VertexCache_getVertex ( VertexCache.indicesRun + 0 );
			face[1] = VertexCache_getVertex ( VertexCache.indicesRun + 1 );
			face[2] = VertexCache_getVertex ( VertexCache.indicesRun + 2 );
		break;
		default:
			face[0] = face[1] = face[2] = VertexCache_getVertex(VertexCache.indicesRun + 0);
		break;
	}

	VertexCache.indicesRun += VertexCache.primitivePitch;
}

/*!
*/
REALINLINE void CBurningVideoDriver::VertexCache_getbypass ( s4DVertex ** face )
{
	const u32 i0 = core::if_c_a_else_0 ( VertexCache.pType != scene::EPT_TRIANGLE_FAN, VertexCache.indicesRun );

	if ( VertexCache.iType == 1 )
	{
		const u16 *p = (const u16 *) VertexCache.indices;
		VertexCache_fill ( p[ i0    ], 0 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 1], 1 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 2], 2 );
	}
	else
	{
		const u32 *p = (const u32 *) VertexCache.indices;
		VertexCache_fill ( p[ i0    ], 0 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 1], 1 );
		VertexCache_fill ( p[ VertexCache.indicesRun + 2], 2 );
	}

	VertexCache.indicesRun += VertexCache.primitivePitch;

	face[0] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 0 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
	face[1] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 1 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );
	face[2] = (s4DVertex *) ( (u8*) VertexCache.mem.data + ( 2 << ( SIZEOF_SVERTEX_LOG2 + 1  ) ) );

}

/*!
*/
void CBurningVideoDriver::VertexCache_reset ( const void* vertices, u32 vertexCount,
											const void* indices, u32 primitiveCount,
											E_VERTEX_TYPE vType,
											scene::E_PRIMITIVE_TYPE pType,
											E_INDEX_TYPE iType)
{
	VertexCache.vertices = vertices;
	VertexCache.vertexCount = vertexCount;

	VertexCache.indices = indices;
	VertexCache.indicesIndex = 0;
	VertexCache.indicesRun = 0;

	switch(Material.org.MaterialType)
	{
		case EMT_REFLECTION_2_LAYER:
		case EMT_TRANSPARENT_REFLECTION_2_LAYER:
			VertexCache.vType = 3;
			break;
		default:
			VertexCache.vType = vType;
			break;
	}
		
	VertexCache.pType = pType;

	switch ( iType )
	{
		case EIT_16BIT: VertexCache.iType = 1; break;
		case EIT_32BIT: VertexCache.iType = 2; break;
		default:
			VertexCache.iType = iType; break;
	}

	switch ( VertexCache.pType )
	{
		// most types here will not work as expected, only triangles/triangle_fan
		// is known to work.
		case scene::EPT_POINTS:
			VertexCache.indexCount = primitiveCount;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINE_STRIP:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINE_LOOP:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_LINES:
			VertexCache.indexCount = 2*primitiveCount;
			VertexCache.primitivePitch = 2;
			break;
		case scene::EPT_TRIANGLE_STRIP:
			VertexCache.indexCount = primitiveCount+2;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_TRIANGLES:
			VertexCache.indexCount = primitiveCount + primitiveCount + primitiveCount;
			VertexCache.primitivePitch = 3;
			break;
		case scene::EPT_TRIANGLE_FAN:
			VertexCache.indexCount = primitiveCount + 2;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_QUAD_STRIP:
			VertexCache.indexCount = 2*primitiveCount + 2;
			VertexCache.primitivePitch = 2;
			break;
		case scene::EPT_QUADS:
			VertexCache.indexCount = 4*primitiveCount;
			VertexCache.primitivePitch = 4;
			break;
		case scene::EPT_POLYGON:
			VertexCache.indexCount = primitiveCount+1;
			VertexCache.primitivePitch = 1;
			break;
		case scene::EPT_POINT_SPRITES:
			VertexCache.indexCount = primitiveCount;
			VertexCache.primitivePitch = 1;
			break;
	}

	irr::memset32 ( VertexCache.info, VERTEXCACHE_MISS, sizeof ( VertexCache.info ) );
}


void CBurningVideoDriver::drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)

{
	if (!checkPrimitiveCount(primitiveCount))
		return;

	CNullDriver::drawVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType, iType);

	// These calls would lead to crashes due to wrong index usage.
	// The vertex cache needs to be rewritten for these primitives.
	if (pType==scene::EPT_POINTS || pType==scene::EPT_LINE_STRIP ||
		pType==scene::EPT_LINE_LOOP || pType==scene::EPT_LINES ||
		pType==scene::EPT_POLYGON ||
		pType==scene::EPT_POINT_SPRITES)
		return;

	if ( 0 == CurrentShader )
		return;

	VertexCache_reset ( vertices, vertexCount, indexList, primitiveCount, vType, pType, iType );
	
	transform_calc(ETS_PROJ_MODEL_VIEW);
/*
	if ( LightSpace.enabled && ( Material.org.Lighting || (LightSpace.Flags & TEXTURE_TRANSFORM)) )
	{
		transform_calc(ETS_VIEW_INVERSE);
	}
*/
	if ( Material.org.Lighting || (EyeSpace.Flags & TEXTURE_TRANSFORM) )
	{
		transform_calc(ETS_MODEL_VIEW);
		transform_calc(ETS_NORMAL);
	}

	s4DVertex* face[3];

	f32 dc_area;
	s32 lodLevel;
	u32 i;
	u32 g;
	u32 m;
	video::CSoftwareTexture2* tex;

	for ( i = 0; i < (u32) primitiveCount; ++i )
	{
		VertexCache_get(face);

		// if fully outside or outside on same side
		if ( ( (face[0]->flag | face[1]->flag | face[2]->flag) & VERTEX4D_CLIPMASK )
				!= VERTEX4D_INSIDE
			)
			continue;

		// if fully inside
		if ( ( face[0]->flag & face[1]->flag & face[2]->flag & VERTEX4D_CLIPMASK ) == VERTEX4D_INSIDE )
		{
			dc_area = screenarea2 ( face );
			u32 sign = dc_area < -AreaMinDrawSize ? CULL_BACK: dc_area > AreaMinDrawSize ? CULL_FRONT : CULL_INVISIBLE;
			if ( Material.Culling & sign ) 
				continue;

			// select mipmap
			dc_area = reciprocal_zero ( dc_area );
			for ( m = 0; m < BURNING_MATERIAL_MAX_TEXTURES && m != vSize[VertexCache.vType].TexSize; ++m )
			{
				if ( 0 == (tex = MAT_TEXTURE ( m )) )
				{
					CurrentShader->setTextureParam(m, 0, 0);
					continue;
				}

				f32 texArea = texelarea2 ( face, m ); 
				lodLevel = s32_log2_f32 ( texArea * dc_area  );

/*
				{
					char buf[256];
					sprintf ( buf,"level:%d %s tex:%f dc:%f\n",lodLevel,texName,texArea,dc_area);
					//OutputDebugString(buf);
				}
*/
				CurrentShader->setTextureParam(m, tex, lodLevel );
				select_polygon_mipmap2 ( face, m, tex->getTexBound() );
			}

			// rasterize
			CurrentShader->drawWireFrameTriangle ( face[0] + 1, face[1] + 1, face[2] + 1 );
			continue;
		}

		// else if not complete inside clipping necessary
		memcpy32_small ( ( (u8*) CurrentOut.data + ( 0 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[0], SIZEOF_SVERTEX * 2 );
		memcpy32_small ( ( (u8*) CurrentOut.data + ( 1 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[1], SIZEOF_SVERTEX * 2 );
		memcpy32_small ( ( (u8*) CurrentOut.data + ( 2 << ( SIZEOF_SVERTEX_LOG2 + 1 ) ) ), face[2], SIZEOF_SVERTEX * 2 );

		//clear clipping & projected flags
		const u32 flag = CurrentOut.data->flag & VERTEX4D_FORMAT_MASK;
		for ( g = 0; g != CurrentOut.ElementSize; ++g )
		{
			CurrentOut.data[g].flag = flag;
			Temp.data[g].flag = flag;
		}

		u32 vOut;
		vOut = clipToFrustum ( CurrentOut.data, Temp.data, 3 );
		if ( vOut < 3 )
			continue;

		vOut <<= 1;

		// to DC Space, project homogenous vertex
		ndc_2_dc_and_project ( CurrentOut.data + 1, CurrentOut.data, vOut );

		// check 2d backface culling on first
		dc_area = screenarea ( CurrentOut.data );
		u32 sign = dc_area < -AreaMinDrawSize ? CULL_BACK: dc_area > AreaMinDrawSize ? CULL_FRONT : CULL_INVISIBLE;
		if ( Material.Culling & sign ) 
			continue;

		// select mipmap
		dc_area = reciprocal_zero ( dc_area );
		for ( m = 0; m != vSize[VertexCache.vType].TexSize; ++m )
		{
			if ( 0 == (tex = MAT_TEXTURE ( m )) )
			{
				CurrentShader->setTextureParam(m, 0, 0);
				continue;
			}

			lodLevel = s32_log2_f32 ( texelarea ( CurrentOut.data, m ) * dc_area );
			CurrentShader->setTextureParam(m, tex, lodLevel );
			select_polygon_mipmap ( CurrentOut.data, vOut, m, tex->getTexBound() );
		}


		// re-tesselate ( triangle-fan, 0-1-2,0-2-3.. )
		for ( g = 0; g <= vOut - 6; g += 2 )
		{
			// rasterize
			CurrentShader->drawWireFrameTriangle ( CurrentOut.data + 0 + 1,
							CurrentOut.data + g + 3,
							CurrentOut.data + g + 5);
		}

	}

	// dump statistics
/*
	char buf [64];
	sprintf ( buf,"VCount:%d PCount:%d CacheMiss: %d",
					vertexCount, primitiveCount,
					VertexCache.CacheMiss
				);
	os::Printer::log( buf );
*/

}


//! Sets the dynamic ambient light color. The default color is
//! (0,0,0,0) which means it is dark.
//! \param color: New color of the ambient light.
void CBurningVideoDriver::setAmbientLight(const SColorf& color)
{
	LightSpace.Global_AmbientLight.setColorf ( color );
	EyeSpace.Global_AmbientLight.setColorf ( color );
}


//! adds a dynamic light
s32 CBurningVideoDriver::addDynamicLight(const SLight& dl)
{
	(void) CNullDriver::addDynamicLight( dl );

	SBurningShaderLight l;
//	l.org = dl;
	l.Type = dl.Type;
	l.LightIsOn = true;

	l.AmbientColor.setColorf ( dl.AmbientColor );
	l.DiffuseColor.setColorf ( dl.DiffuseColor );
	l.SpecularColor.setColorf ( dl.SpecularColor );

	core::vector3df nDirection = dl.Direction;
	nDirection.normalize();

	switch ( dl.Type )
	{
		case ELT_DIRECTIONAL:
			l.pos.x = dl.Position.X;
			l.pos.y = dl.Position.Y;
			l.pos.z = dl.Position.Z;
			l.pos.w = 0.f;

			l.spotDirection.x = -nDirection.X;
			l.spotDirection.y = -nDirection.Y;
			l.spotDirection.z = -nDirection.Z;
			l.spotDirection.w = 0.f;

			l.constantAttenuation = 1.f;
			l.linearAttenuation = 0.f;
			l.quadraticAttenuation = 0.f;

			l.spotCosCutoff = -1.f;
			l.spotCosInnerCutoff = 1.f;
			l.spotExponent = 0.f;

			break;
		case ELT_POINT:
			l.pos.x = dl.Position.X;
			l.pos.y = dl.Position.Y;
			l.pos.z = dl.Position.Z;
			l.pos.w = 1.f;

			l.constantAttenuation = dl.Attenuation.X;
			l.linearAttenuation = dl.Attenuation.Y;
			l.quadraticAttenuation = dl.Attenuation.Z;

			l.spotCosCutoff = -1.f;
			l.spotCosInnerCutoff = 1.f;
			l.spotExponent = 0.f;
			break;

		case ELT_SPOT:
			l.pos.x = dl.Position.X;
			l.pos.y = dl.Position.Y;
			l.pos.z = dl.Position.Z;
			l.pos.w = 1.f;

			l.spotDirection.x = nDirection.X;
			l.spotDirection.y = nDirection.Y;
			l.spotDirection.z = nDirection.Z;
			l.spotDirection.w = 0.0f;

			l.constantAttenuation = dl.Attenuation.X;
			l.linearAttenuation = dl.Attenuation.Y;
			l.quadraticAttenuation = dl.Attenuation.Z;

			l.spotCosCutoff = cosf(dl.OuterCone * 2.0f * core::DEGTORAD * 0.5f);
			l.spotCosInnerCutoff = cosf(dl.InnerCone * 2.0f * core::DEGTORAD * 0.5f);
			l.spotExponent = dl.Falloff;
			break;
		default:
			break;
	}
/*
	if ( LightSpace.enabled )
	{
		l.pos4 = l.pos;
		l.spotDirection4 = l.spotDirection;
		LightSpace.Light.push_back ( l );
	}
*/	
	//which means ETS_VIEW
	setTransform(ETS_WORLD,irr::core::IdentityMatrix);
	transform_calc(ETS_MODEL_VIEW);
	Transformation[ETS_MODEL_VIEW].transformVec4 ( &l.pos4.x, &l.pos.x );
	Transformation[ETS_MODEL_VIEW].rotateVec4 ( &l.spotDirection4.x, &l.spotDirection.x );
	EyeSpace.Light.push_back ( l );

	return EyeSpace.Light.size() - 1;
}

//! Turns a dynamic light on or off
void CBurningVideoDriver::turnLightOn(s32 lightIndex, bool turnOn)
{
	if((u32)lightIndex < LightSpace.Light.size())
	{
		LightSpace.Light[lightIndex].LightIsOn = turnOn;
	}
	if((u32)lightIndex < EyeSpace.Light.size())
	{
		EyeSpace.Light[lightIndex].LightIsOn = turnOn;
	}
}

//! deletes all dynamic lights there are
void CBurningVideoDriver::deleteAllDynamicLights()
{
	LightSpace.reset ();
	EyeSpace.reset ();
	CNullDriver::deleteAllDynamicLights();

}

//! returns the maximal amount of dynamic lights the device can handle
u32 CBurningVideoDriver::getMaximalDynamicLightAmount() const
{
	return 8; //no limit 8 only for convenience
}


//! sets a material
void CBurningVideoDriver::setMaterial(const SMaterial& material)
{
	Material.org = material;
	Material.Culling = CULL_INVISIBLE | (material.BackfaceCulling ? CULL_BACK : 0) | (material.FrontfaceCulling ? CULL_FRONT : 0);

#ifdef SOFTWARE_DRIVER_2_TEXTURE_TRANSFORM
	for (u32 i = 0; i < 2; ++i)
	{
		setTransform((E_TRANSFORMATION_STATE) (ETS_TEXTURE_0 + i),
				material.getTextureMatrix(i));
	}
#endif

#ifdef SOFTWARE_DRIVER_2_LIGHTING
	Material.AmbientColor.setR8G8B8 ( Material.org.AmbientColor.color );
	Material.DiffuseColor.setR8G8B8 ( Material.org.DiffuseColor.color );
	Material.EmissiveColor.setR8G8B8 ( Material.org.EmissiveColor.color );
	Material.SpecularColor.setR8G8B8 ( Material.org.SpecularColor.color );

	core::setbit_cond ( EyeSpace.Flags, (Material.org.Shininess != 0.f) & (Material.org.SpecularColor.color & 0x00ffffff), SPECULAR );
	core::setbit_cond ( EyeSpace.Flags, Material.org.FogEnable, FOG );
	core::setbit_cond ( EyeSpace.Flags, Material.org.NormalizeNormals, NORMALIZE_NORMALS );
	if (EyeSpace.Flags & SPECULAR ) EyeSpace.Flags |= NORMALIZE_NORMALS;
	LightSpace.Flags = EyeSpace.Flags;
#endif

	setCurrentShader();
}



//! Sets the fog mode.
void CBurningVideoDriver::setFog(SColor color, E_FOG_TYPE fogType, f32 start,
	f32 end, f32 density, bool pixelFog, bool rangeFog)
{
	CNullDriver::setFog(color, fogType, start, end, density, pixelFog, rangeFog);
	LightSpace.FogColor.setA8R8G8B8 ( color.color );
	EyeSpace.FogColor.setA8R8G8B8 ( color.color );
}

#if defined(SOFTWARE_DRIVER_2_LIGHTING) && BURNING_MATERIAL_MAX_COLORS > 0

/*!
	applies lighting model
*/
void CBurningVideoDriver::lightVertex_world ( s4DVertex *dest, u32 vertexargb )
{
	sVec3 dColor;

	dColor = Material.EmissiveColor;
	dColor.mulAdd (LightSpace.Global_AmbientLight, Material.AmbientColor );

	if ( Lights.size () == 0 )
	{
		dColor.saturate( dest->Color[0], vertexargb);
		return;
	}

	sVec3 ambient;
	sVec3 diffuse;
	sVec3 specular;


	// the universe started in darkness..
	ambient.set ( 0.f, 0.f, 0.f );
	diffuse.set ( 0.f, 0.f, 0.f );
	specular.set ( 0.f, 0.f, 0.f );


	u32 i;
	f32 dot;
	f32 distance;
	f32 attenuation;
	sVec4 vp;			// unit vector vertex to light
	//sVec4 lightHalf;	// blinn-phong reflection
	sVec4 R; // normalize(-reflect(L,Lightspace.normal))
	sVec4 E; // normalize(-v); // Eye Coordinates(0,0,0) = LightSpace.campos

	f32 spotDot;			// cos of angle between spotlight and point on surface

	for ( i = 0; i!= LightSpace.Light.size (); ++i )
	{
		const SBurningShaderLight &light = LightSpace.Light[i];

		if ( !light.LightIsOn )
			continue;

		switch ( light.Type )
		{
			case ELT_DIRECTIONAL:

				//angle between normal and light vector
				dot = LightSpace.normal.dot_xyz ( light.spotDirection );

				// accumulate ambient
				ambient.add ( light.AmbientColor );

				// diffuse component
				if ( dot > 0.f )
					diffuse.mulAdd ( light.DiffuseColor, dot );
				break;

			case ELT_POINT:
				// surface to light
				vp.x = light.pos.x - LightSpace.vertex.x;
				vp.y = light.pos.y - LightSpace.vertex.y;
				vp.z = light.pos.z - LightSpace.vertex.z;

				distance = vp.get_length_xyz();

				attenuation = 1.f / (  light.constantAttenuation
					                 + light.linearAttenuation * distance
  								     /*+ light.quadraticAttenuation * (distance * distance)*/
								);

				// accumulate ambient
				ambient.mulAdd ( light.AmbientColor,attenuation );

				// build diffuse reflection

				//angle between normal and light vector
				vp.mul ( reciprocal_zero( distance ) );
				dot = LightSpace.normal.dot_xyz ( vp );
				if ( dot <= 0.f ) continue;

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, dot * attenuation );

				if ( !(LightSpace.Flags & SPECULAR) )
					continue;

				R.x = -(vp.x - 2.f * dot * LightSpace.normal.x);
				R.y = -(vp.y - 2.f * dot * LightSpace.normal.y);
				R.z = -(vp.z - 2.f * dot * LightSpace.normal.z);
				R.normalize_xyz();

				E.x = LightSpace.campos.x - LightSpace.vertex.x;
				E.y = LightSpace.campos.y - LightSpace.vertex.y;
				E.z = LightSpace.campos.z - LightSpace.vertex.z;
				E.normalize_xyz();

				if ( (dot = R.dot_xyz(E)) > 0.f)
				{
					f32 srgb = powf ( dot,Material.org.Shininess );
					specular.mulAdd ( light.SpecularColor, srgb * attenuation );
				}
				break;

			case ELT_SPOT:
				// surface to light
				vp.x = light.pos.x - LightSpace.vertex.x;
				vp.y = light.pos.y - LightSpace.vertex.y;
				vp.z = light.pos.z - LightSpace.vertex.z;

				distance = vp.get_length_xyz();

				//normalize
				vp.mul ( reciprocal_zero( distance ) );

				// point on surface inside cone of illumination
				spotDot = light.spotDirection.dot_xyz(vp);
				if (spotDot > light.spotCosCutoff)
					continue;


				attenuation = 1.f / (light.constantAttenuation + light.linearAttenuation * distance
  								+light.quadraticAttenuation * (distance * distance));
				attenuation *= powf (-spotDot,light.spotExponent);

				// accumulate ambient
				ambient.mulAdd ( light.AmbientColor,attenuation );


				// build diffuse reflection
				//angle between normal and light vector
				dot = LightSpace.normal.dot_xyz ( vp );
				if ( dot < 0.f ) continue;

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, dot * attenuation );

				if ( !(LightSpace.Flags & SPECULAR) )
					continue;

				R.x = -(vp.x - 2.f * dot * LightSpace.normal.x);
				R.y = -(vp.y - 2.f * dot * LightSpace.normal.y);
				R.z = -(vp.z - 2.f * dot * LightSpace.normal.z);
				R.normalize_xyz();

				E.x = LightSpace.campos.x - LightSpace.vertex.x;
				E.y = LightSpace.campos.y - LightSpace.vertex.y;
				E.z = LightSpace.campos.z - LightSpace.vertex.z;
				E.normalize_xyz();

				if ( (dot = R.dot_xyz(E)) > 0.f)
				{
					f32 srgb = powf ( dot, Material.org.Shininess );
					specular.mulAdd ( light.SpecularColor, srgb * attenuation );
				}
				break;

			default:
				break;
		}

	}

	// sum up lights
	dColor.mulAdd (ambient, Material.AmbientColor );
	dColor.mulAdd (diffuse, Material.DiffuseColor);

	//has to move to shader (for vertex color only this will fit [expect clamping])

	sVec3 c;
	c.setR8G8B8(vertexargb);
	dColor.r *= c.r;
	dColor.g *= c.g;
	dColor.b *= c.b;

	//separate specular
#if defined(SOFTWARE_DRIVER_2_USE_SEPARATE_SPECULAR_COLOR) && 1
	dest->Color[1].a = 1.f;
	dest->Color[1].r = specular.r * Material.SpecularColor.r;
	dest->Color[1].g = specular.g * Material.SpecularColor.g;
	dest->Color[1].b = specular.b * Material.SpecularColor.b;
#else
	dColor.mulAdd (specular, Material.SpecularColor);
#endif

	dColor.saturate ( dest->Color[0], vertexargb );
}

/*!
	applies lighting model
*/
void CBurningVideoDriver::lightVertex_eye ( s4DVertex *dest, u32 vertexargb )
{
	sVec3 dColor;

	//gl_FrontLightModelProduct.sceneColor = gl_FrontMaterial.emission + gl_FrontMaterial.ambient * gl_LightModel.ambient
	dColor = Material.EmissiveColor;
	dColor.mulAdd (EyeSpace.Global_AmbientLight, Material.AmbientColor );

	if ( EyeSpace.Light.size () == 0 )
	{
		dColor.saturate( dest->Color[0], vertexargb);
		return;
	}

	sVec3 ambient;
	sVec3 diffuse;
	sVec3 specular;


	// the universe started in darkness..
	ambient.set ( 0.f, 0.f, 0.f );
	diffuse.set ( 0.f, 0.f, 0.f );
	specular.set ( 0.f, 0.f, 0.f );


	u32 i;
	f32 dot;
	f32 distance;
	f32 attenuation;
	sVec4 vp;			// unit vector vertex to light
	sVec4 lightHalf;	// blinn-phong reflection

	f32 spotDot;			// cos of angle between spotlight and point on surface

	for ( i = 0; i < EyeSpace.Light.size (); ++i )
	{
		const SBurningShaderLight &light = EyeSpace.Light[i];

		if ( !light.LightIsOn )
			continue;

		switch ( light.Type )
		{
			case ELT_DIRECTIONAL:

				//angle between normal and light vector
				dot = EyeSpace.normal3.dot_xyz ( light.spotDirection4 );

				// accumulate ambient
				ambient.add ( light.AmbientColor );

				// diffuse component
				if ( dot > 0.f )
					diffuse.mulAdd ( light.DiffuseColor, dot );
				break;

			case ELT_POINT:
				// surface to light
				vp.x = light.pos4.x - EyeSpace.vertex3.x;
				vp.y = light.pos4.y - EyeSpace.vertex3.y;
				vp.z = light.pos4.z - EyeSpace.vertex3.z;

				distance = vp.get_length_xyz();

				attenuation = light.constantAttenuation
					          + light.linearAttenuation * distance
					          + light.quadraticAttenuation * (distance * distance);
				attenuation = reciprocal_one(attenuation);

				//att = clamp(1.0 - dist/radius, 0.0, 1.0); att *= att

				// accumulate ambient
				ambient.mulAdd ( light.AmbientColor,attenuation );

				// build diffuse reflection

				//angle between normal and light vector
				vp.mul ( reciprocal_zero( distance ) ); //normalize
				dot = EyeSpace.normal3.dot_xyz ( vp );
				if ( dot <= 0.f ) continue;

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, dot * attenuation );

				if ( !(EyeSpace.Flags & SPECULAR) )
					continue;
/*
				lightHalf.x = vp.x + EyeSpace.campos.x;
				lightHalf.y = vp.y + EyeSpace.campos.y;
				lightHalf.z = vp.z + EyeSpace.campos.z;
				lightHalf.normalize_xyz();

				dot = EyeSpace.normal3.dot_xyz ( lightHalf );
				if ( dot > 0.f)
*/
				{
					f32 srgb = powf ( dot,Material.org.Shininess );
					specular.mulAdd ( light.SpecularColor, srgb * attenuation );
				}
				break;

			case ELT_SPOT:
				// surface to light
				vp.x = light.pos4.x - EyeSpace.vertex3.x;
				vp.y = light.pos4.y - EyeSpace.vertex3.y;
				vp.z = light.pos4.z - EyeSpace.vertex3.z;

				distance = vp.get_length_xyz();

				//normalize
				vp.mul ( reciprocal_zero( distance ) );

				// point on surface inside cone of illumination
				spotDot = vp.dot_minus_xyz(light.spotDirection4);
				if (spotDot < light.spotCosCutoff)
					continue;

				attenuation = light.constantAttenuation
					          + light.linearAttenuation * distance
					          + light.quadraticAttenuation * distance * distance;
				attenuation = reciprocal_one(attenuation);
				attenuation *= powf (spotDot,light.spotExponent);

				// accumulate ambient
				ambient.mulAdd ( light.AmbientColor,attenuation );


				// build diffuse reflection
				//angle between normal and light vector
				dot = EyeSpace.normal3.dot_xyz ( vp );
				if ( dot < 0.f ) continue;

				// diffuse component
				diffuse.mulAdd ( light.DiffuseColor, dot * attenuation );

				if ( !(EyeSpace.Flags & SPECULAR) )
					continue;
/*
				lightHalf.x = vp.x + EyeSpace.campos.x;
				lightHalf.y = vp.y + EyeSpace.campos.y;
				lightHalf.z = vp.z + EyeSpace.campos.z;
				lightHalf.normalize_xyz();

				dot = EyeSpace.normal3.dot_xyz ( lightHalf );
				if ( dot > 0.f)
*/
				{
					f32 srgb = powf ( dot,Material.org.Shininess );
					specular.mulAdd ( light.SpecularColor, srgb * attenuation );
				}
				break;

			default:
				break;
		}

	}

	// sum up lights
	dColor.mulAdd (ambient, Material.AmbientColor );
	dColor.mulAdd (diffuse, Material.DiffuseColor);

	//has to move to shader (for vertex color only this will fit [expect clamping])

	sVec3 c;
	c.setR8G8B8(vertexargb);
	dColor.r *= c.r;
	dColor.g *= c.g;
	dColor.b *= c.b;

	//separate specular
#if defined(SOFTWARE_DRIVER_2_USE_SEPARATE_SPECULAR_COLOR) && 1
	dest->Color[1].a = 1.f;
	dest->Color[1].r = specular.r * Material.SpecularColor.r;
	dest->Color[1].g = specular.g * Material.SpecularColor.g;
	dest->Color[1].b = specular.b * Material.SpecularColor.b;
#else
	dColor.mulAdd (specular, Material.SpecularColor);
#endif

	dColor.saturate ( dest->Color[0], vertexargb );
}

#endif

//setup a quad
#if defined SOFTWARE_DRIVER_2_2D_AS_3D

//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
					 const core::rect<s32>& sourceRect,
					 const core::rect<s32>* clipRect, SColor color,
					 bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	if (!sourceRect.isValid())
		return;

	// clip these coordinates
	core::rect<s32> targetRect(destPos, sourceRect.getSize());
	if (clipRect)
	{
		targetRect.clipAgainst(*clipRect);
		if ( targetRect.getWidth() < 0 || targetRect.getHeight() < 0 )
			return;
	}

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();
	targetRect.clipAgainst( core::rect<s32>(0,0, (s32)renderTargetSize.Width, (s32)renderTargetSize.Height) );
	if ( targetRect.getWidth() < 0 || targetRect.getHeight() < 0 )
			return;

	// ok, we've clipped everything.
	// now draw it.
	const core::dimension2d<s32> sourceSize(targetRect.getSize());
	const core::position2d<s32> sourcePos(sourceRect.UpperLeftCorner + (targetRect.UpperLeftCorner-destPos));

	const core::dimension2d<u32>& tex_orgsize = texture->getOriginalSize();
	const f32 invW = 1.f / static_cast<f32>(tex_orgsize.Width);
	const f32 invH = 1.f / static_cast<f32>(tex_orgsize.Height);
	const core::rect<f32> tcoords(
		sourcePos.X * invW,
		sourcePos.Y * invH,
		(sourcePos.X + sourceSize.Width) * invW,
		(sourcePos.Y + sourceSize.Height) * invH);

#if 0
	disableTextures(1);
	if (!CacheHandler->getTextureCache().set(0, texture))
		return;
	setRenderStates2DMode(color.getAlpha()<255, true, useAlphaChannelOfTexture);

	Quad2DVertices[0].Color = color;
	Quad2DVertices[1].Color = color;
	Quad2DVertices[2].Color = color;
	Quad2DVertices[3].Color = color;

	Quad2DVertices[0].Pos = core::vector3df((f32)targetRect.UpperLeftCorner.X, (f32)targetRect.UpperLeftCorner.Y, 0.0f);
	Quad2DVertices[1].Pos = core::vector3df((f32)targetRect.LowerRightCorner.X, (f32)targetRect.UpperLeftCorner.Y, 0.0f);
	Quad2DVertices[2].Pos = core::vector3df((f32)targetRect.LowerRightCorner.X, (f32)targetRect.LowerRightCorner.Y, 0.0f);
	Quad2DVertices[3].Pos = core::vector3df((f32)targetRect.UpperLeftCorner.X, (f32)targetRect.LowerRightCorner.Y, 0.0f);

	Quad2DVertices[0].TCoords = core::vector2df(tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	Quad2DVertices[1].TCoords = core::vector2df(tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	Quad2DVertices[2].TCoords = core::vector2df(tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	Quad2DVertices[3].TCoords = core::vector2df(tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);

	if (!FeatureAvailable[IRR_ARB_vertex_array_bgra] && !FeatureAvailable[IRR_EXT_vertex_array_bgra])
		getColorBuffer(Quad2DVertices, 4, EVT_STANDARD);

	CacheHandler->setClientState(true, false, true, true);

	glTexCoordPointer(2, GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex*>(Quad2DVertices))[0].TCoords);
	glVertexPointer(2, GL_FLOAT, sizeof(S3DVertex), &(static_cast<const S3DVertex*>(Quad2DVertices))[0].Pos);

#ifdef GL_BGRA
	const GLint colorSize = (FeatureAvailable[IRR_ARB_vertex_array_bgra] || FeatureAvailable[IRR_EXT_vertex_array_bgra]) ? GL_BGRA : 4;
#else
	const GLint colorSize = 4;
#endif
	if (FeatureAvailable[IRR_ARB_vertex_array_bgra] || FeatureAvailable[IRR_EXT_vertex_array_bgra])
		glColorPointer(colorSize, GL_UNSIGNED_BYTE, sizeof(S3DVertex), &(static_cast<const S3DVertex*>(Quad2DVertices))[0].Color);
	else
	{
		_IRR_DEBUG_BREAK_IF(ColorBuffer.size() == 0);
		glColorPointer(colorSize, GL_UNSIGNED_BYTE, 0, &ColorBuffer[0]);
	}

	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, Quad2DIndices);
#endif
}

//! Draws a part of the texture into the rectangle.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
		const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect,
		const video::SColor* const colors, bool useAlphaChannelOfTexture)
{
}


#else // SOFTWARE_DRIVER_2_2D_AS_3D

//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
					 const core::rect<s32>& sourceRect,
					 const core::rect<s32>* clipRect, SColor color,
					 bool useAlphaChannelOfTexture)
{
	if (!texture) return;

	CImage* img = ((CSoftwareTexture2*)texture)->getImage();
	eBlitter op = useAlphaChannelOfTexture ? (color.color == 0xFFFFFFFF ? BLITTER_TEXTURE_ALPHA_BLEND: BLITTER_TEXTURE_ALPHA_COLOR_BLEND) : BLITTER_TEXTURE;
	if (texture->getOriginalSize() == texture->getSize() )
	{
		Blit(op, RenderTargetSurface, clipRect, &destPos, img, &sourceRect,&texture->getOriginalSize(),color.color);
	}
	else
	{
		core::rect<s32> destRect(destPos,sourceRect.getSize());
		SColor c[4] = { color,color,color,color };
		//StretchBlit(op,RenderTargetSurface, 0,&destRect, img,&sourceRect,&texture->getOriginalSize(),color.color);
		draw2DImage(texture,destRect,sourceRect,0,c,useAlphaChannelOfTexture);
	}

}


//! Draws a part of the texture into the rectangle.
void CBurningVideoDriver::draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
		const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect,
		const video::SColor* const colors, bool useAlphaChannelOfTexture)
{
	if (!texture) return;

	if ( OverrideMaterial2DEnabled ) {}

	CImage* img = ((CSoftwareTexture2*)texture)->getImage();
	u32 argb = (colors ? colors[0].color : 0xFFFFFFFF);
	eBlitter op = useAlphaChannelOfTexture ? (argb == 0xFFFFFFFF ? BLITTER_TEXTURE_ALPHA_BLEND: BLITTER_TEXTURE_ALPHA_COLOR_BLEND) : BLITTER_TEXTURE;
	if ( op == BLITTER_TEXTURE )
	{
		core::rect<s32> src(sourceRect);
		const core::dimension2d<u32>& o = texture->getOriginalSize();
		const core::dimension2d<u32>& w = texture->getSize();
		if ( o != w)
		{
			src.UpperLeftCorner.X = (sourceRect.UpperLeftCorner.X*w.Width)/o.Width;
			src.UpperLeftCorner.Y = (sourceRect.UpperLeftCorner.Y*w.Height)/o.Height;
			src.LowerRightCorner.X = (sourceRect.LowerRightCorner.X*w.Width)/o.Width;
			src.LowerRightCorner.Y = (sourceRect.LowerRightCorner.Y*w.Height)/o.Height;
		}
		Resample_subSampling(op,RenderTargetSurface,&destRect,img,&src);
	}
	else
	{
		StretchBlit(op,RenderTargetSurface, clipRect,&destRect, img,&sourceRect,&texture->getOriginalSize(),argb);
	}
}

#endif // SOFTWARE_DRIVER_2_2D_AS_3D

//!Draws an 2d rectangle with a gradient.
void CBurningVideoDriver::draw2DRectangle(const core::rect<s32>& position,
	SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
	const core::rect<s32>* clip)
{
#if defined SOFTWARE_DRIVER_2_2D_AS_3D || 1
	core::rect<s32> pos = position;
	if (clip) pos.clipAgainst(*clip);
	if (!pos.isValid()) return;

	const core::dimension2d<s32> renderTargetSize ( ViewPort.getSize() );

	const f32 xPlus = -(renderTargetSize.Width*0.5f);
	const f32 xFact = 1.0f / (renderTargetSize.Width*0.5f);

	const f32 yPlus = renderTargetSize.Height-(renderTargetSize.Height*0.5f);
	const f32 yFact = 1.0f / (renderTargetSize.Height*0.5f);

	// fill VertexCache direct
	VertexCache.vertexCount = 4;
	VertexCache.vType = 0;

	VertexCache.info[0].index = 0;
	VertexCache.info[1].index = 1;
	VertexCache.info[2].index = 2;
	VertexCache.info[3].index = 3;

	s4DVertex* v = &VertexCache.mem.data [ 0 ];

	v[0].Pos.set ( (pos.UpperLeftCorner.X+xPlus)  * xFact, (yPlus-pos.UpperLeftCorner.Y)  * yFact, 0.f, 1.f );
	v[2].Pos.set ( (pos.LowerRightCorner.X+xPlus) * xFact, (yPlus-pos.UpperLeftCorner.Y)  * yFact, 0.f, 1.f );
	v[4].Pos.set ( (pos.LowerRightCorner.X+xPlus) * xFact, (yPlus-pos.LowerRightCorner.Y) * yFact, 0.f ,1.f );
	v[6].Pos.set ( (pos.UpperLeftCorner.X+xPlus)  * xFact, (yPlus-pos.LowerRightCorner.Y) * yFact, 0.f, 1.f );

#if BURNING_MATERIAL_MAX_COLORS > 0
	v[0].Color[0].setA8R8G8B8 ( colorLeftUp.color );
	v[2].Color[0].setA8R8G8B8 ( colorRightUp.color );
	v[4].Color[0].setA8R8G8B8 ( colorRightDown.color );
	v[6].Color[0].setA8R8G8B8 ( colorLeftDown.color );
#endif

	s32 i;
	for ( i = 0; i < 8; i += 2 )
	{
		v[i + 0].flag = clipToFrustumTest ( v + i ) | vSize[VertexCache.vType].Format;
		v[i + 1].flag = vSize[VertexCache.vType].Format;
		if ( (v[i].flag & VERTEX4D_INSIDE ) == VERTEX4D_INSIDE )
		{
			ndc_2_dc_and_project ( v + i + 1, v + i, 2 );
		}
	}


	IBurningShader * render = BurningShader [ ETR_GOURAUD_ALPHA_NOZ_NOPERSPECTIVE_CORRECT ];
	render->setRenderTarget(RenderTargetSurface, ViewPort);

	static const s16 indexList[6] = {0,1,2,0,2,3};

	s4DVertex * face[3];

	for ( i = 0; i < 6; i += 3 )
	{
		face[0] = VertexCache_getVertex ( indexList [ i + 0 ] );
		face[1] = VertexCache_getVertex ( indexList [ i + 1 ] );
		face[2] = VertexCache_getVertex ( indexList [ i + 2 ] );

		// test clipping
		u32 test = face[0]->flag & face[1]->flag & face[2]->flag & VERTEX4D_INSIDE;

		if ( test == VERTEX4D_INSIDE )
		{
			render->drawTriangle ( face[0] + 1, face[1] + 1, face[2] + 1 );
			continue;
		}
		// Todo: all vertices are clipped in 2d..
		// is this true ?
		u32 vOut = 6;
		memcpy32_small ( CurrentOut.data + 0, face[0], sizeof ( s4DVertex ) * 2 );
		memcpy32_small ( CurrentOut.data + 2, face[1], sizeof ( s4DVertex ) * 2 );
		memcpy32_small ( CurrentOut.data + 4, face[2], sizeof ( s4DVertex ) * 2 );

		//clear clipping & projected flags
		const u32 flag = CurrentOut.data->flag & VERTEX4D_FORMAT_MASK;
		for ( u32 g = 0; g != CurrentOut.ElementSize; ++g )
		{
			CurrentOut.data[g].flag = flag;
			Temp.data[g].flag = flag;
		}

		vOut = clipToFrustum ( CurrentOut.data, Temp.data, 3 );
		if ( vOut < 3 )
			continue;

		vOut <<= 1;
		// to DC Space, project homogenous vertex
		ndc_2_dc_and_project ( CurrentOut.data + 1, CurrentOut.data, vOut );

		// re-tesselate ( triangle-fan, 0-1-2,0-2-3.. )
		for ( u32 g = 0; g <= vOut - 6; g += 2 )
		{
			// rasterize
			render->drawTriangle ( CurrentOut.data + 1, CurrentOut.data + g + 3, CurrentOut.data + g + 5 );
		}

	}

#else
	draw2DRectangle ( colorLeftUp, position, clip );
#endif // SOFTWARE_DRIVER_2_2D_AS_3D
}


//! Draws a 2d line.
void CBurningVideoDriver::draw2DLine(const core::position2d<s32>& start,
					const core::position2d<s32>& end,
					SColor color)
{
	drawLine(BackBuffer, start, end, color );
}


//! Draws a pixel
void CBurningVideoDriver::drawPixel(u32 x, u32 y, const SColor & color)
{
	BackBuffer->setPixel(x, y, color, true);
}


//! draw an 2d rectangle
void CBurningVideoDriver::draw2DRectangle(SColor color, const core::rect<s32>& pos,
									 const core::rect<s32>* clip)
{
	if (clip)
	{
		core::rect<s32> p(pos);

		p.clipAgainst(*clip);

		if(!p.isValid())
			return;

		drawRectangle(BackBuffer, p, color);
	}
	else
	{
		if(!pos.isValid())
			return;

		drawRectangle(BackBuffer, pos, color);
	}
}


//! Only used by the internal engine. Used to notify the driver that
//! the window was resized.
void CBurningVideoDriver::OnResize(const core::dimension2d<u32>& size)
{
	// make sure width and height are multiples of 2
	core::dimension2d<u32> realSize(size);
/*
	if (realSize.Width % 2)
		realSize.Width += 1;

	if (realSize.Height % 2)
		realSize.Height += 1;
*/
	if (ScreenSize != realSize)
	{
		if (ViewPort.getWidth() == (s32)ScreenSize.Width &&
			ViewPort.getHeight() == (s32)ScreenSize.Height)
		{
			ViewPort.UpperLeftCorner.X = 0;
			ViewPort.UpperLeftCorner.Y = 0;
			ViewPort.LowerRightCorner.X = realSize.Width;
			ViewPort.LowerRightCorner.X = realSize.Height;
		}

		ScreenSize = realSize;

		bool resetRT = (RenderTargetSurface == BackBuffer);

		if (BackBuffer)
			BackBuffer->drop();
		BackBuffer = new CImage(BURNINGSHADER_COLOR_FORMAT, realSize);

		if (resetRT)
			setRenderTargetImage(BackBuffer);
	}
}


//! returns the current render target size
const core::dimension2d<u32>& CBurningVideoDriver::getCurrentRenderTargetSize() const
{
	return RenderTargetSize;
}




//! Draws a 3d line.
void CBurningVideoDriver::draw3DLine(const core::vector3df& start,
	const core::vector3df& end, SColor color_start,SColor color_end)
{
	s4DVertex *v = CurrentOut.data;
	transform_calc(ETS_PROJ_MODEL_VIEW);
	Transformation[ETS_PROJ_MODEL_VIEW].transformVect ( &v[0].Pos.x, start );
	Transformation[ETS_PROJ_MODEL_VIEW].transformVect ( &v[2].Pos.x, end );

#if BURNING_MATERIAL_MAX_COLORS > 0
	v[0].Color[0].setA8R8G8B8 ( color_start.color );
	v[2].Color[0].setA8R8G8B8 ( color_end.color );
#endif

	u32 g;
	u32 vOut;

	// no clipping flags
	for ( g = 0; g != CurrentOut.ElementSize; ++g )
	{
		v[g].flag = VERTEX4D_FORMAT_COLOR_1 | 0;
		Temp.data[g].flag = VERTEX4D_FORMAT_COLOR_1 | 0;
	}

	// vertices count per line
	vOut = clipToFrustum ( v, Temp.data, 2 );
	if ( vOut < 2 )
		return;

	vOut <<= 1;

	// to DC Space, project homogenous vertex
	ndc_2_dc_and_project ( v + 1, v, vOut );

	// unproject vertex color
#if 0
#if BURNING_MATERIAL_MAX_COLORS > 0
	for ( g = 0; g != vOut; g+= 2 )
	{
		v[ g + 1].Color[0].setA8R8G8B8 ( color.color );
	}
#endif
#endif

	IBurningShader * shader = 0;
	if ( CurrentShader && CurrentShader->canWireFrame() ) shader = CurrentShader;
	else shader = BurningShader [ ETR_TEXTURE_GOURAUD_WIRE ];
	shader = BurningShader [ ETR_TEXTURE_GOURAUD_WIRE ];

	shader->pushEdgeTest(1,1);
	shader->setRenderTarget(RenderTargetSurface, ViewPort);

	for ( g = 0; g <= vOut - 4; g += 2 )
	{
		shader->drawLine ( v + 1 + g, v + 1 + g + 2 );
	}

	shader->popEdgeTest();

}


//! \return Returns the name of the video driver. Example: In case of the DirectX8
//! driver, it would return "Direct3D8.1".
const wchar_t* CBurningVideoDriver::getName() const
{
#ifdef BURNINGVIDEO_RENDERER_BEAUTIFUL
	return L"Burning's Video 0.50 beautiful";
#elif defined ( BURNINGVIDEO_RENDERER_ULTRA_FAST )
	return L"Burning's Video 0.50 ultra fast";
#elif defined ( BURNINGVIDEO_RENDERER_FAST )
	return L"Burning's Video 0.50 fast";
#elif defined ( BURNINGVIDEO_RENDERER_CE )
	return L"Burning's Video 0.50 CE";
#else
	return L"Burning's Video 0.50";
#endif
}

//! Returns the graphics card vendor name.
core::stringc CBurningVideoDriver::getVendorInfo()
{
	return "Burning's Video: Ing. Thomas Alten (c) 2006-2019";
}


//! Returns type of video driver
E_DRIVER_TYPE CBurningVideoDriver::getDriverType() const
{
	return EDT_BURNINGSVIDEO;
}


//! returns color format
ECOLOR_FORMAT CBurningVideoDriver::getColorFormat() const
{
	return BURNINGSHADER_COLOR_FORMAT;
}


//! Returns the transformation set by setTransform
const core::matrix4& CBurningVideoDriver::getTransform(E_TRANSFORMATION_STATE state) const
{
	return Transformation[state];
}


//! Creates a render target texture.
ITexture* CBurningVideoDriver::addRenderTargetTexture(const core::dimension2d<u32>& size,
		const io::path& name, const ECOLOR_FORMAT format)
{
	IImage* img = createImage(BURNINGSHADER_COLOR_FORMAT, size);
	ITexture* tex = new CSoftwareTexture2(img, name, CSoftwareTexture2::IS_RENDERTARGET );
	img->drop();
	addTexture(tex);
	tex->drop();
	return tex;
}

void CBurningVideoDriver::clearBuffers(u32 flag, SColor color, f32 depth, u32 stencil)
{
	if ((flag & ECBF_COLOR) && RenderTargetSurface)
		RenderTargetSurface->fill(color);

	if ((flag & ECBF_DEPTH) && DepthBuffer)
		DepthBuffer->clear();

	if ((flag & ECBF_STENCIL) && StencilBuffer)
		StencilBuffer->clear();
}


//! Returns an image created from the last rendered frame.
IImage* CBurningVideoDriver::createScreenShot(video::ECOLOR_FORMAT format, video::E_RENDER_TARGET target)
{
	if (target != video::ERT_FRAME_BUFFER)
		return 0;

	if (BackBuffer)
	{
		IImage* tmp = createImage(BackBuffer->getColorFormat(), BackBuffer->getDimension());
		BackBuffer->copyTo(tmp);
		return tmp;
	}
	else
		return 0;
}

ITexture* CBurningVideoDriver::createDeviceDependentTexture(const io::path& name, IImage* image)
{
	CSoftwareTexture2* texture = new CSoftwareTexture2(image, name,
		(getTextureCreationFlag(ETCF_CREATE_MIP_MAPS) ? CSoftwareTexture2::GEN_MIPMAP : 0) |
		(getTextureCreationFlag(ETCF_ALLOW_NON_POWER_2) ? 0 : CSoftwareTexture2::NP2_SIZE) |
		(getTextureCreationFlag(ETCF_IMAGE_IS_LINEAR) ? CSoftwareTexture2::IMAGE_IS_LINEAR : 0) |
		(getTextureCreationFlag(ETCF_TEXTURE_IS_LINEAR) ? CSoftwareTexture2::TEXTURE_IS_LINEAR : 0)
		);

	return texture;
}


//! Returns the maximum amount of primitives (mostly vertices) which
//! the device is able to render with one drawIndexedTriangleList
//! call.
u32 CBurningVideoDriver::getMaximalPrimitiveCount() const
{
	return 0xFFFFFFFF;
}


//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
//! this: First, draw all geometry. Then use this method, to draw the shadow
//! volume. Next use IVideoDriver::drawStencilShadow() to visualize the shadow.
void CBurningVideoDriver::drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible)
{
	const u32 count = triangles.size();
	IBurningShader *shader = BurningShader [ ETR_STENCIL_SHADOW ];

	CurrentShader = shader;
	shader->setRenderTarget(RenderTargetSurface, ViewPort);

	Material.org.MaterialType = video::EMT_SOLID;
	Material.org.Lighting = false;
	Material.org.ZWriteEnable = false;
	Material.org.ZBuffer = ECFN_LESSEQUAL;
	EyeSpace.Flags &= ~TEXTURE_TRANSFORM;
	LightSpace.Flags &= ~TEXTURE_TRANSFORM;

	//glStencilMask(~0);
	//glStencilFunc(GL_ALWAYS, 0, ~0);

	if (true)// zpass does not work yet
	{
		Material.org.BackfaceCulling = true;
		Material.org.FrontfaceCulling = false;
		Material.Culling = CULL_BACK | CULL_INVISIBLE;

		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 1 );
		shader->setParam ( 2, 0 );
		drawVertexPrimitiveList (triangles.const_pointer(), count, 0, count/3, (video::E_VERTEX_TYPE) 4, scene::EPT_TRIANGLES, (video::E_INDEX_TYPE) 4 );
		//glStencilOp(GL_KEEP, incr, GL_KEEP);
		//glDrawArrays(GL_TRIANGLES,0,count);

		Material.org.BackfaceCulling = false;
		Material.org.FrontfaceCulling = true;
		Material.Culling = CULL_FRONT | CULL_INVISIBLE;

		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 2 );
		shader->setParam ( 2, 0 );
		drawVertexPrimitiveList (triangles.const_pointer(), count, 0, count/3, (video::E_VERTEX_TYPE) 4, scene::EPT_TRIANGLES, (video::E_INDEX_TYPE) 4 );
		//glStencilOp(GL_KEEP, decr, GL_KEEP);
		//glDrawArrays(GL_TRIANGLES,0,count);
	}
	else // zpass
	{
		Material.org.BackfaceCulling = true;
		Material.org.FrontfaceCulling = false;
		Material.Culling = CULL_BACK | CULL_INVISIBLE;

		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 0 );
		shader->setParam ( 2, 1 );
		//glStencilOp(GL_KEEP, GL_KEEP, incr);
		//glDrawArrays(GL_TRIANGLES,0,count);

		Material.org.BackfaceCulling = false;
		Material.org.FrontfaceCulling = true;
		Material.Culling = CULL_FRONT | CULL_INVISIBLE;

		shader->setParam ( 0, 0 );
		shader->setParam ( 1, 0 );
		shader->setParam ( 2, 2 );
		//glStencilOp(GL_KEEP, GL_KEEP, decr);
		//glDrawArrays(GL_TRIANGLES,0,count);
	}
}

//! Fills the stencil shadow with color. After the shadow volume has been drawn
//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
//! to draw the color of the shadow.
void CBurningVideoDriver::drawStencilShadow(bool clearStencilBuffer, video::SColor leftUpEdge,
	video::SColor rightUpEdge, video::SColor leftDownEdge, video::SColor rightDownEdge)
{
	if (!StencilBuffer)
		return;
	// draw a shadow rectangle covering the entire screen using stencil buffer
	const u32 h = RenderTargetSurface->getDimension().Height;
	const u32 w = RenderTargetSurface->getDimension().Width;
	tVideoSample *dst;
	u32 *stencil;
	u32* const stencilBase=(u32*) StencilBuffer->lock();

#if defined(SOFTWARE_DRIVER_2_32BIT)
#else
	const u16 alpha = extractAlpha( leftUpEdge.color ) >> 3;
	const u32 src = video::A8R8G8B8toA1R5G5B5( leftUpEdge.color );

#endif
	for ( u32 y = 0; y < h; ++y )
	{
		dst = (tVideoSample*)RenderTargetSurface->getData() + ( y * w );
		stencil =  stencilBase + ( y * w );

		for ( u32 x = 0; x < w; ++x )
		{
			if ( stencil[x] > 0 )
			{
#if defined(SOFTWARE_DRIVER_2_32BIT)
				dst[x] = PixelBlend32 ( dst[x], leftUpEdge.color );
#else
				dst[x] = PixelBlend16( dst[x], src, alpha );
#endif
			}
		}
	}

	StencilBuffer->clear();
}


core::dimension2du CBurningVideoDriver::getMaxTextureSize() const
{
	return core::dimension2du(SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE ? SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE : 1 << 20,
		SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE ? SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE : 1 << 20);
}

bool CBurningVideoDriver::queryTextureFormat(ECOLOR_FORMAT format) const
{
	return format == BURNINGSHADER_COLOR_FORMAT;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_


#if defined(_IRR_WINDOWS_) && defined(_IRR_COMPILE_WITH_BURNINGSVIDEO_) && 0
	#include <windows.h>

struct dreadglobal
{
	DWORD dreadid;
	HANDLE dread;
	irr::video::CBurningVideoDriver *driver;
	HANDLE sync;

	const irr::SIrrlichtCreationParameters* params;
	irr::io::IFileSystem* io;
	irr::video::IImagePresenter* presenter;
};
dreadglobal b;

DWORD WINAPI dreadFun( void *p)
{
    printf("Hi This is burning dread\n");
	b.driver = new irr::video::CBurningVideoDriver(*b.params, b.io, b.presenter);

	SetEvent ( b.sync );
	while ( 1 )
	{
		Sleep ( 1000 );
	}
	return 0;
}

#endif

namespace irr
{
namespace video
{

//! creates a video driver
IVideoDriver* createBurningVideoDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, video::IImagePresenter* presenter)
{
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

	#if defined(_IRR_WINDOWS_) && 0
		b.sync = CreateEventA ( 0, 0, 0, "burnevent0" );
		b.params = &params;
		b.io = io;
		b.presenter = presenter;
		b.dread = CreateThread ( 0, 0, dreadFun, 0, 0, &b.dreadid );
		WaitForSingleObject ( b.sync, INFINITE );
		return b.driver;
	#else
		return new CBurningVideoDriver(params, io, presenter);
	#endif

	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}



} // end namespace video
} // end namespace irr


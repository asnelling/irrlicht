// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_compile_config.h"
#include "IBurningShader.h"
#include "CSoftwareDriver2.h"
#include "IShaderConstantSetCallBack.h"

namespace irr
{
namespace video
{

const tFixPointu IBurningShader::dithermask[] =
{
	0x00,0x80,0x20,0xa0,
	0xc0,0x40,0xe0,0x60,
	0x30,0xb0,0x10,0x90,
	0xf0,0x70,0xd0,0x50
};

IBurningShader::IBurningShader(CBurningVideoDriver* driver)
{
	#ifdef _DEBUG
	setDebugName("IBurningShader");
	#endif

	EdgeTestPass = edge_test_pass;
	EdgeTestPass_stack = edge_test_pass;

	for ( u32 i = 0; i != BURNING_MATERIAL_MAX_TEXTURES; ++i )
	{
		IT[i].Texture = 0;
	}

	Driver = driver;
	CallBack = 0;

	RenderTarget = 0;
	ColorMask = COLOR_BRIGHT_WHITE;
	DepthBuffer = (CDepthBuffer*) driver->getDepthBuffer ();
	if ( DepthBuffer )
		DepthBuffer->grab();

	Stencil = (CStencilBuffer*) driver->getStencilBuffer ();
	if ( Stencil )
		Stencil->grab();

	stencilOp[0] = StencilOp_KEEP;
	stencilOp[1] = StencilOp_KEEP;
	stencilOp[2] = StencilOp_KEEP;
	AlphaRef = 0;
}

//! Constructor
IBurningShader::IBurningShader(
	CBurningVideoDriver* driver,
	s32& outMaterialTypeNr,
	const c8* vertexShaderProgram,
	const c8* vertexShaderEntryPointName,
	E_VERTEX_SHADER_TYPE vsCompileTarget,
	const c8* pixelShaderProgram,
	const c8* pixelShaderEntryPointName,
	E_PIXEL_SHADER_TYPE psCompileTarget,
	const c8* geometryShaderProgram ,
	const c8* geometryShaderEntryPointName,
	E_GEOMETRY_SHADER_TYPE gsCompileTarget,
	scene::E_PRIMITIVE_TYPE inType,
	scene::E_PRIMITIVE_TYPE outType,
	u32 verticesOut,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial,
	s32 userData)
:IBurningShader(driver)
{
	BaseMaterial = baseMaterial;
	CallBack = callback;
	if (CallBack)
		CallBack->grab();

	// register myself as new material
	outMaterialTypeNr = Driver->addMaterialRenderer(this);
}


//! destructor
IBurningShader::~IBurningShader()
{
	if (RenderTarget)
		RenderTarget->drop();

	if (DepthBuffer)
		DepthBuffer->drop();

	if (Stencil)
		Stencil->drop();

	for ( u32 i = 0; i != BURNING_MATERIAL_MAX_TEXTURES; ++i )
	{
		if ( IT[i].Texture )
			IT[i].Texture->drop();
	}

	if (CallBack)
		CallBack->drop();

}

//! sets a render target
void IBurningShader::setRenderTarget(video::IImage* surface, const core::rect<s32>& viewPort)
{
	if (RenderTarget)
		RenderTarget->drop();

	RenderTarget = (video::CImage* ) surface;

	if (RenderTarget)
	{
		RenderTarget->grab();

		//(fp24*) DepthBuffer->lock() = DepthBuffer->lock();
	}
}


//! sets the Texture
void IBurningShader::setTextureParam( const size_t stage, video::CSoftwareTexture2* texture, s32 lodFactor)
{
	sInternalTexture *it = &IT[stage];

	if ( it->Texture)
		it->Texture->drop();

	it->Texture = texture;

	if ( it->Texture)
	{
		it->Texture->grab();

		// select mignify and magnify
		it->lodFactor = lodFactor;

		//only mipmap chain (means positive lodFactor)
		u32 existing_level = it->Texture->getMipmapLevel(lodFactor);
		it->data = (tVideoSample*) it->Texture->lock(ETLM_READ_ONLY, existing_level, 0);

		// prepare for optimal fixpoint
		it->pitchlog2 = s32_log2_s32 ( it->Texture->getPitch() );

		const core::dimension2d<u32> &dim = it->Texture->getSize();
		it->textureXMask = s32_to_fixPoint ( dim.Width - 1 ) & FIX_POINT_UNSIGNED_MASK;
		it->textureYMask = s32_to_fixPoint ( dim.Height - 1 ) & FIX_POINT_UNSIGNED_MASK;
	}
}

//emulate a line with degenerate triangle and special shader mode (not perfect...)
void IBurningShader::drawLine ( const s4DVertex *a,const s4DVertex *b)
{
	sVec2 d;
	d.x = b->Pos.x - a->Pos.x;	d.x *= d.x;
	d.y = b->Pos.y - a->Pos.y;	d.y *= d.y;
	//if ( d.x * d.y < 0.001f ) return;

	if ( a->Pos.x > b->Pos.x ) swapVertexPointer(&a, &b);

	s4DVertex c = *a;

	const f32 w = (f32)RenderTarget->getDimension().Width-1;
	const f32 h = (f32)RenderTarget->getDimension().Height-1;

	if ( d.x < 2.f ) { c.Pos.x = b->Pos.x + 1.f + d.y; if ( c.Pos.x > w ) c.Pos.x = w; }
	else c.Pos.x = b->Pos.x;
	if ( d.y < 2.f ) { c.Pos.y = b->Pos.y + 1.f; if ( c.Pos.y > h ) c.Pos.y = h; EdgeTestPass |= edge_test_first_line; }

	drawTriangle ( a,b,&c );
	EdgeTestPass &= ~edge_test_first_line;

}

void IBurningShader::drawPoint(const s4DVertex *a)
{
}

void IBurningShader::drawWireFrameTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c )
{
	if ( EdgeTestPass & edge_test_pass ) drawTriangle(a, b, c);
	else if (EdgeTestPass & edge_test_point)
	{
		drawPoint(a);
		drawPoint(b);
		drawPoint(c);
	}
	else
	{
		drawLine(a, b);
		drawLine(b, c);
		drawLine(a, c);
	}
}


void IBurningShader::OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
	bool resetAllRenderstates, IMaterialRendererServices* services)
{
	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
	if (CallBack)
		CallBack->OnSetMaterial(material);

}

bool IBurningShader::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	// call callback to set shader constants
	if (CallBack)
		CallBack->OnSetConstants(this, UserData);
	return true;
}

void IBurningShader::OnUnsetMaterial()
{
}

//! Returns if the material is transparent.
bool IBurningShader::isTransparent() const
{
	return false;
}

//! Access the callback provided by the users when creating shader materials
IShaderConstantSetCallBack* IBurningShader::getShaderConstantSetCallBack() const
{
	return CallBack;
}

// implementations for the render services
void IBurningShader::setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates)
{
	// forward
	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

s32 IBurningShader::getVertexShaderConstantID(const c8* name)
{
	return getPixelShaderConstantID(name);
}

s32 IBurningShader::getPixelShaderConstantID(const c8* name)
{
	for (u32 i = 0; i < UniformInfo.size(); ++i)
	{
		if (UniformInfo[i].name == name)
			return i;
	}

	return -1;
}

void IBurningShader::setVertexShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	os::Printer::log("Cannot set constant, please use high level shader call instead.", ELL_WARNING);
}

void IBurningShader::setPixelShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	os::Printer::log("Cannot set constant, use high level shader call.", ELL_WARNING);
}

bool IBurningShader::setVertexShaderConstant(s32 index, const f32* floats, int count)
{
	return setPixelShaderConstant(index, floats, count);
}

bool IBurningShader::setVertexShaderConstant(s32 index, const s32* ints, int count)
{
	return setPixelShaderConstant(index, ints, count);
}

bool IBurningShader::setPixelShaderConstant(s32 index, const f32* floats, int count)
{
	return false;
}

bool IBurningShader::setPixelShaderConstant(s32 index, const s32* ints, int count)
{
	return false;
}

IVideoDriver* IBurningShader::getVideoDriver()
{
	return Driver;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

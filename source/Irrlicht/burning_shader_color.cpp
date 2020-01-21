// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "IBurningShader.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_


namespace irr
{

namespace video
{

class burning_shader_color : public IBurningShader
{
public:

	//! constructor
	burning_shader_color(CBurningVideoDriver* driver);

	//! draws an indexed triangle list
	virtual void drawTriangle(const s4DVertex* burning_restrict a, const s4DVertex* burning_restrict b, const s4DVertex* burning_restrict c) _IRR_OVERRIDE_;
	virtual bool canWireFrame () { return true; }

	virtual void OnSetMaterial(const SBurningShaderMaterial& material) _IRR_OVERRIDE_;

	//! Returns if the material is transparent.
	virtual bool isTransparent() const _IRR_OVERRIDE_;

private:

	// fragment shader
	typedef void (burning_shader_color::*tFragmentShader) ();
	void fragment_depth_less_equal_depth_write();
	void fragment_depth_less_equal_no_depth_write();
	void fragment_nodepth_perspective();
	void fragment_nodepth_noperspective(); // 2D Gradient

	tFragmentShader fragmentShader;

	sScanConvertData scan;
	sScanLineData line;

};

//! constructor
burning_shader_color::burning_shader_color(CBurningVideoDriver* driver)
: IBurningShader(driver)
{
	#ifdef _DEBUG
	setDebugName("burning_shader_color");
	#endif
	fragmentShader = &burning_shader_color::fragment_nodepth_perspective;

}

//! Returns if the material is transparent.
bool burning_shader_color::isTransparent() const
{
	return false;
}


/*!
*/
void burning_shader_color::OnSetMaterial(const SBurningShaderMaterial& material)
{
	if (material.org.ZBuffer == ECFN_LESSEQUAL)
	{
		if (material.depth_write) fragmentShader = &burning_shader_color::fragment_depth_less_equal_depth_write;
		else fragmentShader = &burning_shader_color::fragment_depth_less_equal_no_depth_write;
	}
	else /*if (material.org.ZBuffer == ECFN_DISABLED)*/
	{
		//check triangle on w = 1.f instead..
#ifdef	SOFTWARE_DRIVER_2_BILINEAR
		if (material.org.TextureLayer[0].BilinearFilter) fragmentShader = &burning_shader_color::fragment_nodepth_perspective;
		else
#endif
			fragmentShader = &burning_shader_color::fragment_nodepth_noperspective;
	}
}


#define burning_shader_class burning_shader_color
#define burning_shader_frag

// compile flag for this triangle
#include "burning_shader_compile_start.h"
#define SUBTEXEL
#define IPOL_W
#define IPOL_C0
#include "burning_shader_compile_triangle.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_nodepth_noperspective
#define SUBTEXEL
#define IPOL_C0
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_c.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_nodepth_perspective
#define SUBTEXEL
#define INVERSE_W
#define IPOL_W
#define IPOL_C0
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_c.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_depth_less_equal_no_depth_write
#define SUBTEXEL
#define INVERSE_W
#define IPOL_W
#define IPOL_C0
#define USE_ZBUFFER
#define CMP_W
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_c.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_depth_less_equal_depth_write
#define SUBTEXEL
#define INVERSE_W
#define IPOL_W
#define IPOL_C0
#define USE_ZBUFFER
#define CMP_W
#define WRITE_W
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_c.h"


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

namespace irr
{
namespace video
{

IBurningShader* create_burning_shader_color(CBurningVideoDriver* driver)
{
	//ETR_GOURAUD_NOZ - no texture no depth test no depth write
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
	return new burning_shader_color(driver);
	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}


} // end namespace video
} // end namespace irr


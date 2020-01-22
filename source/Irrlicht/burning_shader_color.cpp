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

#define burning_shader_class burning_shader_color
#define burning_shader_frag "burning_shader_color_fraq.h"
#include "burning_shader_compile_fragment_default.h"


/*!
*/
void burning_shader_class::OnSetMaterial(const SBurningShaderMaterial& material)
{
	AlphaRef = tofix(material.org.MaterialTypeParam, FIXPOINT_COLOR_MAX);

	if (material.org.ZBuffer == ECFN_LESSEQUAL)
	{
		if (material.depth_write) fragmentShader = &burning_shader_class::fragment_depth_less_equal_depth_write;
		else fragmentShader = &burning_shader_class::fragment_depth_less_equal_no_depth_write;
	}
	else /*if (material.org.ZBuffer == ECFN_DISABLED)*/
	{
		//check triangle on w = 1.f instead..
#ifdef	SOFTWARE_DRIVER_2_BILINEAR
		if (material.org.TextureLayer[0].BilinearFilter) fragmentShader = &burning_shader_class::fragment_nodepth_perspective;
		else
#endif
			fragmentShader = &burning_shader_class::fragment_nodepth_noperspective;
	}
}



} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_




class burning_shader_class : public IBurningShader
{
public:

	//! constructor
	burning_shader_class(CBurningVideoDriver* driver);

	//! draws an indexed triangle list
	virtual void drawTriangle(const s4DVertex* burning_restrict a, const s4DVertex* burning_restrict b, const s4DVertex* burning_restrict c) _IRR_OVERRIDE_;
	virtual bool canWireFrame() { return true; }

	virtual void OnSetMaterial(const SBurningShaderMaterial& material) _IRR_OVERRIDE_;

	//! Returns if the material is transparent.
	virtual bool isTransparent() const _IRR_OVERRIDE_;

private:

	// fragment shader
	typedef void (burning_shader_class::*tFragmentShader) ();
	void fragment_depth_less_equal_depth_write();
	void fragment_depth_less_equal_no_depth_write();
	void fragment_nodepth_perspective();
	void fragment_nodepth_noperspective(); // 2D Gradient

	tFragmentShader fragmentShader;

};


//! constructor
burning_shader_class::burning_shader_color(CBurningVideoDriver* driver)
	: IBurningShader(driver)
{
#ifdef _DEBUG
	setDebugName(burning_stringify(burning_shader_class) );
#endif

	fragmentShader = &burning_shader_class::fragment_nodepth_perspective;
}

//! Returns if the material is transparent.
bool burning_shader_class::isTransparent() const
{
	return false;
}

IBurningShader* burning_create(burning_shader_class)(CBurningVideoDriver* driver)
{
	return new burning_shader_class(driver);
}



// compile flag for this triangle
#include "burning_shader_compile_start.h"
#define SUBTEXEL
#define IPOL_W
#define IPOL_C0
#include "burning_shader_compile_triangle.h"

// compile flag for this scanline fragment
#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_nodepth_noperspective
#define SUBTEXEL
#define IPOL_C0
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_start.h"
#include burning_shader_frag
#include "burning_shader_compile_fragment_end.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_nodepth_perspective
#define SUBTEXEL
#define INVERSE_W
#define IPOL_W
#define IPOL_C0
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_start.h"
#include burning_shader_frag
#include "burning_shader_compile_fragment_end.h"

#include "burning_shader_compile_start.h"
#define burning_shader_fragment fragment_depth_less_equal_no_depth_write
#define SUBTEXEL
#define INVERSE_W
#define IPOL_W
#define IPOL_C0
#define USE_ZBUFFER
#define CMP_W
#define INVERSE_W_RANGE FIX_POINT_F32_MUL*COLOR_MAX
#include "burning_shader_compile_fragment_start.h"
#include burning_shader_frag
#include "burning_shader_compile_fragment_end.h"

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
#include "burning_shader_compile_fragment_start.h"
#include burning_shader_frag
#include "burning_shader_compile_fragment_end.h"

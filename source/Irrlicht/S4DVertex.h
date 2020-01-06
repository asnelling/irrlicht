// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h


#ifndef __S_4D_VERTEX_H_INCLUDED__
#define __S_4D_VERTEX_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"
#include "SoftwareDriver2_helper.h"
#include "irrAllocator.h"
#include "EPrimitiveTypes.h"

namespace irr
{

namespace video
{

//! sVec2 used in BurningShader texture coordinates
struct sVec2
{
	f32 x;
	f32 y;

	sVec2 () {}

	sVec2 ( f32 s) : x ( s ), y ( s ) {}
	sVec2 ( f32 _x, f32 _y )
		: x ( _x ), y ( _y ) {}

	void set ( f32 _x, f32 _y )
	{
		x = _x;
		y = _y;
	}

	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec2& a, const sVec2& b, const f32 t)
	{
		x = b.x + ( ( a.x - b.x ) * t );
		y = b.y + ( ( a.y - b.y ) * t );
	}

	sVec2 operator-(const sVec2& other) const
	{
		return sVec2(x - other.x, y - other.y);
	}

	sVec2 operator+(const sVec2& other) const
	{
		return sVec2(x + other.x, y + other.y);
	}

	void operator+=(const sVec2& other)
	{
		x += other.x;
		y += other.y;
	}

	sVec2 operator*(const f32 s) const
	{
		return sVec2(x * s , y * s);
	}

	void operator*=( const f32 s)
	{
		x *= s;
		y *= s;
	}

	void operator=(const sVec2& other)
	{
		x = other.x;
		y = other.y;
	}

};

#include "irrpack.h"

//! sVec3Pack used in BurningShader, packed direction
struct sVec3Pack
{
	f32 x, y, z;

	sVec3Pack() {}
	sVec3Pack(f32 _x, f32 _y, f32 _z)
		: x(_x), y(_y), z(_z) {}

	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec3Pack& v0, const sVec3Pack& v1, const f32 t)
	{
		x = v1.x + ((v0.x - v1.x) * t);
		y = v1.y + ((v0.y - v1.y) * t);
		z = v1.z + ((v0.z - v1.z) * t);
	}

	sVec3Pack operator-(const sVec3Pack& other) const
	{
		return sVec3Pack(x - other.x, y - other.y, z - other.z);
	}

	sVec3Pack operator+(const sVec3Pack& other) const
	{
		return sVec3Pack(x + other.x, y + other.y, z + other.z);
	}

	sVec3Pack operator*(const f32 s) const
	{
		return sVec3Pack(x * s, y * s, z * s);
	}

	void operator+=(const sVec3Pack& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
	}

	void operator=(const sVec3Pack& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
	}

	void normalize_pack_xyz(const f32 len, const f32 ofs)
	{
		//const f32 l = len * core::reciprocal_squareroot ( r * r + g * g + b * b );
		f32 l = x * x + y * y + z * z;

		l = l > 0.0000001f ? len / sqrtf(l) : 0.f;
		x = (x*l) + ofs;
		y = (y*l) + ofs;
		z = (z*l) + ofs;
	}
}  PACK_STRUCT;

#include "irrunpack.h"

//! sVec4 used in Driver,BurningShader, direction/color
struct sVec4
{
	union
	{
		struct { f32 x, y, z, w; };
		struct { f32 a, r, g, b; };
	};

	sVec4 () {}
	sVec4 ( f32 _x, f32 _y, f32 _z, f32 _w )
		: x ( _x ), y ( _y ), z( _z ), w ( _w ){}

	sVec4(const sVec3Pack& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
	}

	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec4& a, const sVec4& b, const f32 t)
	{
		x = b.x + ( ( a.x - b.x ) * t );
		y = b.y + ( ( a.y - b.y ) * t );
		z = b.z + ( ( a.z - b.z ) * t );
		w = b.w + ( ( a.w - b.w ) * t );
	}

	sVec4 operator-(const sVec4& other) const
	{
		return sVec4(x - other.x, y - other.y, z - other.z,w - other.w);
	}

	sVec4 operator+(const sVec4& other) const
	{
		return sVec4(x + other.x, y + other.y, z + other.z,w + other.w);
	}

	void operator+=(const sVec4& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
	}

	sVec4 operator*(const f32 s) const
	{
		return sVec4(x * s , y * s, z * s,w * s);
	}

	sVec4 operator*(const sVec4 &other) const
	{
		return sVec4(x * other.x , y * other.y, z * other.z,w * other.w);
	}

	void operator*=(const sVec4 &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;
	}

	void operator=(const sVec4& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}

	//outside shader
	void set(f32 _x, f32 _y, f32 _z, f32 _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
	void setA8R8G8B8(const u32 argb)
	{
		a = ((argb & 0xFF000000) >> 24) * (1.f / 255.f);
		r = ((argb & 0x00FF0000) >> 16) * (1.f / 255.f);
		g = ((argb & 0x0000FF00) >> 8 ) * (1.f / 255.f);
		b = ((argb & 0x000000FF)      ) * (1.f / 255.f);
	}

	REALINLINE f32 dot(const sVec4& other) const
	{
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	REALINLINE f32 dot_xyz(const sVec4& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	REALINLINE f32 dot_minus_xyz(const sVec4& other) const
	{
		return -x * other.x + -y * other.y + -z * other.z;
	}

	void mul_xyz(const f32 s)
	{
		x *= s;
		y *= s;
		z *= s;
	}

	f32 length_xyz() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	void normalize_dir_xyz()
	{
		//const f32 l = core::reciprocal_squareroot(x * x + y * y + z * z);
		f32 l = x * x + y * y + z * z;
		l = l > 0.0000001f ? 1.f / sqrtf(l) : 1.f;
		x *= l;
		y *= l;
		z *= l;

	}

};


//!sVec4 is argb. sVec3Color is rgba
struct sVec3Color
{
	f32 r, g, b,a;

	void set(const f32 s)
	{
		r = s;
		g = s;
		b = s;
		a = s;
	}

	void setA8R8G8B8(const u32 argb)
	{
		r = ((argb & 0x00FF0000) >> 16) * (1.f / 255.f);
		g = ((argb & 0x0000FF00) >> 8 ) * (1.f / 255.f);
		b = ((argb & 0x000000FF)      ) * (1.f / 255.f);
		a = ((argb & 0xFF000000) >> 24) * (1.f / 255.f);
	}

	void setColorf(const video::SColorf & color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
	}

	void add_rgb(const sVec3Color& other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
	}

	void mad_rgb(const sVec3Color& other, const f32 v)
	{
		r += other.r * v;
		g += other.g * v;
		b += other.b * v;
	}

	void mad_rgbv(const sVec3Color& v0, const sVec3Color& v1)
	{
		r += v0.r * v1.r;
		g += v0.g * v1.g;
		b += v0.b * v1.b;
	}

	//sVec4 is a,r,g,b
	void sat(sVec4 &dest, const u32 argb) const
	{
		dest.a = ((argb & 0xFF000000) >> 24) * (1.f / 255.f);
		dest.r = r <= 1.f ? r : 1.f;
		dest.g = g <= 1.f ? g : 1.f;
		dest.b = b <= 1.f ? b : 1.f;
	}

};

//internal BurningShaderFlag for a Vertex
enum e4DVertexFlag
{
	VERTEX4D_CLIPMASK				= 0x0000003F,
	VERTEX4D_CLIP_INSIDE_NEAR		= 0x00000001,
	VERTEX4D_CLIP_INSIDE_FAR		= 0x00000002,
	VERTEX4D_CLIP_INSIDE_LEFT		= 0x00000004,
	VERTEX4D_CLIP_INSIDE_RIGHT		= 0x00000008,
	VERTEX4D_CLIP_INSIDE_BOTTOM		= 0x00000010,
	VERTEX4D_CLIP_INSIDE_TOP		= 0x00000020,
	VERTEX4D_INSIDE					= 0x0000003F,

	VERTEX4D_PROJECTED				= 0x00000100,
	VERTEX4D_VAL_ZERO				= 0x00000200,
	VERTEX4D_VAL_ONE				= 0x00000400,

	VERTEX4D_FORMAT_MASK			= 0xFFFF0000,

	VERTEX4D_FORMAT_MASK_TEXTURE	= 0x000F0000,
	VERTEX4D_FORMAT_TEXTURE_1		= 0x00010000,
	VERTEX4D_FORMAT_TEXTURE_2		= 0x00020000,
	VERTEX4D_FORMAT_TEXTURE_3		= 0x00030000,
	VERTEX4D_FORMAT_TEXTURE_4		= 0x00040000,

	VERTEX4D_FORMAT_MASK_COLOR		= 0x00F00000,
	VERTEX4D_FORMAT_COLOR_1			= 0x00100000,
	VERTEX4D_FORMAT_COLOR_2			= 0x00200000,

	VERTEX4D_FORMAT_MASK_TANGENT	= 0x0F000000,
	VERTEX4D_FORMAT_BUMP_DOT3		= 0x01000000,

};

//! vertex layout
enum e4DVertexType
{
	E4VT_STANDARD = 0,			// EVT_STANDARD, video::S3DVertex.
	E4VT_2TCOORDS = 1,			// EVT_2TCOORDS, video::S3DVertex2TCoords.
	E4VT_TANGENTS = 2,			// EVT_TANGENTS, video::S3DVertexTangents
	E4VT_REFLECTION_MAP = 3,
	E4VT_SHADOW = 4				// float * 3
};

enum e4DIndexType
{
	E4IT_16BIT = 1, // EIT_16BIT,
	E4IT_32BIT = 2, // EIT_32BIT,
	E4IT_NONE  = 4, //
};


#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	#ifdef SOFTWARE_DRIVER_2_USE_SEPARATE_SPECULAR_COLOR
		#define BURNING_MATERIAL_MAX_COLORS 2
	#else
		#define BURNING_MATERIAL_MAX_COLORS 1
	#endif
#else
	#define BURNING_MATERIAL_MAX_COLORS 0
#endif

#define BURNING_MATERIAL_MAX_TEXTURES 2
#ifdef BURNINGVIDEO_RENDERER_BEAUTIFUL
	#define BURNING_MATERIAL_MAX_TANGENT 1
#else
	#define BURNING_MATERIAL_MAX_TANGENT 0
#endif

// dummy Vertex. used for calculation vertex memory size
struct s4DVertex_proxy
{
	u32 flag; // e4DVertexFlag
	sVec4 Pos;
#if BURNING_MATERIAL_MAX_TEXTURES > 0
	sVec2 Tex[BURNING_MATERIAL_MAX_TEXTURES];
#endif
#if BURNING_MATERIAL_MAX_COLORS > 0
	sVec4 Color[BURNING_MATERIAL_MAX_COLORS];
#endif
#if BURNING_MATERIAL_MAX_TANGENT > 0
	sVec3Pack LightTangent[BURNING_MATERIAL_MAX_TANGENT];
#endif
};

//ensure handcrafted sizeof(s4DVertex)
#define sizeof_s4DVertex	64

/*!
	Internal BurningVideo Vertex
*/
struct s4DVertex
{
	u32 flag; // e4DVertexFlag

	sVec4 Pos;
#if BURNING_MATERIAL_MAX_TEXTURES > 0
	sVec2 Tex[ BURNING_MATERIAL_MAX_TEXTURES ];
#endif
#if BURNING_MATERIAL_MAX_COLORS > 0
	sVec4 Color[ BURNING_MATERIAL_MAX_COLORS ];
#endif
#if BURNING_MATERIAL_MAX_TANGENT > 0
	sVec3Pack LightTangent[BURNING_MATERIAL_MAX_TANGENT];
#endif

#if BURNING_MATERIAL_MAX_COLORS < 1 || BURNING_MATERIAL_MAX_TANGENT < 1
	u8 __align [sizeof_s4DVertex - sizeof (s4DVertex_proxy) ];
#endif

	// f = a * t + b * ( 1 - t )
	void interpolate(const s4DVertex& b, const s4DVertex& a, const f32 t)
	{
		Pos.interpolate ( a.Pos, b.Pos, t );

		size_t i;
		size_t size;

#if BURNING_MATERIAL_MAX_TEXTURES > 0
		size = (flag & VERTEX4D_FORMAT_MASK_TEXTURE) >> 16;
		for ( i = 0; i!= size; ++i )
		{
			Tex[i].interpolate ( a.Tex[i], b.Tex[i], t );
		}
#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
		size = (flag & VERTEX4D_FORMAT_MASK_COLOR) >> 20;
		for ( i = 0; i!= size; ++i )
		{
			Color[i].interpolate ( a.Color[i], b.Color[i], t );
		}
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
		size = (flag & VERTEX4D_FORMAT_MASK_TANGENT) >> 24;
		for ( i = 0; i!= size; ++i )
		{
			LightTangent[i].interpolate ( a.LightTangent[i], b.LightTangent[i], t );
		}
#endif
	}
};

// ----------------- Vertex Cache ---------------------------

// Buffer is used as pairs of S4DVertex (0 ... ndc, 1 .. dc and projected)
typedef s4DVertex s4DVertexPair;
#define sizeof_s4DVertexPairRel 2

struct SAligned4DVertex
{
	SAligned4DVertex()
		:data(0),mem(0), ElementSize(0)	{}

	virtual ~SAligned4DVertex ()
	{
		if (mem)
		{
			delete[] mem;
			mem = 0;
		}
	}

	void resize(size_t element)
	{
		if (element > ElementSize)
		{
			if (mem) delete[] mem;
			size_t byteSize = align_next(element * sizeof_s4DVertex, 4096);
			mem = new u8[byteSize];
		}
		ElementSize = element;
		data = (s4DVertex*)mem;
	}

	s4DVertex* data;	//align to 16 byte
	u8* mem;
	size_t ElementSize;
};

//#define memcpy_s4DVertexPair(dst,src) memcpy(dst,src,sizeof_s4DVertex * 2)
static inline void memcpy_s4DVertexPair(void* dst, const void *src)
{
	u32* dst32 = (u32*)dst;
	const u32* src32 = (const u32*)src;

	//test alignment
#if 0
	if (((size_t)dst & 0xC) | ((size_t)src & 0xC))
	{
		int g = 1;
	}
#endif
	size_t len = sizeof_s4DVertex * sizeof_s4DVertexPairRel;
	while (len >= 32)
	{
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		*dst32++ = *src32++;
		len -= 32;
	}
/*
	while (len >= 4)
	{
		*dst32++ = *src32++;
		len -= 4;
	}
*/
}


//! hold info for different Vertex Types
struct SVSize
{
	u32 Format;		// e4DVertexFlag VERTEX4D_FORMAT_MASK_TEXTURE
	u32 Pitch;		// sizeof Vertex
	size_t TexSize;	// amount Textures
	u32 TexCooSize;	// sizeof TextureCoordinates
};


// a cache info
struct SCacheInfo
{
	u32 index;
	u32 hit;
};

//must at least hold all possible vertices of primitive.
#define VERTEXCACHE_ELEMENT	16			
#define VERTEXCACHE_MISS 0xFFFFFFFF
struct SVertexCache
{
	SVertexCache () {}
	~SVertexCache() {}

	SCacheInfo info[VERTEXCACHE_ELEMENT];


	// Transformed and lite, clipping state
	// + Clipped, Projected
	SAligned4DVertex mem;

	// source
	const void* vertices;
	u32 vertexCount;

	const void* indices;
	u32 indexCount;
	u32 indicesIndex;
	u32 indicesRun;

	// primitives consist of x vertices
	u32 primitiveHasVertex;
	u32 primitivePitch;

	e4DVertexType vType;		//E_VERTEX_TYPE
	scene::E_PRIMITIVE_TYPE pType;		//scene::E_PRIMITIVE_TYPE
	e4DIndexType iType;		//E_INDEX_TYPE iType

};


// swap 2 pointer
REALINLINE void swapVertexPointer(const s4DVertex** v1, const s4DVertex** v2)
{
	const s4DVertex* b = *v1;
	*v1 = *v2;
	*v2 = b;
}


// ------------------------ Internal Scanline Rasterizer -----------------------------



// internal scan convert
struct sScanConvertData
{
	u32 left;			// major edge left/right
	u32 right;			// !left

	f32 invDeltaY[4];	// inverse edge delta y

	f32 x[2];			// x coordinate
	f32 slopeX[2];		// x slope along edges

#if defined ( SOFTWARE_DRIVER_2_USE_WBUFFER ) || defined ( SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT )
	f32 w[2];			// w coordinate
	fp24 slopeW[2];		// w slope along edges
#else
	f32 z[2];			// z coordinate
	f32 slopeZ[2];		// z slope along edges
#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
	sVec4 c[BURNING_MATERIAL_MAX_COLORS][2];		// color
	sVec4 slopeC[BURNING_MATERIAL_MAX_COLORS][2];	// color slope along edges
#endif

#if BURNING_MATERIAL_MAX_TEXTURES > 0
	sVec2 t[BURNING_MATERIAL_MAX_TEXTURES][2];		// texture
	sVec2 slopeT[BURNING_MATERIAL_MAX_TEXTURES][2];	// texture slope along edges
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
	sVec3Pack l[BURNING_MATERIAL_MAX_TANGENT][2];		// Light Tangent
	sVec3Pack slopeL[BURNING_MATERIAL_MAX_TEXTURES][2];	// tanget slope along edges
#endif
};

// passed to scan Line
struct sScanLineData
{
	s32 y;				// y position of scanline
	f32 x[2];			// x start, x end of scanline

#if defined ( SOFTWARE_DRIVER_2_USE_WBUFFER ) || defined ( SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT )
	f32 w[2];			// w start, w end of scanline
#else
	f32 z[2];			// z start, z end of scanline
#endif

#if BURNING_MATERIAL_MAX_COLORS > 0
	sVec4 c[BURNING_MATERIAL_MAX_COLORS][2];			// color start, color end of scanline
#endif

#if BURNING_MATERIAL_MAX_TEXTURES > 0
	sVec2 t[BURNING_MATERIAL_MAX_TEXTURES][2];		// texture start, texture end of scanline
#endif

#if BURNING_MATERIAL_MAX_TANGENT > 0
	sVec3Pack l[BURNING_MATERIAL_MAX_TANGENT][2];		// Light Tangent start, end
#endif
};

// passed to pixel Shader
struct sPixelShaderData
{
	tVideoSample *dst;
	fp24 *z;

	s32 xStart;
	s32 xEnd;
	s32 dx;
	s32 i;
};

/*
	load a color value
*/
REALINLINE void getTexel_plain2 (	tFixPoint &r, tFixPoint &g, tFixPoint &b,
							const sVec4 &v
							)
{
	r = tofix(v.r, FIX_POINT_F32_MUL);
	g = tofix(v.g, FIX_POINT_F32_MUL);
	b = tofix(v.b, FIX_POINT_F32_MUL);
}

/*
	load a color value
*/
REALINLINE void getSample_color (	tFixPoint &a, tFixPoint &r, tFixPoint &g, tFixPoint &b,
							const sVec4 &v
							)
{
	a = tofix ( v.a, FIX_POINT_F32_MUL);
	r = tofix ( v.r, COLOR_MAX * FIX_POINT_F32_MUL);
	g = tofix ( v.g, COLOR_MAX * FIX_POINT_F32_MUL);
	b = tofix ( v.b, COLOR_MAX * FIX_POINT_F32_MUL);
}

/*
	load a color value
*/
REALINLINE void getSample_color ( tFixPoint &r, tFixPoint &g, tFixPoint &b,const sVec4 &v )
{
	r = tofix ( v.r, COLOR_MAX * FIX_POINT_F32_MUL);
	g = tofix ( v.g, COLOR_MAX * FIX_POINT_F32_MUL);
	b = tofix ( v.b, COLOR_MAX * FIX_POINT_F32_MUL);
}

/*
	load a color value mulby controls [0;1] or [0;ColorMax]
*/
REALINLINE void getSample_color (	tFixPoint &r, tFixPoint &g, tFixPoint &b,
								const sVec4 &v, const f32 mulby )
{
	r = tofix ( v.r, mulby);
	g = tofix ( v.g, mulby);
	b = tofix ( v.b, mulby);
}

REALINLINE void getSample_color(tFixPoint &a,tFixPoint &r, tFixPoint &g, tFixPoint &b,const sVec4 &v, const f32 mulby)
{
	a = tofix(v.a, mulby);
	r = tofix(v.r, mulby);
	g = tofix(v.g, mulby);
	b = tofix(v.b, mulby);
}


}

}

#endif


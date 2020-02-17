// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "SoftwareDriver2_helper.h"
#include "CBlit.h"

namespace irr
{
namespace video
{

	struct SBlitJob
	{
		AbsRectangle Dest;
		AbsRectangle Source;

		union
		{
			u32 argb;
			SColor col[4];
		};

		void* src;
		void* dst;

		u32 width;
		u32 height;

		u32 srcPitch;
		u32 dstPitch;

		//u32 srcPixelMul;
		//u32 dstPixelMul;

		bool stretch;
		float x_stretch;
		float y_stretch;

		SBlitJob() : stretch(false) {}
	};

	// Bitfields Cohen Sutherland
	enum eClipCode
	{
		CLIPCODE_EMPTY	=	0,
		CLIPCODE_BOTTOM	=	1,
		CLIPCODE_TOP	=	2,
		CLIPCODE_LEFT	=	4,
		CLIPCODE_RIGHT	=	8
	};

inline u32 GetClipCode( const AbsRectangle &r, const core::position2d<s32> &p )
{
	u32 code = CLIPCODE_EMPTY;

	if ( p.X < r.x0 )
		code = CLIPCODE_LEFT;
	else
	if ( p.X > r.x1 )
		code = CLIPCODE_RIGHT;

	if ( p.Y < r.y0 )
		code |= CLIPCODE_TOP;
	else
	if ( p.Y > r.y1 )
		code |= CLIPCODE_BOTTOM;

	return code;
}


/*!
	Cohen Sutherland clipping
	@return: 1 if valid
*/

static int ClipLine(const AbsRectangle &clipping,
			core::position2d<s32> &p0,
			core::position2d<s32> &p1,
			const core::position2d<s32>& p0_in,
			const core::position2d<s32>& p1_in)
{
	u32 code0;
	u32 code1;
	u32 code;

	p0 = p0_in;
	p1 = p1_in;

	code0 = GetClipCode( clipping, p0 );
	code1 = GetClipCode( clipping, p1 );

	// trivial accepted
	while ( code0 | code1 )
	{
		s32 x=0;
		s32 y=0;

		// trivial reject
		if ( code0 & code1 )
			return 0;

		if ( code0 )
		{
			// clip first point
			code = code0;
		}
		else
		{
			// clip last point
			code = code1;
		}

		if ( (code & CLIPCODE_BOTTOM) == CLIPCODE_BOTTOM )
		{
			// clip bottom viewport
			y = clipping.y1;
			x = p0.X + ( p1.X - p0.X ) * ( y - p0.Y ) / ( p1.Y - p0.Y );
		}
		else
		if ( (code & CLIPCODE_TOP) == CLIPCODE_TOP )
		{
			// clip to viewport
			y = clipping.y0;
			x = p0.X + ( p1.X - p0.X ) * ( y - p0.Y ) / ( p1.Y - p0.Y );
		}
		else
		if ( (code & CLIPCODE_RIGHT) == CLIPCODE_RIGHT )
		{
			// clip right viewport
			x = clipping.x1;
			y = p0.Y + ( p1.Y - p0.Y ) * ( x - p0.X ) / ( p1.X - p0.X );
		}
		else
		if ( (code & CLIPCODE_LEFT) == CLIPCODE_LEFT )
		{
			// clip left viewport
			x = clipping.x0;
			y = p0.Y + ( p1.Y - p0.Y ) * ( x - p0.X ) / ( p1.X - p0.X );
		}

		if ( code == code0 )
		{
			// modify first point
			p0.X = x;
			p0.Y = y;
			code0 = GetClipCode( clipping, p0 );
		}
		else
		{
			// modify second point
			p1.X = x;
			p1.Y = y;
			code1 = GetClipCode( clipping, p1 );
		}
	}

	return 1;
}

/*
*/
inline void GetClip(AbsRectangle &clipping, const video::IImage * t)
{
	clipping.x0 = 0;
	clipping.y0 = 0;
	clipping.x1 = t->getDimension().Width - 1;
	clipping.y1 = t->getDimension().Height - 1;
}


/*
	return alpha in [0;255] Granularity and 32-Bit ARGB
	add highbit alpha ( alpha > 127 ? + 1 )
*/
static inline u32 packAlpha(const u32 c)
{
	return (c > 127 ? c - 1 : c) << 24;
}


/*!
	Scale Color by (1/value)
	value 0 - 256 ( alpha )
*/
inline u32 PixelLerp32(const u32 source, const u32 value)
{
	u32 srcRB = source & 0x00FF00FF;
	u32 srcXG = (source & 0xFF00FF00) >> 8;

	srcRB *= value;
	srcXG *= value;

	srcRB >>= 8;
	//srcXG >>= 8;

	srcXG &= 0xFF00FF00;
	srcRB &= 0x00FF00FF;

	return srcRB | srcXG;
}


/*
*/
void RenderLine32_Decal(video::IImage *t,
				const core::position2d<s32> &p0,
				const core::position2d<s32> &p1,
				u32 argb )
{
	s32 dx = p1.X - p0.X;
	s32 dy = p1.Y - p0.Y;

	s32 c;
	s32 m;
	s32 d = 0;
	s32 run;

	s32 xInc = 4;
	s32 yInc = (s32) t->getPitch();

	if ( dx < 0 )
	{
		xInc = -xInc;
		dx = -dx;
	}

	if ( dy < 0 )
	{
		yInc = -yInc;
		dy = -dy;
	}

	u32 *dst;
	dst = (u32*) ( (u8*) t->getData() + ( p0.Y * t->getPitch() ) + ( p0.X << 2 ) );

	if ( dy > dx )
	{
		s32 tmp;
		tmp = dx;
		dx = dy;
		dy = tmp;
		tmp = xInc;
		xInc = yInc;
		yInc = tmp;
	}

	c = dx << 1;
	m = dy << 1;

	run = dx;
	do
	{
		*dst = argb;

		dst = (u32*) ( (u8*) dst + xInc );	// x += xInc
		d += m;
		if ( d > dx )
		{
			dst = (u32*) ( (u8*) dst + yInc );	// y += yInc
			d -= c;
		}
		run -= 1;
	} while (run>=0);
}


/*
*/
void RenderLine32_Blend(video::IImage *t,
				const core::position2d<s32> &p0,
				const core::position2d<s32> &p1,
				u32 argb, u32 alpha)
{
	s32 dx = p1.X - p0.X;
	s32 dy = p1.Y - p0.Y;

	s32 c;
	s32 m;
	s32 d = 0;
	s32 run;

	s32 xInc = 4;
	s32 yInc = (s32) t->getPitch();

	if ( dx < 0 )
	{
		xInc = -xInc;
		dx = -dx;
	}

	if ( dy < 0 )
	{
		yInc = -yInc;
		dy = -dy;
	}

	u32 *dst;
	dst = (u32*) ( (u8*) t->getData() + ( p0.Y * t->getPitch() ) + ( p0.X << 2 ) );

	if ( dy > dx )
	{
		s32 tmp;
		tmp = dx;
		dx = dy;
		dy = tmp;
		tmp = xInc;
		xInc = yInc;
		yInc = tmp;
	}

	c = dx << 1;
	m = dy << 1;

	run = dx;
	const u32 packA = packAlpha ( alpha );
	do
	{
		*dst = packA | PixelBlend32( *dst, argb, alpha );

		dst = (u32*) ( (u8*) dst + xInc );	// x += xInc
		d += m;
		if ( d > dx )
		{
			dst = (u32*) ( (u8*) dst + yInc );	// y += yInc
			d -= c;
		}
		run -= 1;
	} while (run>=0);
}

/*
*/
void RenderLine16_Decal(video::IImage *t,
				const core::position2d<s32> &p0,
				const core::position2d<s32> &p1,
				u32 argb )
{
	s32 dx = p1.X - p0.X;
	s32 dy = p1.Y - p0.Y;

	s32 c;
	s32 m;
	s32 d = 0;
	s32 run;

	s32 xInc = 2;
	s32 yInc = (s32) t->getPitch();

	if ( dx < 0 )
	{
		xInc = -xInc;
		dx = -dx;
	}

	if ( dy < 0 )
	{
		yInc = -yInc;
		dy = -dy;
	}

	u16 *dst;
	dst = (u16*) ( (u8*) t->getData() + ( p0.Y * t->getPitch() ) + ( p0.X << 1 ) );

	if ( dy > dx )
	{
		s32 tmp;
		tmp = dx;
		dx = dy;
		dy = tmp;
		tmp = xInc;
		xInc = yInc;
		yInc = tmp;
	}

	c = dx << 1;
	m = dy << 1;

	run = dx;
	do
	{
		*dst = (u16)argb;

		dst = (u16*) ( (u8*) dst + xInc );	// x += xInc
		d += m;
		if ( d > dx )
		{
			dst = (u16*) ( (u8*) dst + yInc );	// y += yInc
			d -= c;
		}
		run -= 1;
	} while (run>=0);
}

/*
*/
void RenderLine16_Blend(video::IImage *t,
				const core::position2d<s32> &p0,
				const core::position2d<s32> &p1,
				u16 argb,
				u16 alpha)
{
	s32 dx = p1.X - p0.X;
	s32 dy = p1.Y - p0.Y;

	s32 c;
	s32 m;
	s32 d = 0;
	s32 run;

	s32 xInc = 2;
	s32 yInc = (s32) t->getPitch();

	if ( dx < 0 )
	{
		xInc = -xInc;
		dx = -dx;
	}

	if ( dy < 0 )
	{
		yInc = -yInc;
		dy = -dy;
	}

	u16 *dst;
	dst = (u16*) ( (u8*) t->getData() + ( p0.Y * t->getPitch() ) + ( p0.X << 1 ) );

	if ( dy > dx )
	{
		s32 tmp;
		tmp = dx;
		dx = dy;
		dy = tmp;
		tmp = xInc;
		xInc = yInc;
		yInc = tmp;
	}

	c = dx << 1;
	m = dy << 1;

	run = dx;
	const u16 packA = alpha ? 0x8000 : 0;
	do
	{
		*dst = packA | PixelBlend16( *dst, argb, alpha );

		dst = (u16*) ( (u8*) dst + xInc );	// x += xInc
		d += m;
		if ( d > dx )
		{
			dst = (u16*) ( (u8*) dst + yInc );	// y += yInc
			d -= c;
		}
		run -= 1;
	} while (run>=0);
}


/*!
*/
void executeBlit_TextureCopy_x_to_x( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	if (job->stretch)
	{
		//assume 32bit..
		const u32 *src = static_cast<const u32*>(job->src);
		u32 *dst = static_cast<u32*>(job->dst);

		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		float src_x = 0.f;
		float src_y = 0.f;
		for ( u32 dy = 0; dy < h; ++dy,src_y += hscale )
		{
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*((u32) src_y) );

			src_x = 0.f;
			for ( u32 dx = 0; dx < w; ++dx,src_x += wscale )
			{
				dst[dx] = src[(u32)src_x];
			}
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		const size_t widthPitch = job->dstPitch; // job->width * job->dstPixelMul;
		const void *src = (void*) job->src;
		void *dst = (void*) job->dst;

		for ( u32 dy = 0; dy != h; ++dy )
		{
			memcpy( dst, src, widthPitch );

			src = (void*) ( (u8*) (src) + job->srcPitch );
			dst = (void*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

/*!
*/
void executeBlit_TextureCopy_32_to_16( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u32 *src = static_cast<const u32*>(job->src);
	u16 *dst = static_cast<u16*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*src_y );

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = (u32)(dx*wscale);
				//16 bit Blitter depends on pre-multiplied color
				const u32 s = PixelLerp32( src[src_x] | 0xFF000000, extractAlpha( src[src_x] ) );
				dst[dx] = video::A8R8G8B8toA1R5G5B5( s );
			}
			dst = (u16*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				//16 bit Blitter depends on pre-multiplied color
				const u32 s = PixelLerp32( src[dx] | 0xFF000000, extractAlpha( src[dx] ) );
				dst[dx] = video::A8R8G8B8toA1R5G5B5( s );
			}

			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst = (u16*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

/*!
*/
void executeBlit_TextureCopy_24_to_16( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u8 *src = static_cast<const u8*>(job->src);
	u16 *dst = static_cast<u16*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 3.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u8*)(job->src) + job->srcPitch*src_y;

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u8* src_x = src+(u32)(dx*wscale);
				dst[dx] = video::RGBA16(src_x[0], src_x[1], src_x[2]);
			}
			dst = (u16*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			const u8* s = src;
			for ( u32 dx = 0; dx != w; ++dx )
			{
				dst[dx] = video::RGBA16(s[0], s[1], s[2]);
				s += 3;
			}

			src = src+job->srcPitch;
			dst = (u16*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}


/*!
*/
void executeBlit_TextureCopy_16_to_32( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u16 *src = static_cast<const u16*>(job->src);
	u32 *dst = static_cast<u32*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u16*) ( (u8*) (job->src) + job->srcPitch*src_y );

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = (u32)(dx*wscale);
				dst[dx] = video::A1R5G5B5toA8R8G8B8(src[src_x]);
			}
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				dst[dx] = video::A1R5G5B5toA8R8G8B8( src[dx] );
			}

			src = (u16*) ( (u8*) (src) + job->srcPitch );
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

void executeBlit_TextureCopy_16_to_24( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u16 *src = static_cast<const u16*>(job->src);
	u8 *dst = static_cast<u8*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u16*) ( (u8*) (job->src) + job->srcPitch*src_y );

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = (u32)(dx*wscale);
				u32 color = video::A1R5G5B5toA8R8G8B8(src[src_x]);
				u8 * writeTo = &dst[dx * 3];
				*writeTo++ = (color >> 16)& 0xFF;
				*writeTo++ = (color >> 8) & 0xFF;
				*writeTo++ = color & 0xFF;
			}
			dst += job->dstPitch;
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				u32 color = video::A1R5G5B5toA8R8G8B8(src[dx]);
				u8 * writeTo = &dst[dx * 3];
				*writeTo++ = (color >> 16)& 0xFF;
				*writeTo++ = (color >> 8) & 0xFF;
				*writeTo++ = color & 0xFF;
			}

			src = (u16*) ( (u8*) (src) + job->srcPitch );
			dst += job->dstPitch;
		}
	}
}

/*!
*/
void executeBlit_TextureCopy_24_to_32( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u8 *src = static_cast<const u8*>(job->src);
	u32 *dst = static_cast<u32*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 3.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (const u8*)job->src+(job->srcPitch*src_y);

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u8* s = src+(u32)(dx*wscale);
				dst[dx] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
			}
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		for ( s32 dy = 0; dy != job->height; ++dy )
		{
			const u8* s = src;

			for ( s32 dx = 0; dx != job->width; ++dx )
			{
				dst[dx] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
				s += 3;
			}

			src = src + job->srcPitch;
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

void executeBlit_TextureCopy_32_to_24( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u32 *src = static_cast<const u32*>(job->src);
	u8 *dst = static_cast<u8*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*src_y);

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = src[(u32)(dx*wscale)];
				u8 * writeTo = &dst[dx * 3];
				*writeTo++ = (src_x >> 16)& 0xFF;
				*writeTo++ = (src_x >> 8) & 0xFF;
				*writeTo++ = src_x & 0xFF;
			}
			dst += job->dstPitch;
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				u8 * writeTo = &dst[dx * 3];
				*writeTo++ = (src[dx] >> 16)& 0xFF;
				*writeTo++ = (src[dx] >> 8) & 0xFF;
				*writeTo++ = src[dx] & 0xFF;
			}

			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst += job->dstPitch;
		}
	}
}


/*!
*/
void executeBlit_TextureBlend_16_to_16( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u32 rdx = w>>1;

	const u32 *src = (u32*) job->src;
	u32 *dst = (u32*) job->dst;

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;
		const u32 off = core::if_c_a_else_b(w&1, (u32)((w-1)*wscale), 0);
		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*src_y );

			for ( u32 dx = 0; dx < rdx; ++dx )
			{
				const u32 src_x = (u32)(dx*wscale);
				dst[dx] = PixelBlend16_simd( dst[dx], src[src_x] );
			}
			if ( off )
			{
				((u16*) dst)[off] = PixelBlend16( ((u16*) dst)[off], ((u16*) src)[off] );
			}

			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
	else
	{
		const u32 off = core::if_c_a_else_b(w&1, w-1, 0);
		for (u32 dy = 0; dy != h; ++dy )
		{
			for (u32 dx = 0; dx != rdx; ++dx )
			{
				dst[dx] = PixelBlend16_simd( dst[dx], src[dx] );
			}

			if ( off )
			{
				((u16*) dst)[off] = PixelBlend16( ((u16*) dst)[off], ((u16*) src)[off] );
			}

			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

/*!
*/
void executeBlit_TextureBlend_32_to_32( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u32 *src = (u32*) job->src;
	u32 *dst = (u32*) job->dst;

	if (job->stretch)
	{
		//assume 32bit..
		const u32 *src = static_cast<const u32*>(job->src);
		u32 *dst = static_cast<u32*>(job->dst);

		const float wscale = 1.f / job->x_stretch;
		const float hscale = 1.f / job->y_stretch;

		float src_x = 0.f;
		float src_y = 0.f;
		for (u32 dy = 0; dy < h; ++dy, src_y += hscale)
		{
			src = (u32*)((u8*)(job->src) + job->srcPitch*((u32)src_y));

			src_x = 0.f;
			for (u32 dx = 0; dx < w; ++dx, src_x += wscale)
			{
				dst[dx] = PixelBlend32(dst[dx],src[(u32)src_x]);
			}
			dst = (u32*)((u8*)(dst)+job->dstPitch);
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				dst[dx] = PixelBlend32( dst[dx], src[dx] );
			}
			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

/*!
*/
void executeBlit_TextureBlendColor_16_to_16( const SBlitJob * job )
{
	u16 *src = (u16*) job->src;
	u16 *dst = (u16*) job->dst;

	u16 blend = video::A8R8G8B8toA1R5G5B5 ( job->argb );
	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		for ( s32 dx = 0; dx != job->width; ++dx )
		{
			if ( 0 == (src[dx] & 0x8000) )
				continue;

			dst[dx] = PixelMul16_2( src[dx], blend );
		}
		src = (u16*) ( (u8*) (src) + job->srcPitch );
		dst = (u16*) ( (u8*) (dst) + job->dstPitch );
	}
}


/*!
	c1 == src
	c2 == dest
	alpha = c1.alpha * c2.alpha

	Pixel = (c1.color * src_color) * alpha + c2.color * ( 1 - alpha )
*/
inline void PixelBlend32_2(a8r8g8b8* dst, const a8r8g8b8* src, const tFixPoint c[4])
{
	tFixPoint s[4];
	color_to_fix(s, src[0]);

	tFixPoint a0;
	a0 = imulFix_simple(s[0], c[0]);
	if (0 == a0)
		return;

	s[1] = imulFix_simple(s[1], c[1]);
	s[2] = imulFix_simple(s[2], c[2]);
	s[3] = imulFix_simple(s[3], c[3]);

	if (a0 < FIXPOINT_COLOR_MAX)
	{
		tFixPoint d[4];
		color_to_fix(d, dst[0]);

		fix_color_norm(a0);

		s[1] = d[1] + imulFix(a0, s[1] - d[1]);
		s[2] = d[2] + imulFix(a0, s[2] - d[2]);
		s[3] = d[3] + imulFix(a0, s[3] - d[3]);
	}
	dst[0] = fix_to_sample(s[1], s[2], s[3]);
}

/*!
*/
void executeBlit_TextureBlendColor_32_to_32( const SBlitJob * job )
{
	const u32 *src = (u32*) job->src;
	u32 *dst = (u32*) job->dst;

	tFixPoint c[4];
	color_to_fix1(c, job->argb);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		float src_x = 0.f;
		float src_y = 0.f;
		for ( u32 dy = 0; dy < job->height; ++dy,src_y += hscale )
		{
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*((u32) src_y) );

			src_x = 0.f;
			for ( u32 dx = 0; dx < job->width; ++dx,src_x += wscale )
			{
				//dst[dx] = PixelBlend32( dst[dx], PixelMul32_2( src[(u32)src_x], job->argb ) );
				PixelBlend32_2(dst + dx, src + (size_t)src_x, c);
			}
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}

	}
	else
	{
		for ( u32 dy = 0; dy != job->height; ++dy )
		{
			for ( u32 dx = 0; dx < job->width; ++dx )
			{
				//dst[dx] = PixelBlend32( dst[dx], PixelMul32_2( src[dx], job->argb ) );
				PixelBlend32_2(dst + dx, src + dx, c);
			}
			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst = (u32*) ( (u8*) (dst) + job->dstPitch );
		}
	}
}

/*!
*/
void executeBlit_Color_16_to_16( const SBlitJob * job )
{
	const u16 c = video::A8R8G8B8toA1R5G5B5(job->argb);
	u16 *dst = (u16*) job->dst;

	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		memset16(dst, c, job->srcPitch);
		dst = (u16*) ( (u8*) (dst) + job->dstPitch );
	}
}

/*!
*/
void executeBlit_Color_32_to_32( const SBlitJob * job )
{
	u32 *dst = (u32*) job->dst;

	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		memset32( dst, job->argb, job->srcPitch );
		dst = (u32*) ( (u8*) (dst) + job->dstPitch );
	}
}

/*!
*/
void executeBlit_ColorAlpha_16_to_16( const SBlitJob * job )
{
	u16 *dst = (u16*) job->dst;

	const u16 alpha = extractAlpha( job->argb ) >> 3;
	if ( 0 == alpha )
		return;
	const u32 src = video::A8R8G8B8toA1R5G5B5( job->argb );

	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		for ( s32 dx = 0; dx != job->width; ++dx )
		{
			dst[dx] = 0x8000 | PixelBlend16( dst[dx], src, alpha );
		}
		dst = (u16*) ( (u8*) (dst) + job->dstPitch );
	}
}

/*!
*/
void executeBlit_ColorAlpha_32_to_32( const SBlitJob * job )
{
	u32 *dst = (u32*) job->dst;

	const u32 alpha = extractAlpha( job->argb );
	const u32 src = job->argb;

	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		for ( s32 dx = 0; dx != job->width; ++dx )
		{
			dst[dx] = (job->argb & 0xFF000000 ) | PixelBlend32( dst[dx], src, alpha );
		}
		dst = (u32*) ( (u8*) (dst) + job->dstPitch );
	}
}

/*!
	Pixel =>
			color = sourceAlpha > 0 ? source, else dest
			alpha = max(destAlpha, sourceAlpha)
*/
inline u16 PixelCombine16(const u16 c2, const u16 c1)
{
	if (video::getAlpha(c1) > 0)
		return c1;
	else
		return c2;
}



/*!
	Combine alpha channels (increases alpha / reduces transparency)
*/
void executeBlit_TextureCombineColor_16_to_16( const SBlitJob * job )
{
	/*
		Stretch not supported.
	*/
	const u16 *src = (u16*) job->src;
	u16 *dst = (u16*) job->dst;

	const u16 jobColor = video::A8R8G8B8toA1R5G5B5( job->argb );
	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		for ( s32 dx = 0; dx != job->width; ++dx )
		{
			dst[dx] = PixelCombine16( dst[dx], PixelMul16_2( src[dx], jobColor ) );
		}
		src = (u16*) ( (u8*) (src) + job->srcPitch );
		dst = (u16*) ( (u8*) (dst) + job->dstPitch );
	}

}

/*!
	Combine alpha channels (increases alpha / reduces transparency)
*/
void executeBlit_TextureCombineColor_16_to_24( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u16 *src = static_cast<const u16*>(job->src);
	u8 *dst = static_cast<u8*>(job->dst);

	const u16 jobColor = video::A8R8G8B8toA1R5G5B5( job->argb );

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u16*) ( (u8*) (job->src) + job->srcPitch*src_y );

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = (u32)(dx*wscale);
				u32 color = PixelMul16_2( video::A1R5G5B5toA8R8G8B8(src[src_x]), jobColor);
				u8 * writeTo = &dst[dx * 3];
				if ( video::getAlpha(src[src_x]) > 0 ) // only overlay if source has visible alpha (alpha == 1)
				{
					*writeTo++ = (color >> 16)& 0xFF;
					*writeTo++ = (color >> 8) & 0xFF;
					*writeTo++ = color & 0xFF;
				}
			}
			dst += job->dstPitch;
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				u32 color = PixelMul16_2( video::A1R5G5B5toA8R8G8B8(src[dx]), jobColor);
				u8 * writeTo = &dst[dx * 3];
				if ( video::getAlpha(src[dx]) > 0 ) // only overlay if source has visible alpha (alpha == 1)
				{
					*writeTo++ = (color >> 16)& 0xFF;
					*writeTo++ = (color >> 8) & 0xFF;
					*writeTo++ = color & 0xFF;
				}
			}

			src = (u16*) ( (u8*) (src) + job->srcPitch );
			dst += job->dstPitch;
		}
	}
}

/*!
	Pixel =>
			color = dest * ( 1 - SourceAlpha ) + source * SourceAlpha,
			alpha = destAlpha * ( 1 - SourceAlpha ) + sourceAlpha

	where "1" means "full scale" (255)
*/
inline u32 PixelCombine32(const u32 c2, const u32 c1)
{
	// alpha test
	u32 alpha = c1 & 0xFF000000;

	if (0 == alpha)
		return c2;
	if (0xFF000000 == alpha)
	{
		return c1;
	}

	alpha >>= 24;

	// add highbit alpha, if ( alpha > 127 ) alpha += 1;
	// stretches [0;255] to [0;256] to avoid division by 255. use division 256 == shr 8
	alpha += (alpha >> 7);

	u32 srcRB = c1 & 0x00FF00FF;
	u32 srcXG = c1 & 0x0000FF00;

	u32 dstRB = c2 & 0x00FF00FF;
	u32 dstXG = c2 & 0x0000FF00;


	u32 rb = srcRB - dstRB;
	u32 xg = srcXG - dstXG;

	rb *= alpha;
	xg *= alpha;
	rb >>= 8;
	xg >>= 8;

	rb += dstRB;
	xg += dstXG;

	rb &= 0x00FF00FF;
	xg &= 0x0000FF00;

	u32 sa = c1 >> 24;
	u32 da = c2 >> 24;
	u32 blendAlpha_fix8 = (sa * 256 + da * (256 - alpha)) >> 8;
	return blendAlpha_fix8 << 24 | rb | xg;
}

/*!
	Combine alpha channels (increases alpha / reduces transparency)
	Destination alpha is treated as full 255
*/
void executeBlit_TextureCombineColor_32_to_24( const SBlitJob * job )
{
	const u32 w = job->width;
	const u32 h = job->height;
	const u32 *src = static_cast<const u32*>(job->src);
	u8 *dst = static_cast<u8*>(job->dst);

	if (job->stretch)
	{
		const float wscale = 1.f/job->x_stretch;
		const float hscale = 1.f/job->y_stretch;

		for ( u32 dy = 0; dy < h; ++dy )
		{
			const u32 src_y = (u32)(dy*hscale);
			src = (u32*) ( (u8*) (job->src) + job->srcPitch*src_y);

			for ( u32 dx = 0; dx < w; ++dx )
			{
				const u32 src_x = src[(u32)(dx*wscale)];
				u8* writeTo = &dst[dx * 3];
				const u32 dst_x = 0xFF000000 | writeTo[0] << 16 | writeTo[1] << 8 | writeTo[2];
				const u32 combo = PixelCombine32( dst_x, PixelMul32_2( src_x, job->argb ) );
				*writeTo++ = (combo >> 16) & 0xFF;
				*writeTo++ = (combo >> 8) & 0xFF;
				*writeTo++ = combo & 0xFF;
			}
			dst += job->dstPitch;
		}
	}
	else
	{
		for ( u32 dy = 0; dy != h; ++dy )
		{
			for ( u32 dx = 0; dx != w; ++dx )
			{
				u8* writeTo = &dst[dx * 3];
				const u32 dst_x = 0xFF000000 | writeTo[0] << 16 | writeTo[1] << 8 | writeTo[2];
				const u32 combo = PixelCombine32( dst_x, PixelMul32_2( src[dx], job->argb ) );
				*writeTo++ = (combo >> 16) & 0xFF;
				*writeTo++ = (combo >> 8) & 0xFF;
				*writeTo++ = combo & 0xFF;
			}

			src = (u32*) ( (u8*) (src) + job->srcPitch );
			dst += job->dstPitch;
		}
	}
}

/*!
	Combine alpha channels (increases alpha / reduces transparency)
*/
void executeBlit_TextureCombineColor_32_to_32( const SBlitJob * job )
{
	u32 *src = (u32*) job->src;
	u32 *dst = (u32*) job->dst;

	for ( s32 dy = 0; dy != job->height; ++dy )
	{
		for ( s32 dx = 0; dx != job->width; ++dx )
		{
			dst[dx] = PixelCombine32( dst[dx], PixelMul32_2( src[dx], job->argb ) );
		}
		src = (u32*) ( (u8*) (src) + job->srcPitch );
		dst = (u32*) ( (u8*) (dst) + job->dstPitch );
	}
}


typedef void (*tExecuteBlit) ( const SBlitJob * job );




/*!
*/
struct blitterTable
{
	eBlitter operation;
	s32 destFormat;
	s32 sourceFormat;
	tExecuteBlit func;
};

static const blitterTable blitTable[] =
{
	{ BLITTER_TEXTURE, -2, -2, executeBlit_TextureCopy_x_to_x },
	{ BLITTER_TEXTURE, video::ECF_A1R5G5B5, video::ECF_A8R8G8B8, executeBlit_TextureCopy_32_to_16 },
	{ BLITTER_TEXTURE, video::ECF_A1R5G5B5, video::ECF_R8G8B8, executeBlit_TextureCopy_24_to_16 },
	{ BLITTER_TEXTURE, video::ECF_A8R8G8B8, video::ECF_A1R5G5B5, executeBlit_TextureCopy_16_to_32 },
	{ BLITTER_TEXTURE, video::ECF_A8R8G8B8, video::ECF_R8G8B8, executeBlit_TextureCopy_24_to_32 },
	{ BLITTER_TEXTURE, video::ECF_R8G8B8, video::ECF_A1R5G5B5, executeBlit_TextureCopy_16_to_24 },
	{ BLITTER_TEXTURE, video::ECF_R8G8B8, video::ECF_A8R8G8B8, executeBlit_TextureCopy_32_to_24 },
	{ BLITTER_TEXTURE_ALPHA_BLEND, video::ECF_A1R5G5B5, video::ECF_A1R5G5B5, executeBlit_TextureBlend_16_to_16 },
	{ BLITTER_TEXTURE_ALPHA_BLEND, video::ECF_A8R8G8B8, video::ECF_A8R8G8B8, executeBlit_TextureBlend_32_to_32 },
	{ BLITTER_TEXTURE_ALPHA_COLOR_BLEND, video::ECF_A1R5G5B5, video::ECF_A1R5G5B5, executeBlit_TextureBlendColor_16_to_16 },
	{ BLITTER_TEXTURE_ALPHA_COLOR_BLEND, video::ECF_A8R8G8B8, video::ECF_A8R8G8B8, executeBlit_TextureBlendColor_32_to_32 },
	{ BLITTER_COLOR, video::ECF_A1R5G5B5, -1, executeBlit_Color_16_to_16 },
	{ BLITTER_COLOR, video::ECF_A8R8G8B8, -1, executeBlit_Color_32_to_32 },
	{ BLITTER_COLOR_ALPHA, video::ECF_A1R5G5B5, -1, executeBlit_ColorAlpha_16_to_16 },
	{ BLITTER_COLOR_ALPHA, video::ECF_A8R8G8B8, -1, executeBlit_ColorAlpha_32_to_32 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_A8R8G8B8, video::ECF_A8R8G8B8, executeBlit_TextureCombineColor_32_to_32 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_A8R8G8B8, video::ECF_R8G8B8, executeBlit_TextureCopy_24_to_32 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_R8G8B8, video::ECF_A8R8G8B8, executeBlit_TextureCombineColor_32_to_24 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_R8G8B8, video::ECF_R8G8B8, executeBlit_TextureCopy_x_to_x },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_A1R5G5B5, video::ECF_R8G8B8, executeBlit_TextureCopy_24_to_16 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_A1R5G5B5, video::ECF_A1R5G5B5, executeBlit_TextureCombineColor_16_to_16 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_A1R5G5B5, video::ECF_R8G8B8, executeBlit_TextureCopy_24_to_16 },
	{ BLITTER_TEXTURE_COMBINE_ALPHA, video::ECF_R8G8B8, video::ECF_A1R5G5B5, executeBlit_TextureCombineColor_16_to_24 },
	{ BLITTER_INVALID, -1, -1, 0 }
};


static inline tExecuteBlit getBlitter( eBlitter operation,const video::IImage * dest,const video::IImage * source )
{
	video::ECOLOR_FORMAT sourceFormat = (video::ECOLOR_FORMAT) ( source ? source->getColorFormat() : -1 );
	video::ECOLOR_FORMAT destFormat = (video::ECOLOR_FORMAT) ( dest ? dest->getColorFormat() : -1 );

	const blitterTable * b = blitTable;

	while ( b->operation != BLITTER_INVALID )
	{
		if ( b->operation == operation )
		{
			if (( b->destFormat == -1 || b->destFormat == destFormat ) &&
				( b->sourceFormat == -1 || b->sourceFormat == sourceFormat ) )
					return b->func;
			else
			if ( b->destFormat == -2 && ( sourceFormat == destFormat ) )
					return b->func;
		}
		b += 1;
	}
	return 0;
}


// bounce clipping to texture
inline void setClip ( AbsRectangle &out, const core::rect<s32> *clip,
					const video::IImage * tex,const core::dimension2d<u32>* tex_org, s32 passnative )
{
	if ( clip && 0 == tex && passnative )
	{
		out.x0 = clip->UpperLeftCorner.X;
		out.x1 = clip->LowerRightCorner.X;
		out.y0 = clip->UpperLeftCorner.Y;
		out.y1 = clip->LowerRightCorner.Y;
		return;
	}

	const u32 w = tex ? tex->getDimension().Width : 0;
	const u32 h = tex ? tex->getDimension().Height : 0;
	//driver could have changed texture size.
	if ( clip && tex_org && (w != tex_org->Width || h != tex_org->Height) )
	{
		out.x0 = core::s32_clamp ( (clip->UpperLeftCorner.X*w)/tex_org->Width, 0, w );
		out.x1 = core::s32_clamp ( (clip->LowerRightCorner.X*w)/tex_org->Width, out.x0, w );
		out.y0 = core::s32_clamp ( (clip->UpperLeftCorner.Y*h)/tex_org->Height, 0, h );
		out.y1 = core::s32_clamp ( (clip->LowerRightCorner.Y*h)/tex_org->Height, out.y0, h );
	}
	else
	if ( clip )
	{
		out.x0 = core::s32_clamp ( clip->UpperLeftCorner.X, 0, w );
		out.x1 = core::s32_clamp ( clip->LowerRightCorner.X, out.x0, w );
		out.y0 = core::s32_clamp ( clip->UpperLeftCorner.Y, 0, h );
		out.y1 = core::s32_clamp ( clip->LowerRightCorner.Y, out.y0, h );
	}
	else
	{
		out.x0 = 0;
		out.y0 = 0;
		out.x1 = w;
		out.y1 = h;
	}

}

// bounce clipping to texture
inline void setSourceClip(AbsRectangle &out, const core::rect<s32> *in,
	const video::IImage * tex, const core::dimension2d<u32>* tex_org)
{
	if (0 == in)
	{
		out.x0 = 0;
		out.x1 = 0;
		out.y0 = 0;
		out.y1 = 0;
		return;
	}

	if (0 == tex)
	{
		out.x0 = in->UpperLeftCorner.X;
		out.x1 = in->LowerRightCorner.X;
		out.y0 = in->UpperLeftCorner.Y;
		out.y1 = in->LowerRightCorner.Y;
		return;
	}

	const u32 w = tex->getDimension().Width;
	const u32 h = tex->getDimension().Height;

	//texcoo (x / original) * tex
	out.x0 = core::s32_clamp((in->UpperLeftCorner.X*w) / tex_org->Width, 0, w);
	out.x1 = core::s32_clamp((in->LowerRightCorner.X*w) / tex_org->Width, out.x0, w);
	out.y0 = core::s32_clamp((in->UpperLeftCorner.Y*h) / tex_org->Height, 0, h);
	out.y1 = core::s32_clamp((in->LowerRightCorner.Y*h) / tex_org->Height, out.y0, h);
}

/*!
	a generic 2D Blitter
*/
s32 Blit(eBlitter operation,
		video::IImage* dest,const core::rect<s32>* destClipping,const core::position2d<s32>* destPos,
		const video::IImage* source,const core::rect<s32>* sourceClipping,const core::dimension2d<u32>* src_originalSize,
		const video::SColor* color, u32 color_size
)
{
	tExecuteBlit blitter = getBlitter( operation, dest, source );
	if ( 0 == blitter ) return 0;

	// Clipping
	AbsRectangle sourceClip;
	AbsRectangle destClip;
	AbsRectangle v;

	SBlitJob job;

	setClip ( sourceClip, sourceClipping, source, src_originalSize,1 );
	setClip ( destClip, destClipping, dest, 0, 0 );

	v.x0 = destPos ? destPos->X : 0;
	v.y0 = destPos ? destPos->Y : 0;
	v.x1 = v.x0 + ( sourceClip.x1 - sourceClip.x0 );
	v.y1 = v.y0 + ( sourceClip.y1 - sourceClip.y0 );

	if ( !intersect( job.Dest, destClip, v ) )
		return 0;

	job.width = job.Dest.x1 - job.Dest.x0;
	job.height = job.Dest.y1 - job.Dest.y0;

	job.Source.x0 = sourceClip.x0 + ( job.Dest.x0 - v.x0 );
	job.Source.x1 = job.Source.x0 + job.width;
	job.Source.y0 = sourceClip.y0 + ( job.Dest.y0 - v.y0 );
	job.Source.y1 = job.Source.y0 + job.height;

	for (size_t i = 0; i < array_size(job.col); ++i)
	{
		job.col[i] = color && i < color_size ? color[i].color : 0xFFFFFFFF;
	}

	job.stretch = 0;
	job.x_stretch = 1.f;
	job.y_stretch = 1.f;


	if ( source )
	{
		job.srcPitch = source->getPitch();
		u32 srcPixelMul = source->getBytesPerPixel();
		job.src = (void*) ( (u8*) source->getData() + ( job.Source.y0 * job.srcPitch ) + ( job.Source.x0 * srcPixelMul ) );
	}
	else
	{
		// use srcPitch for color operation on dest
		job.srcPitch = job.width * dest->getBytesPerPixel();
	}

	job.dstPitch = dest->getPitch();
	u32 dstPixelMul = dest->getBytesPerPixel();
	job.dst = (void*) ( (u8*) dest->getData() + ( job.Dest.y0 * job.dstPitch ) + ( job.Dest.x0 * dstPixelMul ) );

	blitter( &job );

	return 1;
}

s32 StretchBlit(eBlitter operation,
		video::IImage* dest, const core::rect<s32>* destClipping,const core::rect<s32> *destRect,
		const video::IImage* source, const core::rect<s32> *srcRect, const core::dimension2d<u32>* src_originalSize,
		const video::SColor* color, u32 color_size
)
{
	SBlitJob job;
	for (size_t i = 0; i < array_size(job.col); ++i)
	{
		job.col[i] = color && i < color_size ? color[i].color : 0xFFFFFFFF;
	}

	tExecuteBlit blitter = getBlitter( operation, dest, source );
	if ( 0 == blitter ) return 0;

	// Clipping
	/*
		source:
		a) Texture     -> build UV on Original Pixelcoordinaten
		b) No Texture  -> srcRect in dest space
	*/
	setSourceClip( job.Source, srcRect, source, src_originalSize );
	setClip ( job.Dest, destRect, dest, 0, 0 );

	job.width = job.Dest.x1-job.Dest.x0;
	job.height = job.Dest.y1-job.Dest.y0;


	//scale gui needs destRect/srcRect. direct call assumes stretching.
	//still confused to match this with openGL.. pass unit test
	const int dst_w = destRect->getWidth();
	const int dst_h = destRect->getHeight();
	const int src_w = destClipping ? srcRect->getWidth() : job.Source.x1-job.Source.x0;
	const int src_h = destClipping ? srcRect->getHeight() : job.Source.y1-job.Source.y0;

	job.stretch = dst_w != src_w || dst_h != src_h;
	job.x_stretch = src_w ? (float)dst_w / (float)src_w : 1.f;
	job.y_stretch = src_h ? (float)dst_h / (float)src_h : 1.f;

	if ( source )
	{
		job.srcPitch = source->getPitch();
		u32 srcPixelMul = source->getBytesPerPixel();
		job.src = (void*) ( (u8*) source->getData() + ( job.Source.y0 * job.srcPitch ) + ( job.Source.x0 * srcPixelMul ) );
	}
	else
	{
		// use srcPitch for color operation on dest
		job.srcPitch = job.width * dest->getBytesPerPixel();
	}

	job.dstPitch = dest->getPitch();
	u32 dstPixelMul = dest->getBytesPerPixel();
	job.dst = (void*) ( (u8*) dest->getData() + ( job.Dest.y0 * job.dstPitch ) + ( job.Dest.x0 * dstPixelMul ) );

	blitter( &job );

	return 1;
}


// Methods for Software drivers
//! draws a rectangle
void drawRectangle(video::IImage* img, const core::rect<s32>& rect, const video::SColor &color)
{
	Blit(color.getAlpha() == 0xFF ? BLITTER_COLOR : BLITTER_COLOR_ALPHA,
			img, 0, &rect.UpperLeftCorner, 0, &rect, 0, &color,1);
}


//! draws a line from to with color
void drawLine(video::IImage* img, const core::position2d<s32>& from,
					const core::position2d<s32>& to, const video::SColor &color)
{
	AbsRectangle clip;
	GetClip(clip, img);

	core::position2d<s32> p[2];
	if (ClipLine( clip, p[0], p[1], from, to))
	{
		u32 alpha = extractAlpha(color.color);

		switch(img->getColorFormat())
		{
		case video::ECF_A1R5G5B5:
				if (alpha == 256)
				{
					RenderLine16_Decal(img, p[0], p[1], video::A8R8G8B8toA1R5G5B5(color.color));
				}
				else
				{
					RenderLine16_Blend(img, p[0], p[1], video::A8R8G8B8toA1R5G5B5(color.color), alpha >> 3);
				}
				break;
		case video::ECF_A8R8G8B8:
				if (alpha == 256)
				{
					RenderLine32_Decal(img, p[0], p[1], color.color);
				}
				else
				{
					RenderLine32_Blend(img, p[0], p[1], color.color, alpha);
				}
				break;
		default:
				break;
		}
	}
}



static const float srgb_8bit_to_linear_float[1 << 8] = {
	0.0f, 3.03527e-4f, 6.07054e-4f, 9.10581e-4f,
	0.001214108f, 0.001517635f, 0.001821162f, 0.0021246888f,
	0.002428216f, 0.002731743f, 0.00303527f, 0.0033465358f,
	0.0036765074f, 0.004024717f, 0.004391442f, 0.0047769537f,
	0.005181517f, 0.005605392f, 0.0060488335f, 0.006512091f,
	0.0069954107f, 0.007499032f, 0.008023193f, 0.008568126f,
	0.009134059f, 0.009721218f, 0.010329823f, 0.010960095f,
	0.011612245f, 0.012286489f, 0.0129830325f, 0.013702083f,
	0.014443845f, 0.015208516f, 0.015996294f, 0.016807377f,
	0.017641956f, 0.018500222f, 0.019382363f, 0.020288564f,
	0.021219011f, 0.022173885f, 0.023153368f, 0.024157634f,
	0.025186861f, 0.026241222f, 0.027320893f, 0.02842604f,
	0.029556835f, 0.030713445f, 0.031896032f, 0.033104766f,
	0.034339808f, 0.035601314f, 0.036889452f, 0.038204372f,
	0.039546236f, 0.0409152f, 0.04231141f, 0.04373503f,
	0.045186203f, 0.046665087f, 0.048171826f, 0.049706567f,
	0.051269464f, 0.05286065f, 0.05448028f, 0.056128494f,
	0.057805438f, 0.059511244f, 0.06124606f, 0.06301002f,
	0.06480327f, 0.066625945f, 0.068478175f, 0.0703601f,
	0.07227185f, 0.07421357f, 0.07618539f, 0.07818743f,
	0.08021983f, 0.082282715f, 0.084376216f, 0.086500466f,
	0.08865559f, 0.09084172f, 0.093058966f, 0.09530747f,
	0.097587354f, 0.09989873f, 0.10224174f, 0.10461649f,
	0.107023105f, 0.10946172f, 0.111932434f, 0.11443538f,
	0.11697067f, 0.119538434f, 0.122138776f, 0.12477182f,
	0.12743768f, 0.13013647f, 0.13286832f, 0.13563333f,
	0.13843162f, 0.14126329f, 0.14412847f, 0.14702727f,
	0.14995979f, 0.15292616f, 0.15592647f, 0.15896083f,
	0.16202939f, 0.1651322f, 0.1682694f, 0.17144111f,
	0.1746474f, 0.17788842f, 0.18116425f, 0.18447499f,
	0.18782078f, 0.19120169f, 0.19461784f, 0.19806932f,
	0.20155625f, 0.20507874f, 0.20863687f, 0.21223076f,
	0.21586053f, 0.21952623f, 0.22322798f, 0.2269659f,
	0.23074007f, 0.23455061f, 0.2383976f, 0.24228115f,
	0.24620135f, 0.2501583f, 0.25415212f, 0.25818288f,
	0.2622507f, 0.26635563f, 0.27049783f, 0.27467734f,
	0.2788943f, 0.28314877f, 0.28744087f, 0.29177067f,
	0.2961383f, 0.3005438f, 0.30498734f, 0.30946895f,
	0.31398875f, 0.3185468f, 0.32314324f, 0.32777813f,
	0.33245155f, 0.33716366f, 0.34191445f, 0.3467041f,
	0.35153264f, 0.35640016f, 0.36130682f, 0.36625263f,
	0.3712377f, 0.37626216f, 0.38132605f, 0.38642946f,
	0.3915725f, 0.39675525f, 0.4019778f, 0.40724024f,
	0.41254264f, 0.4178851f, 0.4232677f, 0.42869052f,
	0.43415368f, 0.4396572f, 0.44520122f, 0.45078582f,
	0.45641103f, 0.46207702f, 0.4677838f, 0.4735315f,
	0.4793202f, 0.48514995f, 0.4910209f, 0.496933f,
	0.5028865f, 0.50888133f, 0.5149177f, 0.5209956f,
	0.52711517f, 0.53327644f, 0.5394795f, 0.5457245f,
	0.55201143f, 0.55834043f, 0.5647115f, 0.57112485f,
	0.57758045f, 0.58407843f, 0.59061885f, 0.5972018f,
	0.60382736f, 0.61049557f, 0.6172066f, 0.62396044f,
	0.63075715f, 0.6375969f, 0.6444797f, 0.65140563f,
	0.65837485f, 0.66538733f, 0.67244315f, 0.6795425f,
	0.6866853f, 0.6938718f, 0.7011019f, 0.7083758f,
	0.71569353f, 0.7230551f, 0.73046076f, 0.73791045f,
	0.74540424f, 0.7529422f, 0.7605245f, 0.76815116f,
	0.7758222f, 0.7835378f, 0.791298f, 0.7991027f,
	0.8069523f, 0.8148466f, 0.82278574f, 0.8307699f,
	0.838799f, 0.8468732f, 0.8549926f, 0.8631572f,
	0.8713671f, 0.8796224f, 0.8879231f, 0.8962694f,
	0.9046612f, 0.91309863f, 0.92158186f, 0.9301109f,
	0.9386857f, 0.9473065f, 0.9559733f, 0.9646863f,
	0.9734453f, 0.9822506f, 0.9911021f, 1.0f,
};
/*
int linear_to_srgb_8bit(const float x) {
	if (x <= 0.f) return 0;
	if (x >= 1.f) return 255;
	const float *table = SRGB_8BIT_TO_LINEAR_FLOAT;
	int y = 0;
	for (int i = 128; i != 0; i >>= 1) {
		if (table[y + i] <= x)
			y += i;
	}
	if (x - table[y] <= table[y + 1] - x)
		return y;
	else
		return y + 1;
}
*/


u32 linear_to_srgb_8bit(const float v)
{
	ieee754 c;
	c.f = v;
	const register size_t x = c.u;
	const u32 *table = (u32*)srgb_8bit_to_linear_float;
	register u32 y = 0;
	y += table[y + 128] <= x ? 128 : 0;
	y += table[y + 64] <= x ? 64 : 0;
	y += table[y + 32] <= x ? 32 : 0;
	y += table[y + 16] <= x ? 16 : 0;
	y += table[y + 8] <= x ? 8 : 0;
	y += table[y + 4] <= x ? 4 : 0;
	y += table[y + 2] <= x ? 2 : 0;
	y += table[y + 1] <= x ? 1 : 0;

	return y;
}

// 2D Region half open [x0;x1[
struct absrect2
{
	s32 x0;
	s32 y0;
	s32 x1;
	s32 y1;
};

static inline int clipTest(absrect2 &o, const core::rect<s32>* a, const absrect2& b)
{
	if (a == 0)
	{
		o.x0 = b.x0;
		o.y0 = b.y0;
		o.x1 = b.x1;
		o.y1 = b.y1;
	}
	else
	{
		o.x0 = core::s32_max(a->UpperLeftCorner.X, b.x0);
		o.x1 = core::s32_min(a->LowerRightCorner.X, b.x1);
		o.y0 = core::s32_max(a->UpperLeftCorner.Y, b.y0);
		o.y1 = core::s32_min(a->LowerRightCorner.Y, b.y1);
	}
	int clipTest = 0;
	clipTest |= o.x0 >= o.x1 ? 1 : 0;
	clipTest |= o.y0 >= o.y1 ? 2 : 0;
	return clipTest;
}

//! stretches srcRect src to dstRect dst, applying a sliding window box filter in linear color space (sRGB->linear->sRGB)
// todo: texture jumps (mip selection problem)
void Resample_subSampling(eBlitter op, video::IImage* dst, const core::rect<s32>* dstRect,
	const video::IImage* src, const core::rect<s32>* srcRect)
{
	const absrect2 dst_clip = { 0,0,(s32)dst->getDimension().Width,(s32)dst->getDimension().Height };
	absrect2 dc;
	if (clipTest(dc, dstRect, dst_clip)) return;
	const video::ECOLOR_FORMAT dstFormat = dst->getColorFormat();
	const int dst_sRGB = dst->get_sRGB();
	u8* dstData = (u8*)dst->getData();

	const absrect2 src_clip = { 0,0,(s32)src->getDimension().Width,(s32)src->getDimension().Height };
	absrect2 sc;
	if (clipTest(sc, srcRect, src_clip)) return;
	const video::ECOLOR_FORMAT srcFormat = src->getColorFormat();
	const u8* srcData = (u8*)src->getData();
	const int src_sRGB = src->get_sRGB();


	float scale[2];
	scale[0] = (float)(sc.x1 - sc.x0) / (float)(dc.x1 - dc.x0);
	scale[1] = (float)(sc.y1 - sc.y0) / (float)(dc.y1 - dc.y0);
	const float rs = 1.f / (scale[0] * scale[1]);

	float sum[4];
	u32 sbgra = 0;

	float f[4];
	int fi[4];
	f[3] = (float)sc.y0;
	for (int dy = dc.y0; dy < dc.y1; ++dy)
	{
		f[1] = f[3];
		f[3] = sc.y0 + (dy + 1 - dc.y0)*scale[1];
		if (f[3] >= sc.y1) f[3] = sc.y1 - 0.001f; //todo:1.f/dim should be enough

		f[2] = (float)sc.x0;
		for (int dx = dc.x0; dx < dc.x1; ++dx)
		{
			f[0] = f[2];
			f[2] = sc.x0 + (dx + 1 - dc.x0)*scale[0];
			if (f[2] >= sc.x1) f[2] = sc.x1 - 0.001f;

			//accumulate linear color
			sum[0] = 0.f;
			sum[1] = 0.f;
			sum[2] = 0.f;
			sum[3] = 0.f;

			//sample border
			fi[0] = (int)(f[0]);
			fi[1] = (int)(f[1]);
			fi[2] = (int)(f[2]);
			fi[3] = (int)(f[3]);

			float w[2];
			for (int fy = fi[1]; fy <= fi[3]; ++fy)
			{
				w[1] = 1.f;
				if (fy == fi[1]) w[1] -= f[1] - fy;
				if (fy == fi[3]) w[1] -= fy + 1 - f[3];

				for (int fx = fi[0]; fx <= fi[2]; ++fx)
				{
					w[0] = 1.f;
					if (fx == fi[0]) w[0] -= f[0] - fx;
					if (fx == fi[2]) w[0] -= fx + 1 - f[2];

					const float ws = w[1] * w[0] * rs;

					switch (srcFormat)
					{
					case video::ECF_A1R5G5B5: sbgra = video::A1R5G5B5toA8R8G8B8(*(u16*)(srcData + (fy*src_clip.x1) * 2 + (fx * 2))); break;
					case video::ECF_R5G6B5: sbgra = video::R5G6B5toA8R8G8B8(*(u16*)(srcData + (fy*src_clip.x1) * 2 + (fx * 2))); break;
					case video::ECF_A8R8G8B8: sbgra = *(u32*)(srcData + (fy*src_clip.x1) * 4 + (fx * 4)); break;
					case video::ECF_R8G8B8:
					{
						const u8* p = srcData + (fy*src_clip.x1) * 3 + (fx * 3);
						sbgra = 0xFF000000 | p[0] << 16 | p[1] << 8 | p[2];
					} break;
					default: break;
					}
					if (src_sRGB)
					{
						sum[0] += srgb_8bit_to_linear_float[(sbgra) & 0xFF] * ws;
						sum[1] += srgb_8bit_to_linear_float[(sbgra >> 8) & 0xFF] * ws;
						sum[2] += srgb_8bit_to_linear_float[(sbgra >> 16) & 0xFF] * ws;
						sum[3] += ((sbgra >> 24) & 0xFF) * ws;
					}
					else
					{
						sum[0] += ((sbgra) & 0xFF) * ws;
						sum[1] += ((sbgra >> 8) & 0xFF) * ws;
						sum[2] += ((sbgra >> 16) & 0xFF) * ws;
						sum[3] += ((sbgra >> 24) & 0xFF) * ws;
					}

				}
			}
			switch (op)
			{
			case BLITTER_TEXTURE_ALPHA_BLEND:
			case BLITTER_TEXTURE_ALPHA_COLOR_BLEND:
				break;
			default:
				break;
			}
			if (dst_sRGB)
			{
				sbgra = linear_to_srgb_8bit(sum[0]) |
					linear_to_srgb_8bit(sum[1]) << 8 |
					linear_to_srgb_8bit(sum[2]) << 16 |
					(u32)(sum[3]) << 24;
			}
			else
			{
				sbgra = (u32)(sum[0]) |
					(u32)(sum[1]) << 8 |
					(u32)(sum[2]) << 16 |
					(u32)(sum[3]) << 24;
			}
			switch (dstFormat)
			{
			case video::ECF_A8R8G8B8: *(u32*)(dstData + (dy*dst_clip.x1) * 4 + (dx * 4)) = sbgra; break;
			case video::ECF_R8G8B8:
			{
				u8* p = dstData + (dy*dst_clip.x1) * 3 + (dx * 3);
				p[2] = (sbgra) & 0xFF;
				p[1] = (sbgra >> 8) & 0xFF;
				p[0] = (sbgra >> 16) & 0xFF;
			} break;
			case video::ECF_A1R5G5B5: *(u16*)(dstData + (dy*dst_clip.x1) * 2 + (dx * 2)) = video::A8R8G8B8toA1R5G5B5(sbgra); break;
			case video::ECF_R5G6B5:   *(u16*)(dstData + (dy*dst_clip.x1) * 2 + (dx * 2)) = video::A8R8G8B8toR5G6B5(sbgra); break;
			default:
				break;
			}
		}
	}

}

} // end namespace video
} // end namespace irr



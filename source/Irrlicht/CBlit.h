// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef _C_BLIT_H_INCLUDED_
#define _C_BLIT_H_INCLUDED_

namespace irr
{
namespace video
{

	// Blitter Operation
	enum eBlitter
	{
		BLITTER_INVALID = 0,
		BLITTER_COLOR,
		BLITTER_COLOR_ALPHA,
		BLITTER_TEXTURE,
		BLITTER_TEXTURE_ALPHA_BLEND,
		BLITTER_TEXTURE_ALPHA_COLOR_BLEND,
		BLITTER_TEXTURE_COMBINE_ALPHA,
	};

	//! stretches srcRect src to dstRect dst, applying a sliding window box filter in linear color space (sRGB->linear->sRGB)
	void Resample_subSampling(eBlitter op, video::IImage* dst, const core::rect<s32>* dstRect, const video::IImage* src, const core::rect<s32>* srcRect);

	/*!
		a generic 2D Blitter
	*/
	s32 Blit(eBlitter operation,
		video::IImage* dest, const core::rect<s32>* destClipping, const core::position2d<s32>* destPos,
		const video::IImage* source, const core::rect<s32>* sourceClipping, const core::dimension2d<u32>* src_originalSize,
		const video::SColor* color, u32 color_size);

	s32 StretchBlit(eBlitter operation,
		video::IImage* dest, const core::rect<s32>* destClipping, const core::rect<s32> *destRect,
		const video::IImage* source, const core::rect<s32> *srcRect, const core::dimension2d<u32>* src_originalSize,
		const video::SColor* color, u32 color_size);

	// Methods for Software drivers
	//! draws a rectangle
	void drawRectangle(video::IImage* img, const core::rect<s32>& rect, const video::SColor &color);

	//! draws a line from to with color
	void drawLine(video::IImage* img, const core::position2d<s32>& from,
		const core::position2d<s32>& to, const video::SColor &color);

	/*
		return alpha in [0;256] Granularity from 32-Bit ARGB
		add highbit alpha ( alpha > 127 ? + 1 )
	*/
	static inline u32 extractAlpha(const u32 c)
	{
		return (c >> 24) + (c >> 31);
	}


} // end namespace video
} // end namespace irr

#endif


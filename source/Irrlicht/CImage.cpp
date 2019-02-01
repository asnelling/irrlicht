// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImage.h"
#include "irrString.h"
#include "CColorConverter.h"
#include "CBlit.h"
#include "os.h"

namespace irr
{
namespace video
{

//! Constructor from raw data
CImage::CImage(ECOLOR_FORMAT format, const core::dimension2d<u32>& size, void* data,
	bool ownForeignMemory, bool deleteMemory) : IImage(format, size, deleteMemory)
{
	if (ownForeignMemory)
	{
		Data = (u8*)data;
	}
	else
	{
		const u32 dataSize = getDataSizeFromFormat(Format, Size.Width, Size.Height);

		Data = new u8[dataSize];
		memcpy(Data, data, dataSize);
		DeleteMemory = true;
	}
}


//! Constructor of empty image
CImage::CImage(ECOLOR_FORMAT format, const core::dimension2d<u32>& size) : IImage(format, size, true)
{
	Data = new u8[getDataSizeFromFormat(Format, Size.Width, Size.Height)];
	DeleteMemory = true;
}


//! sets a pixel
void CImage::setPixel(u32 x, u32 y, const SColor &color, bool blend)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::setPixel method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	if (x >= Size.Width || y >= Size.Height)
		return;

	switch(Format)
	{
		case ECF_A1R5G5B5:
		{
			u16 * dest = (u16*) (Data + ( y * Pitch ) + ( x << 1 ));
			*dest = video::A8R8G8B8toA1R5G5B5( color.color );
		} break;

		case ECF_R5G6B5:
		{
			u16 * dest = (u16*) (Data + ( y * Pitch ) + ( x << 1 ));
			*dest = video::A8R8G8B8toR5G6B5( color.color );
		} break;

		case ECF_R8G8B8:
		{
			u8* dest = Data + ( y * Pitch ) + ( x * 3 );
			dest[0] = (u8)color.getRed();
			dest[1] = (u8)color.getGreen();
			dest[2] = (u8)color.getBlue();
		} break;

		case ECF_A8R8G8B8:
		{
			u32 * dest = (u32*) (Data + ( y * Pitch ) + ( x << 2 ));
			*dest = blend ? PixelBlend32 ( *dest, color.color ) : color.color;
		} break;
#ifndef _DEBUG
		default:
			break;
#endif
	}
}


//! returns a pixel
SColor CImage::getPixel(u32 x, u32 y) const
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::getPixel method doesn't work with compressed images.", ELL_WARNING);
		return SColor(0);
	}

	if (x >= Size.Width || y >= Size.Height)
		return SColor(0);

	switch(Format)
	{
	case ECF_A1R5G5B5:
		return A1R5G5B5toA8R8G8B8(((u16*)Data)[y*Size.Width + x]);
	case ECF_R5G6B5:
		return R5G6B5toA8R8G8B8(((u16*)Data)[y*Size.Width + x]);
	case ECF_A8R8G8B8:
		return ((u32*)Data)[y*Size.Width + x];
	case ECF_R8G8B8:
		{
			u8* p = Data+(y*3)*Size.Width + (x*3);
			return SColor(255,p[0],p[1],p[2]);
		}
#ifndef _DEBUG
	default:
		break;
#endif
	}

	return SColor(0);
}


//! copies this surface into another at given position
void CImage::copyTo(IImage* target, const core::position2d<s32>& pos)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyTo method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	Blit(BLITTER_TEXTURE, target, 0, &pos, this, 0, 0, 0);
}


//! copies this surface partially into another at given position
void CImage::copyTo(IImage* target, const core::position2d<s32>& pos, const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyTo method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	Blit(BLITTER_TEXTURE, target, clipRect, &pos, this, &sourceRect, 0, 0);
}


//! copies this surface into another, using the alpha mask, a cliprect and a color to add with
void CImage::copyToWithAlpha(IImage* target, const core::position2d<s32>& pos, const core::rect<s32>& sourceRect, const SColor &color, const core::rect<s32>* clipRect, bool combineAlpha)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyToWithAlpha method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	if ( combineAlpha )
	{
		Blit(BLITTER_TEXTURE_COMBINE_ALPHA, target, clipRect, &pos, this, &sourceRect, 0, color.color);
	}
	else
	{
		// color blend only necessary on not full spectrum aka. color.color != 0xFFFFFFFF
		Blit(color.color == 0xFFFFFFFF ? BLITTER_TEXTURE_ALPHA_BLEND: BLITTER_TEXTURE_ALPHA_COLOR_BLEND,
				target, clipRect, &pos, this, &sourceRect, 0, color.color);
	}
}


//! copies this surface into another, scaling it to the target image size
// note: this is very very slow.
void CImage::copyToScaling(void* target, u32 width, u32 height, ECOLOR_FORMAT format, u32 pitch)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyToScaling method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	if (!target || !width || !height)
		return;

	const u32 bpp=getBitsPerPixelFromFormat(format)/8;
	if (0==pitch)
		pitch = width*bpp;

	if (Format==format && Size.Width==width && Size.Height==height)
	{
		if (pitch==Pitch)
		{
			memcpy(target, Data, height*pitch);
			return;
		}
		else
		{
			u8* tgtpos = (u8*) target;
			u8* srcpos = Data;
			const u32 bwidth = width*bpp;
			const u32 rest = pitch-bwidth;
			for (u32 y=0; y<height; ++y)
			{
				// copy scanline
				memcpy(tgtpos, srcpos, bwidth);
				// clear pitch
				memset(tgtpos+bwidth, 0, rest);
				tgtpos += pitch;
				srcpos += Pitch;
			}
			return;
		}
	}

	const f32 sourceXStep = (f32)Size.Width / (f32)width;
	const f32 sourceYStep = (f32)Size.Height / (f32)height;
	s32 yval=0, syval=0;
	f32 sy = 0.0f;
	for (u32 y=0; y<height; ++y)
	{
		f32 sx = 0.0f;
		for (u32 x=0; x<width; ++x)
		{
			CColorConverter::convert_viaFormat(Data+ syval + ((s32)sx)*BytesPerPixel, Format, 1, ((u8*)target)+ yval + (x*bpp), format);
			sx+=sourceXStep;
		}
		sy+=sourceYStep;
		syval=((s32)sy)*Pitch;
		yval+=pitch;
	}
}


//! copies this surface into another, scaling it to the target image size
// note: this is very very slow.
void CImage::copyToScaling(IImage* target)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyToScaling method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	if (!target)
		return;

	const core::dimension2d<u32>& targetSize = target->getDimension();

	if (targetSize==Size)
	{
		copyTo(target);
		return;
	}

	copyToScaling(target->getData(), targetSize.Width, targetSize.Height, target->getColorFormat());
}


//! copies this surface into another, scaling it to fit it.
void CImage::copyToScalingBoxFilter(IImage* target, s32 bias, bool blend)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::copyToScalingBoxFilter method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	const core::dimension2d<u32> destSize = target->getDimension();

	const f32 sourceXStep = (f32) Size.Width / (f32) destSize.Width;
	const f32 sourceYStep = (f32) Size.Height / (f32) destSize.Height;

	target->getData();

	s32 fx = core::ceil32( sourceXStep );
	s32 fy = core::ceil32( sourceYStep );
	f32 sx;
	f32 sy;

	sy = 0.f;
	for ( u32 y = 0; y != destSize.Height; ++y )
	{
		sx = 0.f;
		for ( u32 x = 0; x != destSize.Width; ++x )
		{
			target->setPixel( x, y,
				getPixelBox( core::floor32(sx), core::floor32(sy), fx, fy, bias ), blend );
			sx += sourceXStep;
		}
		sy += sourceYStep;
	}
}


//! fills the surface with given color
void CImage::fill(const SColor &color)
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::fill method doesn't work with compressed images.", ELL_WARNING);
		return;
	}

	u32 c;

	switch ( Format )
	{
		case ECF_A1R5G5B5:
			c = color.toA1R5G5B5();
			c |= c << 16;
			break;
		case ECF_R5G6B5:
			c = video::A8R8G8B8toR5G6B5( color.color );
			c |= c << 16;
			break;
		case ECF_A8R8G8B8:
			c = color.color;
			break;
		case ECF_R8G8B8:
		{
			u8 rgb[3];
			CColorConverter::convert_A8R8G8B8toR8G8B8(&color, 1, rgb);
			const u32 size = getImageDataSizeInBytes();
			for (u32 i=0; i<size; i+=3)
			{
				memcpy(Data+i, rgb, 3);
			}
			return;
		}
		break;
		default:
		// TODO: Handle other formats
			return;
	}
	memset32( Data, c, getImageDataSizeInBytes() );
}


//! get a filtered pixel
inline SColor CImage::getPixelBox( s32 x, s32 y, s32 fx, s32 fy, s32 bias ) const
{
	if (IImage::isCompressedFormat(Format))
	{
		os::Printer::log("IImage::getPixelBox method doesn't work with compressed images.", ELL_WARNING);
		return SColor(0);
	}

	SColor c;
	s32 a = 0, r = 0, g = 0, b = 0;

	for ( s32 dx = 0; dx != fx; ++dx )
	{
		for ( s32 dy = 0; dy != fy; ++dy )
		{
			c = getPixel(	core::s32_min ( x + dx, Size.Width - 1 ) ,
							core::s32_min ( y + dy, Size.Height - 1 )
						);

			a += c.getAlpha();
			r += c.getRed();
			g += c.getGreen();
			b += c.getBlue();
		}

	}

	s32 sdiv = s32_log2_s32(fx * fy);

	a = core::s32_clamp( ( a >> sdiv ) + bias, 0, 255 );
	r = core::s32_clamp( ( r >> sdiv ) + bias, 0, 255 );
	g = core::s32_clamp( ( g >> sdiv ) + bias, 0, 255 );
	b = core::s32_clamp( ( b >> sdiv ) + bias, 0, 255 );

	c.set( a, r, g, b );
	return c;
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


unsigned linear_to_srgb_8bit(const float v)
{
	core::inttofloat c;
	c.f = v;
	const register unsigned x = c.u;
	const unsigned *table = (unsigned*)srgb_8bit_to_linear_float;
	register unsigned y = 0;
	y += table[y + 128] <= x ? 128 : 0;
	y += table[y +  64] <= x ?  64 : 0;
	y += table[y +  32] <= x ?  32 : 0;
	y += table[y +  16] <= x ?  16 : 0;
	y += table[y +   8] <= x ?   8 : 0;
	y += table[y +   4] <= x ?   4 : 0;
	y += table[y +   2] <= x ?   2 : 0;
	y += table[y +   1] <= x ?   1 : 0;

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

static inline int clipTest ( absrect2 &o, const core::rect<s32>* a, const absrect2& b)
{
	if ( a == 0 )
	{
		o.x0 = b.x0;
		o.y0 = b.y0;
		o.x1 = b.x1;
		o.y1 = b.y1;
	}
	else
	{
		o.x0 = core::s32_max( a->UpperLeftCorner.X, b.x0 );
		o.x1 = core::s32_min( a->LowerRightCorner.X, b.x1 );
		o.y0 = core::s32_max( a->UpperLeftCorner.Y, b.y0 );
		o.y1 = core::s32_min( a->LowerRightCorner.Y, b.y1 );
	}
	int clipTest = 0;
	clipTest |= o.x0 >= o.x1 ? 1 : 0;
	clipTest |= o.y0 >= o.y1 ? 2 : 0;
	return clipTest;
}

//! stretches srcRect src to dstRect dst, applying a sliding window box filter in linear color space (sRGB->linear->sRGB)
void Resample_subSampling(eBlitter op,video::IImage* dst,const core::rect<s32>* dstRect,const video::IImage* src,const core::rect<s32>* srcRect)
{
	const absrect2 dst_clip = {0,0,(s32)dst->getDimension().Width,(s32)dst->getDimension().Height};
	absrect2 dc;
	if (clipTest(dc,dstRect,dst_clip)) return;
	const video::ECOLOR_FORMAT dstFormat = dst->getColorFormat();
	u8* dstData= (u8*)dst->getData();

	const absrect2 src_clip = {0,0,(s32)src->getDimension().Width,(s32)src->getDimension().Height};
	absrect2 sc;
	if (clipTest(sc,srcRect,src_clip)) return;
	const video::ECOLOR_FORMAT srcFormat = src->getColorFormat();
	const u8* srcData= (u8*)src->getData();

	float scale[2];
	scale[0] = (float)(sc.x1-sc.x0) / (float)(dc.x1-dc.x0);
	scale[1] = (float)(sc.y1-sc.y0) / (float)(dc.y1-dc.y0);
	const float rs = 1.f/(scale[0]*scale[1]);

	float sum[4];
	u32 sbgra;

	float f[4];
	int fi[4];
	f[3]= (float)sc.y0;
	for ( int dy = dc.y0; dy < dc.y1; ++dy )
	{
		f[1] = f[3];
		f[3] = sc.y0 + (dy+1-dc.y0)*scale[1];
		if ( f[3] >= sc.y1) f[3] = sc.y1 - 0.1f; //todo:1.f/dim should be enough

		f[2]= (float)sc.x0;
		for ( int dx = dc.x0; dx < dc.x1; ++dx )
		{
			f[0] = f[2];
			f[2] = sc.x0 + (dx+1-dc.x0)*scale[0];
			if ( f[2] >= sc.x1) f[2] = sc.x1 - 0.1f;

			//accumulate linear color
			sum[0] = 0.f;
			sum[1] = 0.f;
			sum[2] = 0.f;
			sum[3] = 0.f;

			//sample border
			fi[0]=(int)(f[0]);
			fi[1]=(int)(f[1]);
			fi[2]=(int)(f[2]);
			fi[3]=(int)(f[3]);

			float w[2];
			for ( int fy = fi[1]; fy <= fi[3]; ++fy)
			{
				w[1] = 1.f;
				if ( fy == fi[1]) w[1] -= f[1]-fy;
				if ( fy == fi[3]) w[1] -= fy+1 - f[3];

				for ( int fx = fi[0]; fx <= fi[2]; ++fx)
				{
					w[0] = 1.f;
					if ( fx == fi[0]) w[0] -= f[0]-fx;
					if ( fx == fi[2]) w[0] -= fx+1 - f[2];

					const float ws = w[1]*w[0]*rs;

					switch(srcFormat)
					{
					case video::ECF_A1R5G5B5: sbgra = video::A1R5G5B5toA8R8G8B8(*(u16*)(srcData+(fy*src_clip.x1)*2 + (fx*2))); break;
					case video::ECF_R5G6B5  : sbgra = video::R5G6B5toA8R8G8B8  (*(u16*)(srcData+(fy*src_clip.x1)*2 + (fx*2))); break;
					case video::ECF_A8R8G8B8: sbgra =  *(u32*)(srcData+(fy*src_clip.x1)*4 + (fx*4)); break;
					case video::ECF_R8G8B8:
					{	
						const u8* p = srcData+(fy*src_clip.x1)*3 + (fx*3);
						sbgra = 0xFF000000 | p[0] << 16 | p[1] << 8 | p[2];
					} break;
					default: break;
					}
					sum[0] += srgb_8bit_to_linear_float[(sbgra    )&0xFF] * ws;
					sum[1] += srgb_8bit_to_linear_float[(sbgra>>8 )&0xFF] * ws;
					sum[2] += srgb_8bit_to_linear_float[(sbgra>>16)&0xFF] * ws;
					sum[3] += ((sbgra>>24)&0xFF)*ws;

				}
			}
			switch(op)
			{
				case BLITTER_TEXTURE_ALPHA_BLEND:
				case BLITTER_TEXTURE_ALPHA_COLOR_BLEND:
					break;
			}
			sbgra = linear_to_srgb_8bit(sum[0])       |
					linear_to_srgb_8bit(sum[1]) <<  8 |
					linear_to_srgb_8bit(sum[2]) << 16 |
					(u32) (sum[3]) << 24;
			switch(dstFormat)
			{
				case video::ECF_A8R8G8B8: *(u32*)(dstData+(dy*dst_clip.x1)*4 + (dx*4)) = sbgra; break;
				case video::ECF_R8G8B8:
				{	
					u8* p = dstData+(dy*dst_clip.x1)*3 + (dx*3);
					p[2] = (sbgra    )&0xFF;
					p[1] = (sbgra>>8 )&0xFF;
					p[0] = (sbgra>>16)&0xFF;
				} break;
				case video::ECF_A1R5G5B5: *(u16*)(dstData+(dy*dst_clip.x1)*2 + (dx*2)) = video::A8R8G8B8toA1R5G5B5(sbgra); break;
				case video::ECF_R5G6B5:   *(u16*)(dstData+(dy*dst_clip.x1)*2 + (dx*2)) = video::A8R8G8B8toR5G6B5(sbgra); break;
			}
		}
	}

}


} // end namespace video
} // end namespace irr

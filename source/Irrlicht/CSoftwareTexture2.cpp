// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_compile_config.h"
#include "SoftwareDriver2_helper.h"
#include "CSoftwareTexture2.h"
#include "CSoftwareDriver2.h"
#include "CBlit.h"
#include "os.h"

namespace irr
{
namespace video
{

//! constructor
CSoftwareTexture2::CSoftwareTexture2(IImage* image, const io::path& name, u32 flags, CBurningVideoDriver* driver)
	: ITexture(name, ETT_2D),MipMapLOD(0), Flags ( flags ), OriginalFormat(video::ECF_UNKNOWN),Driver(driver)
{
	#ifdef _DEBUG
	setDebugName("CSoftwareTexture2");
	#endif

#ifndef SOFTWARE_DRIVER_2_MIPMAPPING
	Flags &= ~GEN_MIPMAP;
#endif

	DriverType = EDT_BURNINGSVIDEO;
	ColorFormat = BURNINGSHADER_COLOR_FORMAT;
	IsRenderTarget = (Flags & IS_RENDERTARGET) != 0;
	HasMipMaps = (Flags & GEN_MIPMAP) != 0;
	MipMap0_Area[0] = 1;
	MipMap0_Area[1] = 1;
	for ( size_t i = 0; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i ) MipMap[i] = 0;
	if (!image) return;

	OriginalSize = image->getDimension();
	OriginalFormat = image->getColorFormat();

	if ( Flags & IMAGE_IS_LINEAR ) image->set_sRGB(0);

	bool isCompressed = IImage::isCompressedFormat(OriginalFormat);
	if (isCompressed)
	{
		os::Printer::log("Texture compression not available.", ELL_ERROR);
	}

	core::dimension2d<u32> optSize(	OriginalSize.getOptimalSize(
			(Flags & ALLOW_NPOT) ? 0 : 1, // requirePowerOfTwo
			false, // requireSquare
			(Flags & ALLOW_NPOT) ? 1 : SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE == 0, // larger
			(Flags & ALLOW_NPOT) ? 0 : SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE // maxValue
		)
	);

	if (OriginalSize == optSize)
	{
		MipMap[0] = new CImage(BURNINGSHADER_COLOR_FORMAT, image->getDimension());
		MipMap[0]->set_sRGB( (Flags & TEXTURE_IS_LINEAR ) ? 0 : image->get_sRGB()  );
		
		if (!isCompressed)
			image->copyTo(MipMap[0]);
	}
	else
	{
		char buf[256];
		core::stringw showName ( name );
		snprintf_irr ( buf, sizeof(buf), "Burningvideo: Warning Texture %ls reformat %dx%d,%d -> %dx%d,%d",
						showName.c_str(),
						OriginalSize.Width, OriginalSize.Height,OriginalFormat,
						optSize.Width, optSize.Height,BURNINGSHADER_COLOR_FORMAT
					);

		os::Printer::log ( buf, ELL_WARNING );
		MipMap[0] = new CImage(BURNINGSHADER_COLOR_FORMAT, optSize);
		MipMap[0]->set_sRGB( (Flags & TEXTURE_IS_LINEAR ) ? 0 : image->get_sRGB()  );
		if (!isCompressed)
		{
			//image->copyToScalingBoxFilter ( MipMap[0],0, false );
			Resample_subSampling(BLITTER_TEXTURE,MipMap[0],0,image,0);
		}
		// if Original Size is used for calculation ( 2D position, font) it will be wrong
		//OriginalSize = optSize;
	}

	//select highest mipmap 0
	regenerateMipMapLevels(image->getMipMapsData());
}


//! destructor
CSoftwareTexture2::~CSoftwareTexture2()
{
	for ( size_t i = 0; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i )
	{
		if ( MipMap[i] )
		{
			MipMap[i]->drop();
			MipMap[i] = 0;
		}
	}
}



//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void CSoftwareTexture2::regenerateMipMapLevels(void* data, u32 layer)
{
	int i;

	// release
	for ( i = 1; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i )
	{
		if ( MipMap[i] )
		{
			MipMap[i]->drop();
			MipMap[i] = 0;
		}
	}

	core::dimension2d<u32> newSize;

	//deactivated outside mipdata until TA knows how to handle this.
	if (HasMipMaps && (0 == data || 1))
	{
		for (i = 1; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i)
		{
			const core::dimension2du& upperDim = MipMap[i - 1]->getDimension();
			//isotropic
			newSize.Width = core::s32_max(SOFTWARE_DRIVER_2_MIPMAPPING_MIN_SIZE, upperDim.Width >> 1);
			newSize.Height = core::s32_max(SOFTWARE_DRIVER_2_MIPMAPPING_MIN_SIZE, upperDim.Height >> 1);
			if (upperDim == newSize)
				break;

			MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);
			MipMap[i]->set_sRGB(MipMap[i - 1]->get_sRGB());
			//MipMap[i]->fill ( 0xFFFF4040 );
			//MipMap[i-1]->copyToScalingBoxFilter( MipMap[i], 0, false );
			Resample_subSampling(BLITTER_TEXTURE, MipMap[i], 0, MipMap[0], 0);
		}
	}
	else if (HasMipMaps && data)
	{
		core::dimension2d<u32> origSize = Size;

		for (i = 1; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i)
		{
			const core::dimension2du& upperDim = MipMap[i - 1]->getDimension();
			//isotropic
			newSize.Width = core::s32_max(SOFTWARE_DRIVER_2_MIPMAPPING_MIN_SIZE, upperDim.Width >> 1);
			newSize.Height = core::s32_max(SOFTWARE_DRIVER_2_MIPMAPPING_MIN_SIZE, upperDim.Height >> 1);
			if (upperDim == newSize)
				break;

			origSize.Width = core::s32_max(1, origSize.Width >> 1);
			origSize.Height = core::s32_max(1, origSize.Height >> 1);

			if (OriginalFormat != BURNINGSHADER_COLOR_FORMAT)
			{
				IImage* tmpImage = new CImage(OriginalFormat, origSize, data, true, false);
				MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);
				if (origSize == newSize)
					tmpImage->copyTo(MipMap[i]);
				else
					tmpImage->copyToScalingBoxFilter(MipMap[i]);
				tmpImage->drop();
			}
			else
			{
				if (origSize == newSize)
					MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize, data, false);
				else
				{
					MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);
					IImage* tmpImage = new CImage(BURNINGSHADER_COLOR_FORMAT, origSize, data, true, false);
					tmpImage->copyToScalingBoxFilter(MipMap[i]);
					tmpImage->drop();
				}
			}
			data = (u8*)data + origSize.getArea()*IImage::getBitsPerPixelFromFormat(OriginalFormat) / 8;
		}
	}


	//visualize mipmap
	for (i=1; i < 0 && i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i)
	{
/*
		static u32 color[] = { 
			0x30bf7f00,0x3040bf00,0x30bf00bf,0x3000bf00,
			0x300080bf,0x30bf4000,0x300040bf,0x307f00bf,
			0x30bf0000,0x3000bfbf,0x304000bf,0x307fbf00,
			0x8000bf7f,0x80bf0040,0x80bfbf00,0x800000bf
		};
*/
		static u32 color[] = { 
			0xFFFFFFFF,0xFFFF0000,0xFF00FF00,0xFF0000FF,
			0xFF0000FF,0xFF0000FF,0xFF0000FF,0xFF0000FF,
			0xFF0000FF,0xFF0000FF,0xFF0000FF,0xFF0000FF,
			0xFF0000FF,0xFF0000FF,0xFF0000FF,0xFFFF00FF
		};

		if ( MipMap[i] )
		{
			core::rect<s32> p (core::position2di(0,0),MipMap[i]->getDimension());
			SColor c((color[i & 15] & 0x00FFFFFF) | 0xFF000000);
			Blit(BLITTER_TEXTURE_ALPHA_COLOR_BLEND, MipMap[i], 0, 0, MipMap[i], &p, 0,&c,1);
		}
	}

	//save mipmap chain
	if ( 0 )
	{
		char buf[256];
		const char* name = getName().getPath().c_str();
		int filename = 0;
		//int ext = -1;
		i = 0;
		while (name[i])
		{
			if (name[i] == '/' || name[i] == '\\') filename = i + 1;
			//if (name[i] == '.') ext = i;
			i += 1;
		}
		for (i = 0; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i)
		{
			if (MipMap[i])
			{
				snprintf_irr(buf, sizeof(buf),"mip/%s_%02d.png", name + filename,i);
				Driver->writeImageToFile(MipMap[i], buf);
			}
		}
	}

	calcDerivative();
}

void CSoftwareTexture2::calcDerivative()
{
	//reset current MipMap
	MipMapLOD = 0;
	if (MipMap[0])
	{
		const core::dimension2du& dim = MipMap[0]->getDimension();
		MipMap0_Area[0] = dim.Width;
		MipMap0_Area[1] = dim.Height; // screensize of a triangle

		Size = dim; // MipMap[MipMapLOD]->getDimension();
		Pitch = MipMap[MipMapLOD]->getPitch();
	}

	//preCalc mipmap texel center boundaries
	for ( s32 i = 0; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i )
	{
		CSoftwareTexture2_Bound& b = TexBound[i];
		if (MipMap[i])
		{
			const core::dimension2du& dim = MipMap[i]->getDimension();
			//f32 u = 1.f / dim.Width;
			//f32 v = 1.f / dim.Height;

			b.w = dim.Width - 1.f;
			b.h = dim.Height - 1.f;
			b.cx = 0.f; //u*0.005f;
			b.cy = 0.f; //v*0.005f;
		}
		else
		{
			b.w = 0.f;
			b.h = 0.f;
			b.cx = 0.f;
			b.cy = 0.f;
		}
	}

}


/* Software Render Target 2 */

CSoftwareRenderTarget2::CSoftwareRenderTarget2(CBurningVideoDriver* driver) : Driver(driver)
{
	DriverType = EDT_BURNINGSVIDEO;

	Texture.set_used(1);
	Texture[0] = 0;
}

CSoftwareRenderTarget2::~CSoftwareRenderTarget2()
{
	if (Texture[0])
		Texture[0]->drop();
}

void CSoftwareRenderTarget2::setTexture(const core::array<ITexture*>& texture, ITexture* depthStencil, const core::array<E_CUBE_SURFACE>& cubeSurfaces)
{
	if (Texture != texture)
	{
		ITexture* prevTexture = Texture[0];

		bool textureDetected = false;

		for (u32 i = 0; i < texture.size(); ++i)
		{
			if (texture[i] && texture[i]->getDriverType() == EDT_BURNINGSVIDEO)
			{
				Texture[0] = texture[i];
				Texture[0]->grab();
				textureDetected = true;

				break;
			}
		}

		if (prevTexture)
			prevTexture->drop();

		if (!textureDetected)
			Texture[0] = 0;
	}
}

ITexture* CSoftwareRenderTarget2::getTexture() const
{
	return Texture[0];
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

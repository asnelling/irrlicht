// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "SoftwareDriver2_compile_config.h"
#include "CDepthBuffer.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

namespace irr
{
namespace video
{


//! constructor
CDepthBuffer::CDepthBuffer(const core::dimension2d<u32>& size)
: Buffer(0), Size(0,0)
{
	#ifdef _DEBUG
	setDebugName("CDepthBuffer");
	#endif

	setSize(size);
}



//! destructor
CDepthBuffer::~CDepthBuffer()
{
	if (Buffer)
	{
		delete[] Buffer;
		Buffer = 0;
	}
}



//! clears the zbuffer
void CDepthBuffer::clear()
{
	core::inttofloat zMaxValue;

#ifdef SOFTWARE_DRIVER_2_USE_WBUFFER
	zMaxValue.f = 0.f;
#else
	zMaxValue.f = 1.f;
#endif

	memset32 ( Buffer, zMaxValue.u, TotalSize );
}



//! sets the new size of the zbuffer
void CDepthBuffer::setSize(const core::dimension2d<u32>& size)
{
	if (size == Size)
		return;

	Size = size;

	delete [] Buffer;

	Pitch = size.Width * sizeof ( fp24 );
	TotalSize = Pitch * size.Height;
	Buffer = new u8[align_next(TotalSize,16)];
	clear ();
}



//! returns the size of the zbuffer
const core::dimension2d<u32>& CDepthBuffer::getSize() const
{
	return Size;
}

// -----------------------------------------------------------------

//! constructor
CStencilBuffer::CStencilBuffer(const core::dimension2d<u32>& size, unsigned bit)
: Buffer(0), Size(0,0),Bit(bit)
{
	#ifdef _DEBUG
	setDebugName("CStencilBuffer");
	#endif

	setSize(size);
}



//! destructor
CStencilBuffer::~CStencilBuffer()
{
	if (Buffer)
	{
		delete[] Buffer;
		Buffer = 0;
	}
}



//! clears the zbuffer
void CStencilBuffer::clear()
{
	memset32 ( Buffer, 0, TotalSize );
}



//! sets the new size of the zbuffer
void CStencilBuffer::setSize(const core::dimension2d<u32>& size)
{
	if (size == Size)
		return;

	Size = size;

	delete [] Buffer;

	Pitch = size.Width * sizeof (tStencilSample);
	TotalSize = Pitch * size.Height;
	Buffer = new u8[align_next(TotalSize,16)];
	clear ();
}



//! returns the size of the zbuffer
const core::dimension2d<u32>& CStencilBuffer::getSize() const
{
	return Size;
}



} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

namespace irr
{
namespace video
{

//! creates a ZBuffer
IDepthBuffer* createDepthBuffer(const core::dimension2d<u32>& size)
{
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
	return new CDepthBuffer(size);
	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}


//! creates a Stencil Buffer
IStencilBuffer* createStencilBuffer(const core::dimension2d<u32>& size, u32 bit)
{
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
	return new CStencilBuffer(size,bit);
	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}

} // end namespace video
} // end namespace irr




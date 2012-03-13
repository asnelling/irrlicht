// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_INDEX_BUFFER_H_INCLUDED__
#define __I_INDEX_BUFFER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrArray.h"
#include "EHardwareBufferFlags.h"

namespace irr
{
namespace video
{
	enum E_INDEX_TYPE
	{
		EIT_16BIT = 0,
		EIT_32BIT
	};
}
namespace scene
{
	class IIndexBuffer : public virtual IReferenceCounted
	{
	public:
		virtual void clear() = 0;

		virtual u32 getLast() = 0;

		virtual void set_used(u32 pUsed) = 0;

		virtual void reallocate(u32 pSize) = 0;

		virtual u32 allocated_size() const = 0;

		virtual s32 linear_reverse_search(const u32& pElement) const = 0;

		virtual video::E_INDEX_TYPE getType() const = 0;

		virtual void setType(video::E_INDEX_TYPE IndexType) = 0;

		virtual E_HARDWARE_MAPPING getHardwareMappingHint() const = 0;

		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING pHardwareMappingHint) = 0;

		virtual void addIndex(const u32& pIndex) = 0;

		virtual u32 getIndex(u32 pID) const = 0;

		virtual void* getIndices() = 0;

		virtual u32 getIndexCount() const = 0;

		virtual u32 getIndexSize() const = 0;

		virtual void setIndex(u32 pID, u32 pValue) = 0;

		virtual void setDirty() = 0;

		virtual u32 getChangedID() const = 0;
	};
}
}

#endif

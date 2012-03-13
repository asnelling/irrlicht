// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VERTEX_DESCRIPTOR_H_INCLUDED__
#define __C_VERTEX_DESCRIPTOR_H_INCLUDED__

#include "IVertexDescriptor.h"

namespace irr
{
namespace video
{
	class CVertexAttribute : public IVertexAttribute
	{
	public:
		CVertexAttribute(const CVertexAttribute& pVertexAttribute);
		CVertexAttribute(const core::stringc& pName, u32 pElementCount, E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic, E_VERTEX_ATTRIBUTE_TYPE pType, u32 pOffset);
		virtual ~CVertexAttribute();

		virtual const core::stringc& getName() const;

		virtual u32 getElementCount() const;

		virtual E_VERTEX_ATTRIBUTE_SEMANTIC getSemantic() const;

		virtual E_VERTEX_ATTRIBUTE_TYPE getType() const;

		virtual u32 getTypeSize() const;

		virtual u32 getOffset() const;

		virtual void setOffset(u32 pOffset);

	protected:
		core::stringc Name;

		u32 ElementCount;

		E_VERTEX_ATTRIBUTE_SEMANTIC Semantic;

		E_VERTEX_ATTRIBUTE_TYPE Type;

		u32 TypeSize;

		u32 Offset;
	};

	class CVertexDescriptor : public IVertexDescriptor
	{
	public:
		CVertexDescriptor(const CVertexDescriptor& pVertexDescriptor);
		CVertexDescriptor(const core::stringc& pName);
		virtual ~CVertexDescriptor();

		virtual const core::stringc& getName() const;

		virtual u32 getVertexSize() const;

		virtual bool addAttribute(const core::stringc& pName, u32 pElementCount, E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic, E_VERTEX_ATTRIBUTE_TYPE pType);

		virtual IVertexAttribute* getAttribute(u32 pID) const;

		virtual IVertexAttribute* getAttributeByName(const core::stringc& pName) const;

		virtual IVertexAttribute* getAttributeBySemantic(E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic) const;

		virtual u32 getAttributeCount() const;

		virtual bool removeAttribute(u32 pID);

		virtual void removeAllAttribute();

	protected:
		void updatePointers();

		core::stringc Name;

		u32 VertexSize;

		core::array<CVertexAttribute> Attribute;

		s32 AttributePointer[(u32)EVAS_CUSTOM+1];
	};

}
}

#endif

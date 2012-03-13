// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CVertexDescriptor.h"

namespace irr
{
namespace video
{
	CVertexAttribute::CVertexAttribute(const CVertexAttribute& pVertexAttribute)
	{
		Name = pVertexAttribute.getName();
		ElementCount = pVertexAttribute.getElementCount();
		Semantic = pVertexAttribute.getSemantic();
		Type = pVertexAttribute.getType();
		TypeSize = pVertexAttribute.getTypeSize();
		Offset = pVertexAttribute.getOffset();
	}

	CVertexAttribute::CVertexAttribute(const core::stringc& pName, u32 pElementCount, E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic, E_VERTEX_ATTRIBUTE_TYPE pType, u32 pOffset) :
		Name(pName), ElementCount(pElementCount), Semantic(pSemantic), Type(pType), Offset(pOffset)
	{
		switch(Type)
		{
		case EVAT_BYTE:
		case EVAT_UBYTE:
			TypeSize = sizeof(u8);
			break;
		case EVAT_SHORT:
		case EVAT_USHORT:
			TypeSize = sizeof(u16);
			break;
		case EVAT_INT:
		case EVAT_UINT:
			TypeSize = sizeof(u32);
			break;
		case EVAT_FLOAT:
			TypeSize = sizeof(f32);
			break;
		case EVAT_DOUBLE:
			TypeSize = sizeof(f64);
			break;
		}
	}

	CVertexAttribute::~CVertexAttribute()
	{
	}

	const core::stringc& CVertexAttribute::getName() const
	{
		return Name;
	}

	u32 CVertexAttribute::getElementCount() const
	{
		return ElementCount;
	}

	E_VERTEX_ATTRIBUTE_SEMANTIC CVertexAttribute::getSemantic() const
	{
		return Semantic;
	}

	E_VERTEX_ATTRIBUTE_TYPE CVertexAttribute::getType() const
	{
		return Type;
	}

	u32 CVertexAttribute::getTypeSize() const
	{
		return TypeSize;
	}

	u32 CVertexAttribute::getOffset() const
	{
		return Offset;
	}

	void CVertexAttribute::setOffset(u32 pOffset)
	{
		Offset = pOffset;
	}

	CVertexDescriptor::CVertexDescriptor(const CVertexDescriptor& pVertexDescriptor) : VertexSize(0)
	{
		Name = pVertexDescriptor.getName();

		for(u32 i = 0; i < EVAS_CUSTOM+1; ++i)
			AttributePointer[i] = -1;

		for(u32 i = 0; i < pVertexDescriptor.getAttributeCount(); ++i)
		{
			CVertexAttribute* attribute = (CVertexAttribute*)pVertexDescriptor.getAttribute(i);
			Attribute.push_back(*attribute);

			AttributePointer[(u32)attribute->getSemantic()] = i;

			VertexSize += (attribute->getTypeSize() * attribute->getElementCount());
		}
	}

	CVertexDescriptor::CVertexDescriptor(const core::stringc& pName) : Name(pName), VertexSize(0)
	{
		for(u32 i = 0; i < EVAS_CUSTOM+1; ++i)
			AttributePointer[i] = -1;
	}

	CVertexDescriptor::~CVertexDescriptor()
	{
		Attribute.clear();
	}

	const core::stringc& CVertexDescriptor::getName() const
	{
		return Name;
	}

	u32 CVertexDescriptor::getVertexSize() const
	{
		return VertexSize;
	}

	bool CVertexDescriptor::addAttribute(const core::stringc& pName, u32 pElementCount, E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic, E_VERTEX_ATTRIBUTE_TYPE pType)
	{
		for(u32 i = 0; i < Attribute.size(); ++i)
			if(pName == Attribute[i].getName() || (pSemantic != EVAS_CUSTOM && pSemantic == Attribute[i].getSemantic()))
				return false;

		if(pElementCount < 1)
			pElementCount = 1;

		if(pElementCount > 4)
			pElementCount = 4;

		CVertexAttribute attribute(pName, pElementCount, pSemantic, pType, VertexSize);
		Attribute.push_back(attribute);

		AttributePointer[(u32)attribute.getSemantic()] = Attribute.size()-1;

		VertexSize += (attribute.getTypeSize() * attribute.getElementCount());

		return false;
	}

	IVertexAttribute* CVertexDescriptor::getAttribute(u32 pID) const
	{
		if(pID < Attribute.size())
			return (IVertexAttribute*)(&Attribute[pID]);

		return 0;
	}

	IVertexAttribute* CVertexDescriptor::getAttributeByName(const core::stringc& pName) const
	{
		for(u32 i = 0; i < Attribute.size(); ++i)
			if(pName == Attribute[i].getName())
				return (IVertexAttribute*)(&Attribute[i]);

		return 0;
	}

	IVertexAttribute* CVertexDescriptor::getAttributeBySemantic(E_VERTEX_ATTRIBUTE_SEMANTIC pSemantic) const
	{
		s32 ID = AttributePointer[(u32)pSemantic];

		if(ID >= 0)
			return (IVertexAttribute*)(&Attribute[ID]);

		return 0;
	}

	u32 CVertexDescriptor::getAttributeCount() const
	{
		return Attribute.size();
	}

	bool CVertexDescriptor::removeAttribute(u32 pID)
	{
		if(pID < Attribute.size())
		{
			AttributePointer[(u32)Attribute[pID].getSemantic()] = -1;
			Attribute.erase(pID);

			// Recalculate vertex size and attribute offsets.

			VertexSize = 0;

			for(u32 i = 0; i < Attribute.size(); ++i)
			{
				Attribute[i].setOffset(VertexSize);
				VertexSize += (Attribute[pID].getTypeSize() * Attribute[pID].getElementCount());
			}

			return true;
		}

		return false;		
	}

	void CVertexDescriptor::removeAllAttribute()
	{
		Attribute.clear();

		VertexSize = 0;

		for(u32 i = 0; i < EVAS_CUSTOM+1; ++i)
			AttributePointer[i] = -1;
	}
}
}

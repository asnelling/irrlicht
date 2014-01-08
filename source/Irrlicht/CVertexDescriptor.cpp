// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CVertexDescriptor.h"

namespace irr
{
namespace video
{
	CVertexAttribute::CVertexAttribute(const CVertexAttribute& vertexAttribute)
	{
		Name = vertexAttribute.getName();
		ElementCount = vertexAttribute.getElementCount();
		Semantic = vertexAttribute.getSemantic();
		Type = vertexAttribute.getType();
		TypeSize = vertexAttribute.getTypeSize();
		Offset = vertexAttribute.getOffset();
		BufferID = vertexAttribute.getBufferID();
	}

	CVertexAttribute::CVertexAttribute(const core::stringc& name, u32 elementCount, E_VERTEX_ATTRIBUTE_SEMANTIC semantic, E_VERTEX_ATTRIBUTE_TYPE type, u32 offset, u32 bufferID) :
		Name(name), ElementCount(elementCount), Semantic(semantic), Type(type), Offset(offset), BufferID(bufferID)
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

	void CVertexAttribute::setOffset(u32 offset)
	{
		Offset = offset;
	}

	u32 CVertexAttribute::getBufferID() const
	{
		return BufferID;
	}

	CVertexDescriptor::CVertexDescriptor(const core::stringc& name, u32 id) : ID(id), Name(name), VertexSize(0)
	{
#ifdef _DEBUG
		setDebugName("CVertexDescriptor");
#endif

		for(u32 i = 0; i < EVAS_COUNT; ++i)
			AttributePointer[i] = -1;
	}

	CVertexDescriptor::~CVertexDescriptor()
	{
		Attribute.clear();
	}

	u32 CVertexDescriptor::getID() const
	{
		return ID;
	}

	void CVertexDescriptor::setID(u32 id)
	{
		ID = id;
	}

	const core::stringc& CVertexDescriptor::getName() const
	{
		return Name;
	}

	u32 CVertexDescriptor::getVertexSize() const
	{
		return VertexSize;
	}

	bool CVertexDescriptor::addAttribute(const core::stringc& name, u32 elementCount, E_VERTEX_ATTRIBUTE_SEMANTIC semantic, E_VERTEX_ATTRIBUTE_TYPE type, u32 bufferID)
	{
		for(u32 i = 0; i < Attribute.size(); ++i)
			if(name == Attribute[i].getName() || (semantic != EVAS_CUSTOM && semantic == Attribute[i].getSemantic()))
				return false;

		if(elementCount < 1)
			elementCount = 1;

		if(elementCount > 4)
			elementCount = 4;

		CVertexAttribute attribute(name, elementCount, semantic, type, VertexSize, bufferID);
		Attribute.push_back(attribute);

		AttributePointer[(u32)attribute.getSemantic()] = Attribute.size()-1;

		VertexSize += (attribute.getTypeSize() * attribute.getElementCount());

		return true;
	}

	IVertexAttribute* CVertexDescriptor::getAttribute(u32 id) const
	{
		if(id < Attribute.size())
			return (IVertexAttribute*)(&Attribute[id]);

		return 0;
	}

	IVertexAttribute* CVertexDescriptor::getAttributeByName(const core::stringc& name) const
	{
		for(u32 i = 0; i < Attribute.size(); ++i)
			if(name == Attribute[i].getName())
				return (IVertexAttribute*)(&Attribute[i]);

		return 0;
	}

	IVertexAttribute* CVertexDescriptor::getAttributeBySemantic(E_VERTEX_ATTRIBUTE_SEMANTIC semantic) const
	{
		s32 ID = AttributePointer[(u32)semantic];

		if(ID >= 0)
			return (IVertexAttribute*)(&Attribute[ID]);

		return 0;
	}

	u32 CVertexDescriptor::getAttributeCount() const
	{
		return Attribute.size();
	}

	bool CVertexDescriptor::removeAttribute(u32 id)
	{
		if(id < Attribute.size())
		{
			AttributePointer[(u32)Attribute[id].getSemantic()] = -1;
			Attribute.erase(id);

			// Recalculate vertex size and attribute offsets.

			VertexSize = 0;

			for(u32 i = 0; i < Attribute.size(); ++i)
			{
				Attribute[i].setOffset(VertexSize);
				VertexSize += (Attribute[id].getTypeSize() * Attribute[id].getElementCount());
			}

			return true;
		}

		return false;		
	}

	void CVertexDescriptor::removeAllAttribute()
	{
		Attribute.clear();

		VertexSize = 0;

		for(u32 i = 0; i < EVAS_COUNT; ++i)
			AttributePointer[i] = -1;
	}
}
}

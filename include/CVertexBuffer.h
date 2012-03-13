// Copyright (C) 2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VERTEX_BUFFER_H_INCLUDED__
#define __C_VERTEX_BUFFER_H_INCLUDED__

#include "S3DVertex.h"
#include "IVertexBuffer.h"

namespace irr
{
namespace scene
{
	template <class T>
	class CVertexBuffer : public IVertexBuffer
	{
	public:
		CVertexBuffer(video::IVertexDescriptor* pVertexDescriptor) : Vertices(0), vertexDescriptor(pVertexDescriptor), HardwareMappingHint(EHM_NEVER), ChangedID(1)
		{
			if(vertexDescriptor)
				vertexDescriptor->grab();
		}

		CVertexBuffer(const CVertexBuffer& pVertexBuffer) : Vertices(0), ChangedID(1)
		{
			vertexDescriptor = pVertexBuffer.getVertexDescriptor();

			if(vertexDescriptor)
				vertexDescriptor->grab();

			HardwareMappingHint = pVertexBuffer.getHardwareMappingHint();

			Vertices.reallocate(pVertexBuffer.getVertexCount());

			for(u32 i = 0; i < pVertexBuffer.getVertexCount(); ++i)
				Vertices.push_back(pVertexBuffer.getVertex(i));
		}

		virtual ~CVertexBuffer()
		{
			Vertices.clear();

			if(vertexDescriptor)
				vertexDescriptor->drop();
		}

		virtual void clear()
		{
			Vertices.clear();
		}

		virtual T& getLast()
		{
			return Vertices.getLast();
		}

		virtual void set_used(u32 pUsed)
		{
			Vertices.set_used(pUsed);
		}

		virtual void reallocate(u32 pSize)
		{
			Vertices.reallocate(pSize);
		}

		virtual u32 allocated_size() const
		{
			return Vertices.allocated_size();
		}

		virtual s32 linear_reverse_search(const T& pElement) const
		{
			return Vertices.linear_reverse_search(pElement);
		}

		virtual s32 linear_reverse_search(const void* pElement) const
		{
			return Vertices.linear_reverse_search((T&)(pElement));
		}

		virtual video::IVertexDescriptor* getVertexDescriptor() const
		{
			return vertexDescriptor;
		}

		virtual bool setVertexDescriptor(video::IVertexDescriptor* pVertexDescriptor)
		{
			if(pVertexDescriptor && pVertexDescriptor != vertexDescriptor)
			{
				if(vertexDescriptor)
					vertexDescriptor->drop();

				vertexDescriptor = pVertexDescriptor;
				vertexDescriptor->grab();

				return true;
			}

			return false;
		}

		virtual E_HARDWARE_MAPPING getHardwareMappingHint() const
		{
			return HardwareMappingHint;
		}

		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING pHardwareMappingHint)
		{
			if(HardwareMappingHint != pHardwareMappingHint)
				setDirty();

			HardwareMappingHint = pHardwareMappingHint;
		}

		virtual void addVertex(const T& pVertex)
		{
			Vertices.push_back(pVertex);
		}

		virtual void addVertex(const void* pVertex)
		{
			T* Vertex = (T*)pVertex;
			Vertices.push_back(*Vertex);
		}

		virtual T& getVertex(u32 pID)
		{
			return Vertices[pID];
		}

		virtual void* getVertices()
		{
			return Vertices.pointer();
		}

		virtual u32 getVertexCount() const
		{
			return Vertices.size();
		}

		virtual u32 getVertexSize() const
		{
			return sizeof(T);
		}

		virtual void setDirty()
		{
			++ChangedID;
		}

		virtual u32 getChangedID() const
		{
			return ChangedID;
		}

	protected:
		core::array<T> Vertices;

		video::IVertexDescriptor* vertexDescriptor;

		E_HARDWARE_MAPPING HardwareMappingHint;

		u32 ChangedID;
	};

	typedef CVertexBuffer<video::S3DVertex> SVertexBuffer;
	typedef CVertexBuffer<video::S3DVertex2TCoords> SVertexBufferLightMap;
	typedef CVertexBuffer<video::S3DVertexTangents> SVertexBufferTangents;
}
}

#endif

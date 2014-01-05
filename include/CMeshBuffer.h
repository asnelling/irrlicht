// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __T_MESH_BUFFER_H_INCLUDED__
#define __T_MESH_BUFFER_H_INCLUDED__

#include "IMeshBuffer.h"
#include "IVertexDescriptor.h"

namespace irr
{
namespace scene
{
	//! Implementation of the IMeshBuffer interface
	template <class T>
	class CMeshBuffer : public IMeshBuffer
	{
	public:
		//! Constructor
		CMeshBuffer(video::IVertexDescriptor* vertexDescriptor, video::E_INDEX_TYPE type = video::EIT_16BIT) : BoundingBoxNeedsRecalculated(true)
		{
			#ifdef _DEBUG
			setDebugName("CMeshBuffer");
			#endif

			if (vertexDescriptor)
				VertexBuffer.push_back(new CVertexBuffer<T>(vertexDescriptor));

			IndexBuffer = new CIndexBuffer(type);
		}

		//! Destructor
		virtual ~CMeshBuffer()
		{
			for (u32 i = 0; i < VertexBuffer.size(); ++i)
			{
				if (VertexBuffer[i])
					VertexBuffer[i]->drop();
			}

			if (IndexBuffer)
				IndexBuffer->drop();
		}

		virtual bool addVertexBuffer(IVertexBuffer* vertexBuffer)
		{
			bool Status = false;

			if (vertexBuffer)
			{
				vertexBuffer->grab();
				VertexBuffer.push_back(vertexBuffer);

				Status = true;
			}

			return Status;
		}

		virtual IVertexBuffer* getVertexBuffer(u32 id = 0) const
		{
			IVertexBuffer* vb = 0;

			if (id < VertexBuffer.size())
				vb = VertexBuffer[id];

			return vb;
		}

		virtual u32 getVertexBufferCount() const
		{
			return VertexBuffer.size();
		}

		virtual void removeVertexBuffer(u32 id)
		{
			if (id > 0 && id < VertexBuffer.size())
			{
				VertexBuffer[id]->drop();
				VertexBuffer.erase(id);
			}
		}

		virtual bool setVertexBuffer(IVertexBuffer* vertexBuffer, u32 id = 0)
		{
			if (id >= VertexBuffer.size() || !vertexBuffer || VertexBuffer[id] == vertexBuffer)
				return false;

			VertexBuffer[id]->drop();
			vertexBuffer->grab();

			VertexBuffer[id] = vertexBuffer;

			return true;
		}

		virtual IIndexBuffer* getIndexBuffer() const
		{
			return IndexBuffer;
		}

		virtual bool setIndexBuffer(IIndexBuffer* indexBuffer)
		{
			if(!indexBuffer || IndexBuffer == indexBuffer)
				return false;

			IndexBuffer->drop();
			indexBuffer->grab();

			IndexBuffer = indexBuffer;

			return true;
		}

		//! Get material of this meshbuffer
		/** \return Material of this buffer */
		virtual const video::SMaterial& getMaterial() const
		{
			return Material;
		}

		//! Get material of this meshbuffer
		/** \return Material of this buffer */
		virtual video::SMaterial& getMaterial()
		{
			return Material;
		}

		//! Get the axis aligned bounding box
		/** \return Axis aligned bounding box of this buffer. */
		virtual const core::aabbox3d<f32>& getBoundingBox() const
		{
			return BoundingBox;
		}

		//! Set the axis aligned bounding box
		/** \param box New axis aligned bounding box for this buffer. */
		//! set user axis aligned bounding box
		virtual void setBoundingBox(const core::aabbox3df& boundingBox)
		{
			BoundingBox = boundingBox;
		}

		//! Call this after changing the positions of any vertex.
		virtual void boundingBoxNeedsRecalculated()
		{
			BoundingBoxNeedsRecalculated = true;
		}

		//! Recalculate the bounding box.
		/** should be called if the mesh changed. */
		virtual void recalculateBoundingBox()
		{
			/*if(!BoundingBoxNeedsRecalculated)
				return;*/

			BoundingBoxNeedsRecalculated = false;

			if(!VertexBuffer[0]->getVertexCount() || !VertexBuffer[0]->getVertexDescriptor())
				BoundingBox.reset(0,0,0);
			else
			{
				video::IVertexAttribute* attribute = VertexBuffer[0]->getVertexDescriptor()->getAttributeBySemantic(video::EVAS_POSITION);

				if(!attribute)
				{
					BoundingBox.reset(0,0,0);
					return;
				}

				u8* offset = static_cast<u8*>(VertexBuffer[0]->getVertices());
				offset += attribute->getOffset();

				core::vector3df position(0.0f);

				memcpy(&position, offset, sizeof(core::vector3df));

				BoundingBox.reset(position);

				for(u32 j = 1; j < VertexBuffer[0]->getVertexCount(); ++j)
				{
					offset += VertexBuffer[0]->getVertexSize();
					memcpy(&position, offset, sizeof(core::vector3df));
					BoundingBox.addInternalPoint(position);
				}
			}
		}

		//! Append the vertices and indices to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		*/
		virtual void append(IVertexBuffer* vertexBuffer, IIndexBuffer* indexBuffer)
		{
			if(vertexBuffer == VertexBuffer[0] || vertexBuffer->getVertexDescriptor() != VertexBuffer[0]->getVertexDescriptor() || vertexBuffer->getVertexSize() != VertexBuffer[0]->getVertexSize())
				return;

			VertexBuffer[0]->reallocate(VertexBuffer[0]->getVertexCount() + vertexBuffer->getVertexCount());

			video::IVertexAttribute* attribute = vertexBuffer->getVertexDescriptor()->getAttributeBySemantic(video::EVAS_POSITION);

			u8* offset = static_cast<u8*>(vertexBuffer->getVertices());

			for(u32 i = 0; i < vertexBuffer->getVertexCount(); ++i)
			{
				VertexBuffer[0]->addVertex(offset);

				if(attribute)
				{
					u8* positionBuffer = offset + attribute->getOffset();

					core::vector3df position(0.0f);
					memcpy(&position, positionBuffer, sizeof(core::vector3df));
					BoundingBox.addInternalPoint(position);
				}

				offset += vertexBuffer->getVertexSize();
			}

			IndexBuffer->reallocate(IndexBuffer->getIndexCount() + indexBuffer->getIndexCount());

			for(u32 i = 0; i < indexBuffer->getIndexCount(); ++i)
				IndexBuffer->addIndex(indexBuffer->getIndex(i));
		}

		//! Append the meshbuffer to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		\param other Meshbuffer to be appended to this one.
		*/
		virtual void append(IMeshBuffer* meshBuffer)
		{
			if(this == meshBuffer)
				return;

			append(meshBuffer->getVertexBuffer(), meshBuffer->getIndexBuffer());
		}

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const
		{
			return VertexBuffer[0]->getHardwareMappingHint();
		}

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const
		{
			return IndexBuffer->getHardwareMappingHint();
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING pMappingHint, E_BUFFER_TYPE type = EBT_VERTEX_AND_INDEX)
		{
			if(type == EBT_VERTEX_AND_INDEX || type == EBT_VERTEX)
				VertexBuffer[0]->setHardwareMappingHint(pMappingHint);

			if(type == EBT_VERTEX_AND_INDEX || type == EBT_INDEX)
				IndexBuffer->setHardwareMappingHint(pMappingHint);
		}

		//! flags the mesh as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE type = EBT_VERTEX_AND_INDEX)
		{
			if(type == EBT_VERTEX_AND_INDEX || type == EBT_VERTEX)
				VertexBuffer[0]->setDirty();

			if(type == EBT_VERTEX_AND_INDEX || type == EBT_INDEX)
				IndexBuffer->setDirty();
		}

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID_Vertex() const
		{
			return VertexBuffer[0]->getChangedID();
		}

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID_Index() const
		{
			return IndexBuffer->getChangedID();
		}

		virtual core::matrix4& getTransformation()
		{
			return Transformation;
		}

		//! Material for this meshbuffer.
		video::SMaterial Material;
		//! Vertices of this buffer
		core::array<scene::IVertexBuffer*> VertexBuffer;
		//! Indices into the vertices of this buffer.
		scene::IIndexBuffer* IndexBuffer;
		//! Bounding box of this meshbuffer.
		core::aabbox3d<f32> BoundingBox;

		core::matrix4 Transformation;

		bool BoundingBoxNeedsRecalculated;
	};
} // end namespace scene
} // end namespace irr

#endif



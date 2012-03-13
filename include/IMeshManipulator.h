// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_MESH_MANIPULATOR_H_INCLUDED__
#define __I_MESH_MANIPULATOR_H_INCLUDED__

#include "IReferenceCounted.h"
#include "vector3d.h"
#include "aabbox3d.h"
#include "matrix4.h"
#include "IAnimatedMesh.h"
#include "CMeshBuffer.h"
#include "SMesh.h"

namespace irr
{
namespace scene
{
	//! An interface for easy manipulation of meshes.
	/** Scale, set alpha value, flip surfaces, and so on. This exists for
	fixing problems with wrong imported or exported meshes quickly after
	loading. It is not intended for doing mesh modifications and/or
	animations during runtime.
	*/
	class IMeshManipulator : public virtual IReferenceCounted
	{
	public:

		//! Flips the direction of surfaces.
		/** Changes backfacing triangles to frontfacing
		triangles and vice versa.
		\param mesh Mesh on which the operation is performed. */
		virtual void flipSurfaces(IMesh* pMesh) const = 0;

		//! Sets the alpha vertex color value of the whole mesh to a new value.
		/** \param buffer Meshbuffer on which the operation is performed.
		\param alpha New alpha value. Must be a value between 0 and 255. */
		virtual void setVertexColorAlpha(IMeshBuffer* pMeshBuffer, s32 pAlpha) const = 0;

		//! Sets the alpha vertex color value of the whole mesh to a new value.
		/** \param mesh Mesh on which the operation is performed.
		\param alpha New alpha value. Must be a value between 0 and 255. */
		void setVertexColorAlpha(IMesh* mesh, s32 alpha) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				setVertexColorAlpha(mesh->getMeshBuffer(i), alpha);
		}

		//! Sets the colors of all vertices to one color
		/** \param buffer Meshbuffer on which the operation is performed.
		\param color New color. */
		virtual void setVertexColors(IMeshBuffer* pMeshBuffer, video::SColor pColor) const = 0;

		//! Sets the colors of all vertices to one color
		/** \param mesh Mesh on which the operation is performed.
		\param color New color. */
		void setVertexColors(IMesh* mesh, video::SColor color) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				setVertexColors(mesh->getMeshBuffer(i), color);
		}

		//! Scales the actual meshbuffer, not a scene node.
		/** \param buffer Meshbuffer on which the operation is performed.
		\param factor Scale factor for each axis. */
		virtual void scale(IMeshBuffer* pMeshBuffer, const core::vector3df& pFactor) const = 0;

		//! Scales the actual mesh, not a scene node.
		/** \param mesh Mesh on which the operation is performed.
		\param factor Scale factor for each axis. */
		void scale(IMesh* mesh, const core::vector3df& factor) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				scale(mesh->getMeshBuffer(i), factor);
		}

		//! Scales the actual mesh, not a scene node.
		/** \deprecated Use scale() instead. This method may be removed by Irrlicht 1.9
		\param mesh Mesh on which the operation is performed.
		\param factor Scale factor for each axis. */
		_IRR_DEPRECATED_ void scaleMesh(IMesh* mesh, const core::vector3df& factor) const {return scale(mesh,factor);}

		//! Scale the texture coords of a meshbuffer.
		/** \param buffer Meshbuffer on which the operation is performed.
		\param factor Vector which defines the scale for each axis.
		\param level Number of texture coord. */
		virtual void scaleTCoords(IMeshBuffer* pMeshBuffer, const core::vector2df& pFactor, u32 pLevel = 0) const = 0;

		//! Scale the texture coords of a mesh.
		/** \param mesh Mesh on which the operation is performed.
		\param factor Vector which defines the scale for each axis.
		\param level Number of texture coord. */
		void scaleTCoords(scene::IMesh* mesh, const core::vector2df& factor, u32 level=0) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				scaleTCoords(mesh->getMeshBuffer(i), factor, level);
		}

		//! Applies a transformation to a meshbuffer
		/** \param buffer Meshbuffer on which the operation is performed.
		\param m transformation matrix. */
		virtual void transform(IMeshBuffer* pMeshBuffer, const core::matrix4& pMatrix) const = 0;

		//! Applies a transformation to a mesh
		/** \param mesh Mesh on which the operation is performed.
		\param m transformation matrix. */
		void transform(IMesh* mesh, const core::matrix4& m) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				transform(mesh->getMeshBuffer(i), m);
		}

		//! Applies a transformation to a mesh
		/** \deprecated Use transform() instead. This method may be removed by Irrlicht 1.9
		\param mesh Mesh on which the operation is performed.
		\param m transformation matrix. */
		_IRR_DEPRECATED_ virtual void transformMesh(IMesh* mesh, const core::matrix4& m) const {return transform(mesh,m);}

		//! Recalculates all normals of the mesh buffer.
		/** \param buffer: Mesh buffer on which the operation is performed.
		\param smooth: If the normals shall be smoothed.
		\param angleWeighted: If the normals shall be smoothed in relation to their angles. More expensive, but also higher precision. */
		virtual void recalculateNormals(IMeshBuffer* pMeshBuffer, bool pSmooth = false, bool pAngleWeighted = false) const = 0;

		//! Recalculates all normals of the mesh.
		/** \param mesh: Mesh on which the operation is performed.
		\param smooth: If the normals shall be smoothed.
		\param angleWeighted: If the normals shall be smoothed in relation to their angles. More expensive, but also higher precision. */
		void recalculateNormals(IMesh* mesh, bool smooth = false, bool angleWeighted = false) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				recalculateNormals(mesh->getMeshBuffer(i), smooth, angleWeighted);
		}

		virtual void recalculateTangents(IMeshBuffer* pMeshBuffer, bool pRecalculateNormals = false, bool pSmooth = false, bool pAngleWeighted = false) const = 0;

		void recalculateTangents(IMesh* mesh, bool recNormals = false, bool smooth = false, bool angleWeighted = false) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				recalculateTangents(mesh->getMeshBuffer(i), recNormals, smooth, angleWeighted);
		}

		//! Creates a planar texture mapping on the meshbuffer
		/** \param meshbuffer: Buffer on which the operation is performed.
		\param resolution: resolution of the planar mapping. This is
		the value specifying which is the relation between world space
		and texture coordinate space. */
		virtual void makePlanarTextureMapping(IMeshBuffer* pMeshBuffer, f32 pResolution = 0.001f) const = 0;

		//! Creates a planar texture mapping on the mesh
		/** \param mesh: Mesh on which the operation is performed.
		\param resolution: resolution of the planar mapping. This is
		the value specifying which is the relation between world space
		and texture coordinate space. */
		void makePlanarTextureMapping(IMesh* mesh, f32 resolution=0.001f) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				makePlanarTextureMapping(mesh->getMeshBuffer(i), resolution);
		}

		//! Creates a planar texture mapping on the meshbuffer
		/** This method is currently implemented towards the LWO planar mapping. A more general biasing might be required.
		\param buffer Buffer on which the operation is performed.
		\param resolutionS Resolution of the planar mapping in horizontal direction. This is the ratio between object space and texture space.
		\param resolutionT Resolution of the planar mapping in vertical direction. This is the ratio between object space and texture space.
		\param axis The axis along which the texture is projected. The allowed values are 0 (X), 1(Y), and 2(Z).
		\param offset Vector added to the vertex positions (in object coordinates).
		*/
		virtual void makePlanarTextureMapping(IMeshBuffer* pMeshBuffer, f32 pResolutionS, f32 pResolutionT, u8 pAxis, const core::vector3df& pOffset) const = 0;

		//! Creates a planar texture mapping on the buffer
		/** This method is currently implemented towards the LWO planar mapping. A more general biasing might be required.
		\param mesh Mesh on which the operation is performed.
		\param resolutionS Resolution of the planar mapping in horizontal direction. This is the ratio between object space and texture space.
		\param resolutionT Resolution of the planar mapping in vertical direction. This is the ratio between object space and texture space.
		\param axis The axis along which the texture is projected. The allowed values are 0 (X), 1(Y), and 2(Z).
		\param offset Vector added to the vertex positions (in object coordinates).
		*/
		void makePlanarTextureMapping(scene::IMesh* mesh,
				f32 resolutionS, f32 resolutionT,
				u8 axis, const core::vector3df& offset) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				makePlanarTextureMapping(mesh->getMeshBuffer(i), resolutionS, resolutionT, axis, offset);
		}

		virtual bool copyVertices(IVertexBuffer* pSrcBuffer, IVertexBuffer* pDestBuffer, bool pCopyCustomAttribute) const = 0;

		//! Clones a static IMesh into a modifiable SMesh.
		/** All meshbuffers in the returned SMesh.
		\param mesh pMesh to copy.
		\return mesh mesh. If you no longer need the
		cloned mesh, you should call SMesh::drop(). See
		IReferenceCounted::drop() for more information. */
		template<class T>
		SMesh* createMeshCopy(scene::IMesh* pMesh, video::IVertexDescriptor* pVertexDescriptor) const
		{
			if(!pMesh)
				return 0;

			if(pVertexDescriptor->getVertexSize() != sizeof(T))
				return 0;

			SMesh* mesh = new SMesh();

			for(u32 i = 0; i < pMesh->getMeshBufferCount(); ++i)
			{
				IMeshBuffer* meshBuffer = pMesh->getMeshBuffer(i);

				CMeshBuffer<T>* buffer = new CMeshBuffer<T>(pVertexDescriptor, meshBuffer->getIndexBuffer()->getType());
				buffer->Material = meshBuffer->getMaterial();

				if(!buffer->getVertexBuffer())
					continue;

				copyVertices(meshBuffer->getVertexBuffer(), buffer->getVertexBuffer(), true);
				buffer->getIndexBuffer()->reallocate(meshBuffer->getIndexBuffer()->getIndexCount());

				for(u32 j = 0; j < meshBuffer->getIndexBuffer()->getIndexCount(); ++j)
					buffer->getIndexBuffer()->addIndex(meshBuffer->getIndexBuffer()->getIndex(j));

				mesh->addMeshBuffer(buffer);
				buffer->drop();
			}

			mesh->BoundingBox = pMesh->getBoundingBox();

			return mesh;
		}

		template <class T>
		bool convertVertices(IMeshBuffer* pMeshBuffer, video::IVertexDescriptor* pVertexDescriptor, bool pCopyCustomAttribute)
		{
			if(!pMeshBuffer || !pVertexDescriptor)
				return false;

			// Is the same vertex type?

			if(pVertexDescriptor == pMeshBuffer->getVertexBuffer()->getVertexDescriptor())
				return false;

			// Is Descriptor compatible with vertex size?

			if(pVertexDescriptor->getVertexSize() != sizeof(T))
				return false;

			// Create new buffers and copy old data.

			CVertexBuffer<T>* VertexBuffer = new CVertexBuffer<T>(pVertexDescriptor);
			copyVertices(pMeshBuffer->getVertexBuffer(), VertexBuffer, true);

			pMeshBuffer->setVertexBuffer(VertexBuffer);

			return true;
		}

		template <class T>
		bool createTangents(IMeshBuffer* pMeshBuffer, video::IVertexDescriptor* pVertexDescriptor, bool pCopyCustomAttribute)
		{
			if(!pMeshBuffer || !pVertexDescriptor)
				return false;

			// Check Descriptor format for required 5 components.

			int Found = 0;

			for(u32 i = 0; i < pVertexDescriptor->getAttributeCount(); ++i)
			{
				if(pVertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
					Found++;

				if(pVertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_NORMAL)
					Found++;

				if(pVertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_TEXCOORD0)
					Found++;

				if(pVertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_TANGENT)
					Found++;

				if(pVertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_BINORMAL)
					Found++;
			}

			if(Found != 5)
				return false;

			// Create new buffers and copy old data.

			if(pVertexDescriptor != pMeshBuffer->getVertexBuffer()->getVertexDescriptor() && !convertVertices<T>(pMeshBuffer, pVertexDescriptor, pCopyCustomAttribute))
				return false;

			// Calculate tangents.

			recalculateTangents(pMeshBuffer, false, false, false);

			return true;
		}

		template <class T>
		bool createUniquePrimitives(IMeshBuffer* pMeshBuffer) const
		{
			if(!pMeshBuffer)
				return false;

			// Is Descriptor compatible with vertex size?

			if(pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getVertexSize() != sizeof(T))
				return false;

			// Create Vertex Buffer.

			CVertexBuffer<T>* VertexBuffer = new CVertexBuffer<T>(pMeshBuffer->getVertexBuffer()->getVertexDescriptor());
			VertexBuffer->reallocate(pMeshBuffer->getIndexBuffer()->getIndexCount());

			T* Vertices = static_cast<T*>(pMeshBuffer->getVertexBuffer()->getVertices());

			// Copy vertices.

			for(u32 i = 0; i < pMeshBuffer->getIndexBuffer()->getIndexCount(); i+=3)
			{
				VertexBuffer->addVertex(Vertices[pMeshBuffer->getIndexBuffer()->getIndex(i+0)]);
				VertexBuffer->addVertex(Vertices[pMeshBuffer->getIndexBuffer()->getIndex(i+1)]);
				VertexBuffer->addVertex(Vertices[pMeshBuffer->getIndexBuffer()->getIndex(i+2)]);

				pMeshBuffer->getIndexBuffer()->setIndex(i+0, i+0);
				pMeshBuffer->getIndexBuffer()->setIndex(i+1, i+1);
				pMeshBuffer->getIndexBuffer()->setIndex(i+2, i+2);
			}

			pMeshBuffer->setVertexBuffer(VertexBuffer);

			pMeshBuffer->recalculateBoundingBox();

			return true;
		}

		template <class T>
		bool createWelded(IMeshBuffer* pMeshBuffer, f32 pTolerance = core::ROUNDING_ERROR_f32, bool pCheck4Component = true, bool pCheck3Component = true, bool pCheck2Component = true, bool pCheck1Component = true) const
		{
			if(!pMeshBuffer)
				return false;

			// Is Descriptor compatible with vertex size?

			if(pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getVertexSize() != sizeof(T))
				return false;

			// Create Vertex and Index Buffers.

			CVertexBuffer<T>* VertexBuffer = new CVertexBuffer<T>(pMeshBuffer->getVertexBuffer()->getVertexDescriptor());
			CIndexBuffer* IndexBuffer = new CIndexBuffer(pMeshBuffer->getIndexBuffer()->getType());

			VertexBuffer->reallocate(pMeshBuffer->getVertexBuffer()->getVertexCount());
			IndexBuffer->set_used(pMeshBuffer->getIndexBuffer()->getIndexCount());

			core::array<u32> Redirects;
			Redirects.set_used(pMeshBuffer->getVertexBuffer()->getVertexCount());

			u8* Vertices = static_cast<u8*>(pMeshBuffer->getVertexBuffer()->getVertices());

			// Create indices.

			bool CheckComponents[4] =
			{
				pCheck1Component,
				pCheck2Component,
				pCheck3Component,
				pCheck4Component
			};

			for(u32 i = 0; i < pMeshBuffer->getVertexBuffer()->getVertexCount(); ++i)
			{
				bool found = false;

				for(u32 j = 0; j < i; ++j)
				{
					bool Equal = true;
					bool Compare = false;

					for(u32 l = 0; l < pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeCount() && Equal; ++l)
					{
						u32 ElementCount = pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getElementCount();

						if(ElementCount > 4)
							continue;

						switch(pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getType())
						{
						case video::EVAT_BYTE:
							{
								s8* valueA = (s8*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								s8* valueB = (s8*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_UBYTE:
							{
								u8* valueA = (u8*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								u8* valueB = (u8*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_SHORT:
							{
								s16* valueA = (s16*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								s16* valueB = (s16*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_USHORT:
							{
								u16* valueA = (u16*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								u16* valueB = (u16*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_INT:
							{
								s32* valueA = (s32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								s32* valueB = (s32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_UINT:
							{
								u32* valueA = (u32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								u32* valueB = (u32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_FLOAT:
							{
								f32* valueA = (f32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								f32* valueB = (f32*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((f32)(valueA[k]), (f32)(valueB[k]), (f32)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_DOUBLE:
							{
								f64* valueA = (f64*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
								f64* valueB = (f64*)(Vertices + pMeshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(l)->getOffset() + pMeshBuffer->getVertexBuffer()->getVertexSize() * j);

								if(CheckComponents[ElementCount-1])
								{
									for(u32 k = 0; k < ElementCount; ++k)
										if(!core::equals((f64)(valueA[k]), (f64)(valueB[k]), (f64)pTolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						}
					}

					if(Equal && Compare)
					{
						Redirects[i] = Redirects[j];
						found = true;
						break;
					}
				}

				if(!found)
				{
					Redirects[i] = VertexBuffer->getVertexCount();
					VertexBuffer->addVertex(Vertices + pMeshBuffer->getVertexBuffer()->getVertexSize() * i);
				}
			}

			for(u32 i = 0; i < pMeshBuffer->getIndexBuffer()->getIndexCount(); ++i)
				IndexBuffer->setIndex(i, Redirects[pMeshBuffer->getIndexBuffer()->getIndex(i)]);

			pMeshBuffer->setVertexBuffer(VertexBuffer);
			pMeshBuffer->setIndexBuffer(IndexBuffer);

			pMeshBuffer->recalculateBoundingBox();

			return true;
		}

		//! Get amount of polygons in mesh.
		/** \param mesh Input mesh
		\return Number of polygons in mesh. */
		virtual s32 getPolyCount(IMesh* pMesh) const = 0;

		//! Get amount of polygons in mesh.
		/** \param mesh Input mesh
		\return Number of polygons in mesh. */
		virtual s32 getPolyCount(IAnimatedMesh* pMesh) const = 0;

		//! Create a new AnimatedMesh and adds the mesh to it
		/** \param mesh Input mesh
		\param type The type of the animated mesh to create.
		\return Newly created animated mesh with mesh as its only
		content. When you don't need the animated mesh anymore, you
		should call IAnimatedMesh::drop(). See
		IReferenceCounted::drop() for more information. */
		virtual IAnimatedMesh * createAnimatedMesh(IMesh* pMesh, E_ANIMATED_MESH_TYPE pType = EAMT_UNKNOWN) const = 0;

		//! Vertex cache optimization according to the Forsyth paper
		/** More information can be found at
		http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html

		The function is thread-safe (read: you can optimize several
		meshes in different threads).

		\param mesh Source mesh for the operation.
		\return A new mesh optimized for the vertex cache. */
		virtual IMesh* createForsythOptimizedMesh(const IMesh *pMesh) const = 0;
};

} // end namespace scene
} // end namespace irr


#endif

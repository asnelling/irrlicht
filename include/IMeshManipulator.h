// Copyright (C) 2002-2012 Nikolaus Gebhardt
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
		virtual void flipSurfaces(IMesh* mesh) const = 0;

		//! Sets the alpha vertex color value of the whole mesh to a new value.
		/** \param buffer Meshbuffer on which the operation is performed.
		\param alpha New alpha value. Must be a value between 0 and 255. */
		virtual void setVertexColorAlpha(IMeshBuffer* meshBuffer, s32 alpha) const = 0;

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
		virtual void setVertexColors(IMeshBuffer* meshBuffer, video::SColor color) const = 0;

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
		virtual void scale(IMeshBuffer* meshBuffer, const core::vector3df& factor) const = 0;

		//! Scales the actual mesh, not a scene node.
		/** \param mesh Mesh on which the operation is performed.
		\param factor Scale factor for each axis. */
		void scale(IMesh* mesh, const core::vector3df& factor) const
		{
			core::aabbox3df BufferBox;

			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
			{
				scale(mesh->getMeshBuffer(i), factor);

				if (0 == i)
					BufferBox.reset(mesh->getMeshBuffer(i)->getBoundingBox());
				else
					BufferBox.addInternalBox(mesh->getMeshBuffer(i)->getBoundingBox());
			}

			mesh->setBoundingBox(BufferBox);
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
		virtual void scaleTCoords(IMeshBuffer* meshBuffer, const core::vector2df& factor, u32 level = 0) const = 0;

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
		virtual void transform(IMeshBuffer* meshBuffer, const core::matrix4& mat) const = 0;

		//! Applies a transformation to a mesh
		/** \param mesh Mesh on which the operation is performed.
		\param m transformation matrix. */
		void transform(IMesh* mesh, const core::matrix4& m) const
		{
			core::aabbox3df BufferBox;

			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
			{
				transform(mesh->getMeshBuffer(i), m);

				if (0 == i)
					BufferBox.reset(mesh->getMeshBuffer(i)->getBoundingBox());
				else
					BufferBox.addInternalBox(mesh->getMeshBuffer(i)->getBoundingBox());
			}

			mesh->setBoundingBox(BufferBox);
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
		virtual void recalculateNormals(IMeshBuffer* meshBuffer, bool smooth = false, bool angleWeighted = false) const = 0;

		//! Recalculates all normals of the mesh.
		/** \param mesh: Mesh on which the operation is performed.
		\param smooth: If the normals shall be smoothed.
		\param angleWeighted: If the normals shall be smoothed in relation to their angles. More expensive, but also higher precision. */
		void recalculateNormals(IMesh* mesh, bool smooth = false, bool angleWeighted = false) const
		{
			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
				recalculateNormals(mesh->getMeshBuffer(i), smooth, angleWeighted);
		}

		virtual void recalculateTangents(IMeshBuffer* meshBuffer, bool recalculateNormals = false, bool smooth = false, bool angleWeighted = false) const = 0;

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
		virtual void makePlanarTextureMapping(IMeshBuffer* meshBuffer, f32 resolution = 0.001f) const = 0;

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
		virtual void makePlanarTextureMapping(IMeshBuffer* meshBuffer, f32 resolutionS, f32 resolutionT, u8 axis, const core::vector3df& offset) const = 0;

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

		virtual bool copyVertices(IVertexBuffer* srcBuffer, IVertexBuffer* dstBuffer, bool copyCustomAttribute) const = 0;

		//! Clones a static IMesh into a modifiable SMesh.
		/** All meshbuffers in the returned SMesh.
		\param mesh mesh to copy.
		\return mesh mesh. If you no longer need the
		cloned mesh, you should call SMesh::drop(). See
		IReferenceCounted::drop() for more information. */
		template<class T>
		SMesh* createMeshCopy(scene::IMesh* mesh, video::IVertexDescriptor* vertexDescriptor) const
		{
			if(!mesh)
				return 0;

			if(vertexDescriptor->getVertexSize() != sizeof(T))
				return 0;

			SMesh* newMesh = new SMesh();

			for(u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
			{
				IMeshBuffer* meshBuffer = mesh->getMeshBuffer(i);

				CMeshBuffer<T>* buffer = new CMeshBuffer<T>(vertexDescriptor, meshBuffer->getIndexBuffer()->getType());
				buffer->Material = meshBuffer->getMaterial();

				if(!buffer->getVertexBuffer(0))
					continue;

                copyVertices(meshBuffer->getVertexBuffer(0), buffer->getVertexBuffer(0), true);
                buffer->getIndexBuffer()->reallocate(meshBuffer->getIndexBuffer()->getIndexCount());

                for (u32 j = 0; j < meshBuffer->getIndexBuffer()->getIndexCount(); ++j)
                    buffer->getIndexBuffer()->addIndex(meshBuffer->getIndexBuffer()->getIndex(j));

                for (u32 i = 1; i < meshBuffer->getVertexBufferCount(); ++i)
                {
                    CVertexBuffer<T>* vertexBuffer = new CVertexBuffer<T>(vertexDescriptor);

                    if (buffer->addVertexBuffer(vertexBuffer))
                        copyVertices(meshBuffer->getVertexBuffer(i), buffer->getVertexBuffer(i), true);
                }

                newMesh->addMeshBuffer(buffer);
                buffer->drop();
			}

			newMesh->BoundingBox = mesh->getBoundingBox();

			return newMesh;
		}

		template <class T>
		bool convertVertices(IMeshBuffer* meshBuffer, video::IVertexDescriptor* vertexDescriptor, bool copyCustomAttribute)
		{
			if(!meshBuffer || !vertexDescriptor)
				return false;

			// Is the same vertex type?

			if(vertexDescriptor == meshBuffer->getVertexBuffer(0)->getVertexDescriptor())
				return false;

			// Is Descriptor compatible with vertex size?

			if(vertexDescriptor->getVertexSize() != sizeof(T))
				return false;

			// Create new buffers and copy old data.

			for (u32 i = 0; i < meshBuffer->getVertexBufferCount(); ++i)
            {
                CVertexBuffer<T>* vertexBuffer = new CVertexBuffer<T>(vertexDescriptor);
                copyVertices(meshBuffer->getVertexBuffer(i), vertexBuffer, true);

                // setVert(/Ind)exBuffer increase the refcount of the buffer so we have to drop them after that
                meshBuffer->setVertexBuffer(vertexBuffer, i);

                vertexBuffer->drop();
            }

			return true;
		}

		template <class T>
		bool createTangents(IMeshBuffer* meshBuffer, video::IVertexDescriptor* vertexDescriptor, bool copyCustomAttribute)
		{
			if (!meshBuffer || !vertexDescriptor)
				return false;

			// Check Descriptor format for required 5 components.

			int Found = 0;

			for (u32 i = 0; i < vertexDescriptor->getAttributeCount(); ++i)
			{
				if (vertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
					Found++;

				if (vertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_NORMAL)
					Found++;

				if (vertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_TEXCOORD0)
					Found++;

				if (vertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_TANGENT)
					Found++;

				if (vertexDescriptor->getAttribute(i)->getSemantic() == video::EVAS_BINORMAL)
					Found++;
			}

			if (Found != 5)
				return false;

			// Create new buffers and copy old data.

			if (vertexDescriptor != meshBuffer->getVertexBuffer(0)->getVertexDescriptor() && !convertVertices<T>(meshBuffer, vertexDescriptor, copyCustomAttribute))
				return false;

			// Calculate tangents.

			recalculateTangents(meshBuffer, false, false, false);

			return true;
		}

        // Only mesh buffers with one vertex buffer are supported.
		template <class T>
		bool createUniquePrimitives(IMeshBuffer* meshBuffer) const
		{
			if (!meshBuffer || meshBuffer->getVertexBufferCount() > 0)
				return false;

			// Is Descriptor compatible with vertex size?

			if (meshBuffer->getVertexBuffer(0)->getVertexDescriptor()->getVertexSize() != sizeof(T))
				return false;

			// Create Vertex Buffer.

			IIndexBuffer* indexBuffer = meshBuffer->getIndexBuffer();
			const u32 indexCount = indexBuffer->getIndexCount();

			CVertexBuffer<T>* vertexBuffer = new CVertexBuffer<T>(meshBuffer->getVertexBuffer(0)->getVertexDescriptor());
			vertexBuffer->reallocate(indexCount);

			T* Vertices = static_cast<T*>(meshBuffer->getVertexBuffer(0)->getVertices());

			// Copy vertices.

			for (u32 i = 0; i < indexCount; i+=3)
			{
				vertexBuffer->addVertex(Vertices[indexBuffer->getIndex(i+0)]);
				vertexBuffer->addVertex(Vertices[indexBuffer->getIndex(i+1)]);
				vertexBuffer->addVertex(Vertices[indexBuffer->getIndex(i+2)]);

				indexBuffer->setIndex(i+0, i+0);
				indexBuffer->setIndex(i+1, i+1);
				indexBuffer->setIndex(i+2, i+2);
			}

			meshBuffer->setVertexBuffer(vertexBuffer);

			// setVert(/Ind)exBuffer increase the refcount of the buffer so we have to drop them after that
			vertexBuffer->drop();

			meshBuffer->recalculateBoundingBox();

			return true;
		}

        // Only mesh buffers with one vertex buffer are supported.
		template <class T>
		bool createWelded(IMeshBuffer* meshBuffer, f32 tolerance = core::ROUNDING_ERROR_f32, bool check4Component = true, bool check3Component = true, bool check2Component = true, bool check1Component = true) const
		{
			if (!meshBuffer || meshBuffer->getVertexBufferCount() > 0)
				return false;

			// Is Descriptor compatible with vertex size?

            IVertexBuffer* origVertexBuffer = meshBuffer->getVertexBuffer(0);
            const u32 vertexSize = origVertexBuffer->getVertexSize();

			if (vertexSize != sizeof(T))
				return false;

			// Create Vertex and Index Buffers.

			CVertexBuffer<T>* vertexBuffer = new CVertexBuffer<T>(origVertexBuffer->getVertexDescriptor());
			CIndexBuffer* indexBuffer = new CIndexBuffer(meshBuffer->getIndexBuffer()->getType());

			vertexBuffer->reallocate(origVertexBuffer->getVertexCount());
			indexBuffer->set_used(meshBuffer->getIndexBuffer()->getIndexCount());

			core::array<u32> Redirects;
			Redirects.set_used(origVertexBuffer->getVertexCount());

			u8* Vertices = static_cast<u8*>(origVertexBuffer->getVertices());

			// Create indices.

			bool checkComponents[4] =
			{
				check1Component,
				check2Component,
				check3Component,
				check4Component
			};

			for (u32 i = 0; i < origVertexBuffer->getVertexCount(); ++i)
			{
				bool found = false;

				for (u32 j = 0; j < i; ++j)
				{
					bool Equal = true;
					bool Compare = false;

					for (u32 l = 0; l < origVertexBuffer->getVertexDescriptor()->getAttributeCount() && Equal; ++l)
					{
						u32 ElementCount = origVertexBuffer->getVertexDescriptor()->getAttribute(l)->getElementCount();

						if(ElementCount > 4)
							continue;

                        const u32 AttributeOffset = origVertexBuffer->getVertexDescriptor()->getAttribute(l)->getOffset();

						switch (origVertexBuffer->getVertexDescriptor()->getAttribute(l)->getType())
						{
						case video::EVAT_BYTE:
							{
								s8* valueA = (s8*)(Vertices + AttributeOffset + vertexSize * i);
								s8* valueB = (s8*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_UBYTE:
							{
								u8* valueA = (u8*)(Vertices + AttributeOffset + vertexSize * i);
								u8* valueB = (u8*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_SHORT:
							{
								s16* valueA = (s16*)(Vertices + AttributeOffset + vertexSize * i);
								s16* valueB = (s16*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_USHORT:
							{
								u16* valueA = (u16*)(Vertices + AttributeOffset + vertexSize * i);
								u16* valueB = (u16*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_INT:
							{
								s32* valueA = (s32*)(Vertices + AttributeOffset + vertexSize * i);
								s32* valueB = (s32*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((s32)(valueA[k]), (s32)(valueB[k]), (s32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_UINT:
							{
								u32* valueA = (u32*)(Vertices + AttributeOffset + vertexSize * i);
								u32* valueB = (u32*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((u32)(valueA[k]), (u32)(valueB[k]), (u32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_FLOAT:
							{
								f32* valueA = (f32*)(Vertices + AttributeOffset + vertexSize * i);
								f32* valueB = (f32*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((f32)(valueA[k]), (f32)(valueB[k]), (f32)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						case video::EVAT_DOUBLE:
							{
								f64* valueA = (f64*)(Vertices + AttributeOffset + vertexSize * i);
								f64* valueB = (f64*)(Vertices + AttributeOffset + vertexSize * j);

								if (checkComponents[ElementCount-1])
								{
									for (u32 k = 0; k < ElementCount; ++k)
										if (!core::equals((f64)(valueA[k]), (f64)(valueB[k]), (f64)tolerance))
											Equal = false;

									Compare = true;
								}
							}
							break;
						}
					}

					if (Equal && Compare)
					{
						Redirects[i] = Redirects[j];
						found = true;
						break;
					}
				}

				if (!found)
				{
					Redirects[i] = vertexBuffer->getVertexCount();
					vertexBuffer->addVertex(Vertices + origVertexBuffer->getVertexSize() * i);
				}
			}

			for (u32 i = 0; i < meshBuffer->getIndexBuffer()->getIndexCount(); ++i)
				indexBuffer->setIndex(i, Redirects[meshBuffer->getIndexBuffer()->getIndex(i)]);

			meshBuffer->setVertexBuffer(vertexBuffer);
			meshBuffer->setIndexBuffer(indexBuffer);

			// setVert(/Ind)exBuffer increase the refcount of the buffer so we have to drop them after that
			vertexBuffer->drop();
			indexBuffer->drop();

			meshBuffer->recalculateBoundingBox();

			return true;
		}

		//! Get amount of polygons in mesh.
		/** \param mesh Input mesh
		\return Number of polygons in mesh. */
		virtual s32 getPolyCount(IMesh* mesh) const = 0;

		//! Get amount of polygons in mesh.
		/** \param mesh Input mesh
		\return Number of polygons in mesh. */
		virtual s32 getPolyCount(IAnimatedMesh* mesh) const = 0;

		//! Create a new AnimatedMesh and adds the mesh to it
		/** \param mesh Input mesh
		\param type The type of the animated mesh to create.
		\return Newly created animated mesh with mesh as its only
		content. When you don't need the animated mesh anymore, you
		should call IAnimatedMesh::drop(). See
		IReferenceCounted::drop() for more information. */
		virtual IAnimatedMesh * createAnimatedMesh(IMesh* mesh, E_ANIMATED_MESH_TYPE type = EAMT_UNKNOWN) const = 0;

		//! Vertex cache optimization according to the Forsyth paper
		/** More information can be found at
		http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html

		The function is thread-safe (read: you can optimize several
		meshes in different threads).

		\param mesh Source mesh for the operation.
		\return A new mesh optimized for the vertex cache. */
		virtual IMesh* createForsythOptimizedMesh(const IMesh *mesh) const = 0;
};

} // end namespace scene
} // end namespace irr


#endif

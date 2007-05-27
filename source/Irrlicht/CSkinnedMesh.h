// Copyright (C) 2002-2006 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h


//New skinned mesh

#ifndef __C_SKINNED_MESH_H_INCLUDED__
#define __C_SKINNED_MESH_H_INCLUDED__

#include "S3DVertex.h"
#include "irrString.h"
#include "matrix4.h"
#include "SMeshBuffer.h"
#include <quaternion.h>

#include "ISkinnedMesh.h"

namespace irr
{
namespace scene
{

	class IAnimatedMeshSceneNode;
	class IBoneSceneNode;

	class CSkinnedMesh: public ISkinnedMesh
	{
	public:

		//! constructor
		CSkinnedMesh();

		//! destructor
		virtual ~CSkinnedMesh();

		//! returns the amount of frames. If the amount is 1, it is a static (=non animated) mesh.
		virtual s32 getFrameCount();

		//! returns the animated mesh based on a detail level (which is ignored)
		virtual IMesh* getMesh(s32 frame, s32 detailLevel=255, s32 startFrameLoop=-1, s32 endFrameLoop=-1);

		//! Animates this mesh's joints based on frame input
		//! blend: {0-old position, 1-New position}
		virtual void animateMesh(f32 frame, f32 blend);

		//! Preforms a software skin on this mesh based of joint positions
		virtual void skinMesh();

		//! returns amount of mesh buffers.
		virtual u32 getMeshBufferCount() const;

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(u32 nr) const;

		//! Returns pointer to a mesh buffer which fits a material
 		/** \param material: material to search for
		\return Returns the pointer to the mesh buffer or
		NULL if there is no such mesh buffer. */
		virtual IMeshBuffer* getMeshBuffer( const video::SMaterial &material) const;

		//! returns an axis aligned bounding box
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! set user axis aligned bounding box
		virtual void setBoundingBox( const core::aabbox3df& box);

		//! sets a flag of all contained materials to a new value
		virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue);

		//! Returns the type of the animated mesh.
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

		//! Gets joint count.
		virtual s32 getJointCount() const;

		//! Gets the name of a joint.
		virtual const c8* getJointName(s32 number) const;

		//! Gets a joint number from its name
		virtual s32 getJointNumber(const c8* name) const;

		//! Update Normals when Animating
		//! False= Don't (default)
		//! True = Update normals, slower
		virtual void updateNormalsWhenAnimating(bool on);

		//! Sets Interpolation Mode
		virtual void setInterpolationMode(E_INTERPOLATION_MODE mode);

		//! Recovers the joints from the mesh
		virtual void recoverJointsFromMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes);

		//! Tranfers the joint data to the mesh
		virtual void tranferJointsToMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes);

		//! Creates an array of joints from this mesh
		virtual void createJoints(core::array<IBoneSceneNode*> &JointChildSceneNodes, IAnimatedMeshSceneNode* AnimatedMeshSceneNode, ISceneManager* SceneManager);


		virtual void convertMeshToTangents();





		//! A mesh buffer able to choose between
		//! S3DVertex2TCoords, S3DVertex and S3DVertexTangents at runtime
		struct SSkinMeshBuffer : public IMeshBuffer
		{
			SSkinMeshBuffer()
			{
				#ifdef _DEBUG
				setDebugName("SSkinMeshBuffer");
				#endif
			}

			~SSkinMeshBuffer() {};

			virtual const video::SMaterial& getMaterial() const
			{
				return Material;
			}

			virtual video::SMaterial& getMaterial()
			{
				return Material;
			}

			virtual video::S3DVertex *getVertex(u32 index)
			{
				switch (VertexType)
				{
					case video::EVT_2TCOORDS: 	return (video::S3DVertex*)&Vertices_2TCoords[index];
					case video::EVT_TANGENTS: 	return (video::S3DVertex*)&Vertices_Tangents[index];
					default: 					return &Vertices_Standard[index];
				}
			}

			virtual const void* getVertices() const
			{
				switch (VertexType)
				{
					case video::EVT_2TCOORDS: 	return Vertices_2TCoords.const_pointer();
					case video::EVT_TANGENTS: 	return Vertices_Tangents.const_pointer();
					default: 					return Vertices_Standard.const_pointer();
				}
			}

			virtual void* getVertices()
			{
				switch (VertexType)
				{
					case video::EVT_2TCOORDS: 	return Vertices_2TCoords.pointer();
					case video::EVT_TANGENTS: 	return Vertices_Tangents.pointer();
					default: 					return Vertices_Standard.pointer();
				}
			}

			virtual u32 getVertexCount() const
			{
				switch (VertexType)
				{
					case video::EVT_2TCOORDS: 	return Vertices_2TCoords.size();
					case video::EVT_TANGENTS: 	return Vertices_Tangents.size();
					default: 					return Vertices_Standard.size();
				}
			}

			virtual const u16* getIndices() const
			{
				return Indices.const_pointer();
			}

			virtual u16* getIndices()
			{
				return Indices.pointer();
			}

			virtual u32 getIndexCount() const
			{
				return Indices.size();
			}

			virtual const core::aabbox3d<f32>& getBoundingBox() const
			{
				return BoundingBox;
			}

			virtual void setBoundingBox( const core::aabbox3df& box)
			{
				BoundingBox = box;
			}

			virtual void recalculateBoundingBox()
			{
				switch (VertexType)
				{
					case video::EVT_STANDARD:
					{
						if (Vertices_Standard.empty())
							BoundingBox.reset(0,0,0);
						else
						{
							BoundingBox.reset(Vertices_Standard[0].Pos);
							for (u32 i=1; i<Vertices_Standard.size(); ++i)
								BoundingBox.addInternalPoint(Vertices_Standard[i].Pos);
						}
					}
					case video::EVT_2TCOORDS:
					{
						if (Vertices_2TCoords.empty())
							BoundingBox.reset(0,0,0);
						else
						{
							BoundingBox.reset(Vertices_2TCoords[0].Pos);
							for (u32 i=1; i<Vertices_2TCoords.size(); ++i)
								BoundingBox.addInternalPoint(Vertices_2TCoords[i].Pos);
						}
					}
					case video::EVT_TANGENTS:
					{
						if (Vertices_Tangents.empty())
							BoundingBox.reset(0,0,0);
						else
						{
							BoundingBox.reset(Vertices_Tangents[0].Pos);
							for (u32 i=1; i<Vertices_Tangents.size(); ++i)
								BoundingBox.addInternalPoint(Vertices_Tangents[i].Pos);
						}
					}
				}
			}

			virtual video::E_VERTEX_TYPE getVertexType() const
			{
				return VertexType;
			}

			//! returns the byte size (stride, pitch) of the vertex
			virtual u32 getVertexPitch() const
			{
				switch (VertexType)
				{
					case video::EVT_2TCOORDS: 	return sizeof(video::S3DVertex2TCoords);
					case video::EVT_TANGENTS: 	return sizeof(video::S3DVertexTangents);
					default: 					return sizeof(video::S3DVertex);
				}
			}

			virtual void MoveTo_2TCoords()
			{
				if (VertexType==video::EVT_STANDARD)
				{
					for(u32 n=0;n<Vertices_Standard.size();++n)
					{
						video::S3DVertex2TCoords Vertex;
						Vertex.Color=Vertices_Standard[n].Color;
						Vertex.Pos=Vertices_Standard[n].Pos;
						Vertex.Normal=Vertices_Standard[n].Normal;
						Vertex.TCoords=Vertices_Standard[n].TCoords;
						Vertices_2TCoords.push_back(Vertex);
					}
					Vertices_Standard.clear();
					VertexType=video::EVT_2TCOORDS;
				}
			}

			virtual void MoveTo_Tangents()
			{
				if (VertexType==video::EVT_STANDARD)
				{
					for(u32 n=0;n<Vertices_Standard.size();++n)
					{
						video::S3DVertexTangents Vertex;
						Vertex.Color=Vertices_Standard[n].Color;
						Vertex.Pos=Vertices_Standard[n].Pos;
						Vertex.Normal=Vertices_Standard[n].Normal;
						Vertex.TCoords=Vertices_Standard[n].TCoords;
						Vertices_Tangents.push_back(Vertex);
					}
					Vertices_Standard.clear();
					VertexType=video::EVT_TANGENTS;
				}
				else if (VertexType==video::EVT_2TCOORDS)
				{
					for(u32 n=0;n<Vertices_2TCoords.size();++n)
					{
						video::S3DVertexTangents Vertex;
						Vertex.Color=Vertices_2TCoords[n].Color;
						Vertex.Pos=Vertices_2TCoords[n].Pos;
						Vertex.Normal=Vertices_2TCoords[n].Normal;
						Vertex.TCoords=Vertices_2TCoords[n].TCoords;
						Vertices_Tangents.push_back(Vertex);
					}
					Vertices_2TCoords.clear();
					VertexType=video::EVT_TANGENTS;
				}
			}

			video::SMaterial Material;
			video::E_VERTEX_TYPE VertexType;
			core::array<video::S3DVertexTangents> Vertices_Tangents;
			core::array<video::S3DVertex2TCoords> Vertices_2TCoords;
			core::array<video::S3DVertex> Vertices_Standard;
			core::array<u16> Indices;
			core::aabbox3d<f32> BoundingBox;
		};

		//! A vertex weight
		struct SWeight
		{
			//! Index of the mesh buffer
			u16 buffer_id; //I doubt 32bits is needed

			//! Index of the vertex
			u32 vertex_id; //Store global ID here

			//! Weight Strength/Percentage (0-1)
			f32 strength;

			//! internal, please do not use
			bool *_Moved;
			core::vector3df _StaticPos;
			core::vector3df _StaticNormal;
		};


		//! Animation keyframe which describes a new position, scale or rotation
		struct SPositionKey
		{
			f32 frame;
			core::vector3df position;
		};

		struct SScaleKey
		{
			f32 frame;
			core::vector3df scale;
		};

		struct SRotationKey
		{
			f32 frame;
			core::quaternion rotation;
		};

		//! Joints
	  	struct SJoint
		{
			//! The name of this joint
			core::stringc Name;

			//! Local matrix of this joint
			core::matrix4 LocalMatrix;

			//! List of child joints
			core::array<SJoint*> Children;

			//! Animation keys causing translation change
			core::array<SPositionKey> PositionKeys;

			//! Animation keys causing scale change
			core::array<SScaleKey> ScaleKeys;

			//! Animation keys causing rotation change
			core::array<SRotationKey> RotationKeys;

			//! Skin weights
			core::array<SWeight> Weights;

			//! Internal, please do not use
			core::matrix4 _GlobalMatrix;
			core::matrix4 _GlobalInversedMatrix;
			core::vector3df _Animatedposition;
			core::vector3df _Animatedscale;
			core::quaternion _Animatedrotation;
			core::matrix4 _GlobalAnimatedMatrix;
			core::matrix4 _LocalAnimatedMatrix;
			bool _LocalAnimatedMatrix_Animated;
		};


		//! exposed for loaders to add mesh buffers
		core::array<SSkinMeshBuffer*> &getMeshBuffers();

		//! exposed for loaders to add joints
		//core::array<SJoint*> *getRootJoints(); //there is no need to expose this

		//! alternative method for adding joints
		core::array<SJoint*> &getAllJoints();

		//! loaders should call this after populating the mesh
		void finalize();


		//Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_

		//these functions will use the needed arrays, set vaules, etc to help the loaders


		SSkinMeshBuffer *createBuffer();

		SJoint *createJoint(SJoint *parent=0);

		//SKey *createKey(SJoint *joint, E_KEYFRAME_TYPE type);

		SPositionKey *createPositionKey(SJoint *joint);
		SScaleKey *createScaleKey(SJoint *joint);
		SRotationKey *createRotationKey(SJoint *joint);
		SWeight *createWeight(SJoint *joint);


private:

		core::array<SSkinMeshBuffer*> Buffers;
		core::array<SJoint*> AllJoints;
		core::array<SJoint*> RootJoints;

		bool HasAnimation;

		f32 AnimationFrames;

		f32 lastAnimatedFrame;
		f32 lastSkinnedFrame;

		bool AnimateNormals;

		E_INTERPOLATION_MODE InterpolationMode;

		core::aabbox3d<f32> BoundingBox;

		core::array< core::array<bool> > Vertices_Moved;

		void normalizeWeights();

		void buildAll_LocalAnimatedMatrices(); //public?

		void buildAll_GlobalAnimatedMatrices(SJoint *Joint=0, SJoint *ParentJoint=0);

		void getFrameData(f32 frame,SJoint *Node,core::vector3df &position, s32 &positionHint, core::vector3df &scale, s32 &scaleHint, core::quaternion &rotation, s32 &rotationHint);

		void CalculateGlobalMatrixes(SJoint *Joint,SJoint *ParentJoint);

		void SkinJoint(SJoint *Joint, SJoint *ParentJoint);

		void calculateTangents(core::vector3df& normal,
			core::vector3df& tangent, core::vector3df& binormal,
			core::vector3df& vt1, core::vector3df& vt2, core::vector3df& vt3,
			core::vector2df& tc1, core::vector2df& tc2, core::vector2df& tc3);

		//void createSkelton_Helper(ISceneManager* SceneManager, core::array<IBoneSceneNode*> &JointChildSceneNodes, IAnimatedMeshSceneNode *AnimatedMeshSceneNode, ISceneNode* ParentNode, SJoint *ParentNode, SJoint *Node);

	};

} // end namespace scene
} // end namespace irr

#endif



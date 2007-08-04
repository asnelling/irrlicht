// Copyright (C) 2002-2006 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SKINNED_MESH_H_INCLUDED__
#define __I_SKINNED_MESH_H_INCLUDED__

#include "irrArray.h"
#include "IBoneSceneNode.h"
#include "IAnimatedMesh.h"

namespace irr
{
namespace scene
{

	enum E_INTERPOLATION_MODE
	{
		// constant interpolation
		EIM_CONSTANT = 0,

		// linear interpolation
		EIM_LINEAR,

		//! count of all available interpolation modes
		EIM_COUNT
	};



	//! Interface for using some special functions of Skinned meshes
	class ISkinnedMesh : public IAnimatedMesh
	{
	public:


		//! Gets joint count.
		//! \return Returns amount of joints in the skeletal animated mesh.
		virtual s32 getJointCount() const = 0;

		//! Gets the name of a joint.
		//! \param number: Zero based index of joint. The last joint has the number
		//! IAnimatedMeshB3d::getJointCount()-1;
		//! \return Returns name of joint and null if an error happened.
		virtual const c8* getJointName(s32 number) const = 0;

		//! Gets a joint number from its name
		//! \param name: Name of the joint.
		//! \return Returns the number of the joint or -1 if not found.
		virtual s32 getJointNumber(const c8* name) const = 0;

		//! uses animation from another mesh
		//! the animation is linked (not copied) based on joint names (so make sure they are unique)
		//! \return Returns true if all joints in this mesh were matched up (empty names will not be matched, and it's case sensitive)
		//! unmatched joints will not be animated
		virtual bool useAnimationFrom(ISkinnedMesh *mesh) = 0;

		//!Update Normals when Animating
		//!False= Don't (default)
		//!True= Update normals, slower
		virtual void updateNormalsWhenAnimating(bool on) = 0;

		//!Sets Interpolation Mode
		virtual void setInterpolationMode(E_INTERPOLATION_MODE mode) = 0;

		//! Animates this mesh's joints based on frame input
		virtual void animateMesh(f32 frame, f32 blend)=0;

		//! Preforms a software skin on this mesh based of joint positions
		virtual void skinMesh() = 0;

		//!Recovers the joints from the mesh
		virtual void recoverJointsFromMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes) = 0;

		//!Tranfers the joint data to the mesh
		virtual void tranferJointsToMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes) = 0;

		//!Creates an array of joints from this mesh
		virtual void createJoints(core::array<IBoneSceneNode*> &JointChildSceneNodes,
			IAnimatedMeshSceneNode* AnimatedMeshSceneNode, ISceneManager* SceneManager) = 0;

		virtual void convertMeshToTangents() = 0;








		//! A mesh buffer able to choose between
		//! S3DVertex2TCoords, S3DVertex and S3DVertexTangents at runtime
		struct SSkinMeshBuffer : public IMeshBuffer
		{
			SSkinMeshBuffer(video::E_VERTEX_TYPE vt=video::EVT_STANDARD) : VertexType(vt)
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
						break;
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
						break;
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
						break;
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

		private:
			//! Internal members used by CSkinnedMesh
			friend class CSkinnedMesh;
			bool *Moved;
			core::vector3df StaticPos;
			core::vector3df StaticNormal;
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
			SJoint() :
				Name(""), LocalMatrix(),
				Children(), PositionKeys(), ScaleKeys(), RotationKeys(), Weights(),
				UseAnimationFrom(0), LocalAnimatedMatrix_Animated(false),positionHint(-1),scaleHint(-1),rotationHint(-1)
			{
			}

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

			//! Unnecessary for loaders, will be overwritten on finalize
			core::matrix4 GlobalMatrix;
			core::matrix4 GlobalAnimatedMatrix;
			core::matrix4 LocalAnimatedMatrix;
			core::vector3df Animatedposition;
			core::vector3df Animatedscale;
			core::quaternion Animatedrotation;

			core::matrix4 GlobalInversedMatrix; //the x format pre-calculates this

		private:
			//! Internal members used by CSkinnedMesh
			friend class CSkinnedMesh;

			SJoint *UseAnimationFrom;
			bool LocalAnimatedMatrix_Animated;

			s32 positionHint;
			s32 scaleHint;
			s32 rotationHint;

		};






		//Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_

		//these functions will use the needed arrays, set vaules, etc to help the loaders

		//! exposed for loaders to add mesh buffers
		virtual core::array<SSkinMeshBuffer*> &getMeshBuffers() = 0;

		//! alternative method for adding joints
		virtual core::array<SJoint*> &getAllJoints() = 0;

		//! loaders should call this after populating the mesh
		virtual void finalize() = 0;




		virtual SSkinMeshBuffer *createBuffer() = 0;

		virtual SJoint *createJoint(SJoint *parent=0) = 0;

		virtual SPositionKey *createPositionKey(SJoint *joint) = 0;
		virtual SScaleKey *createScaleKey(SJoint *joint) = 0;
		virtual SRotationKey *createRotationKey(SJoint *joint) = 0;

		virtual SWeight *createWeight(SJoint *joint) = 0;




	};

} // end namespace scene
} // end namespace irr

#endif




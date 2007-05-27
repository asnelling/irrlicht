// Copyright (C) 2002-2006 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SKINNED_MESH_H_INCLUDED__
#define __I_SKINNED_MESH_H_INCLUDED__

#include "irrArray.h"
#include "IBoneSceneNode.h"
#include "IMesh.h"
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
	class ISkinnedMesh : public IAnimatedMesh, public IMesh
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

	};

} // end namespace scene
} // end namespace irr

#endif



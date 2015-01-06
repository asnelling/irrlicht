// Copyright (C) 2015 Christian Holz
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_INSTANCED_MESH_SCENE_NODE_H_INCLUDED__
#define __I_INSTANCED_MESH_SCENE_NODE_H_INCLUDED__

#include "IMeshSceneNode.h"

namespace irr
{
namespace scene
{
	class ISceneManager;

	//! Scene node interface.
	/** A scene node is a node in the hierarchical scene graph. Every scene
	node may have children, which are also scene nodes. Children move
	relative to their parent's position. If the parent of a node is not
	visible, its children won't be visible either. In this way, it is for
	example easily possible to attach a light to a moving car, or to place
	a walking character on a moving platform on a moving ship.
	*/
	class IInstancedMeshSceneNode : public IMeshSceneNode
	{
	public:

		//! Constructor
		IInstancedMeshSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id = -1,
				const core::vector3df& position = core::vector3df(0,0,0),
				const core::vector3df& rotation = core::vector3df(0,0,0),
				const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f))
				: IMeshSceneNode(parent, mgr, id, position, rotation, scale)
		{
		}


		//! Destructor
		virtual ~IInstancedMeshSceneNode()
		{
		}

		virtual ISceneNode* addInstance(ISceneNode* node) = 0;

		virtual ISceneNode* addInstance(const core::vector3df& position,
			const core::vector3df& rotation = core::vector3df(0, 0, 0),
			const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f),
			s32 id = -1) = 0;

		virtual bool removeInstance(u32 index) = 0;

		virtual bool removeInstanceById(s32 id) = 0;

		virtual bool removeInstance(ISceneNode* instance) = 0;

		virtual ISceneNode* getInstance(u32 index) const = 0;

		virtual ISceneNode* getInstanceById(s32 id) const = 0;

		virtual u32 getInstanceCount() const = 0;

	protected:
	};


} // end namespace scene
} // end namespace irr

#endif
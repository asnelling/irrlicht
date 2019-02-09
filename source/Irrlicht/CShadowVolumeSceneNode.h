// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__
#define __C_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__

#include "IShadowVolumeSceneNode.h"

namespace irr
{
namespace scene
{

	//! Scene node for rendering a shadow volume into a stencil buffer.
	class CShadowVolumeSceneNode : public IShadowVolumeSceneNode
	{
	public:

		//! constructor
		CShadowVolumeSceneNode(const IMesh* shadowMesh, ISceneNode* parent, ISceneManager* mgr,
			s32 id, bool zfailmethod=true, f32 infinity=10000.0f);

		//! destructor
		virtual ~CShadowVolumeSceneNode();

		//! Sets the mesh from which the shadow volume should be generated.
		/** To optimize shadow rendering, use a simpler mesh for shadows.
		*/
		virtual void setShadowMesh(const IMesh* mesh) _IRR_OVERRIDE_;

		//! Updates the shadow volumes for current light positions.
		/** Called each render cycle from Animated Mesh SceneNode render method. */
		virtual void updateShadowVolumes() _IRR_OVERRIDE_;
		virtual void extendBoundingBox(core::aabbox3d<f32>& dst) _IRR_OVERRIDE_;

		//! pre render method
		virtual void OnRegisterSceneNode() _IRR_OVERRIDE_;

		//! renders the node.
		virtual void render() _IRR_OVERRIDE_;

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const _IRR_OVERRIDE_;

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const _IRR_OVERRIDE_ { return ESNT_SHADOW_VOLUME; }

	private:

		typedef core::array<core::vector3df> SShadowVolume;

		void createShadowVolume(const core::vector3df& pos, bool isDirectional=false);
		u32 createEdgesAndCaps(const core::vector3df& light, SShadowVolume* svp, core::aabbox3d<f32>* bb);

		core::aabbox3d<f32> Box;

		// a shadow volume for every light
		core::array<SShadowVolume> ShadowVolumes;

		// a back cap bounding box for every light
		core::array<core::aabbox3d<f32> > ShadowBBox;

		core::array<core::vector3df> Vertices;
		core::array<u16> Indices;
		core::array<u16> Edges;

/*
	shadow is always broken

	cases
	a)  #undef IRR_USE_ADJACENCY,#undef IRR_USE_REVERSE_EXTRUDED

		openGL: works best
		D3D: works(specialfx), but not visible in demo
		burning: as openGL

	b) #define IRR_USE_ADJACENCY,#define IRR_USE_REVERSE_EXTRUDED
		openGL: produces some infinity shadow mesh-vertices
		D3D:
	
*/

//#define IRR_USE_ADJACENCY
//#define IRR_USE_REVERSE_EXTRUDED

#ifdef IRR_USE_ADJACENCY
		core::array<u16> Adjacency;
		//! Generates adjacency information based on mesh indices.
		void calculateAdjacency();
#endif
		// tells if face is front facing
		core::array<bool> FaceData;

		const scene::IMesh* ShadowMesh;

		u32 IndexCount;
		u32 VertexCount;
		u32 ShadowVolumesUsed;

		f32 Infinity;

		bool UseZFailMethod;
	};

} // end namespace scene
} // end namespace irr

#endif

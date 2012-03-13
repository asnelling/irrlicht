// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_MESH_MANIPULATOR_H_INCLUDED__
#define __C_MESH_MANIPULATOR_H_INCLUDED__

#include "IMeshManipulator.h"

namespace irr
{
namespace scene
{

class CMeshManipulator : public IMeshManipulator
{
public:
	virtual void flipSurfaces(IMesh* pMesh) const;

	virtual void setVertexColor(IMeshBuffer* pMeshBuffer, video::SColor pColor, bool pOnlyAlpha) const;

	virtual void setVertexColorAlpha(IMeshBuffer* pMeshBuffer, s32 pAlpha) const;

	virtual void setVertexColors(IMeshBuffer* pMeshBuffer, video::SColor pColor) const;

	virtual void scale(IMeshBuffer* pMeshBuffer, const core::vector3df& pFactor) const;

	virtual void scaleTCoords(IMeshBuffer* pMeshBuffer, const core::vector2df& pFactor, u32 pLevel = 0) const;

	virtual void transform(IMeshBuffer* pMeshBuffer, const core::matrix4& pMatrix) const;

	virtual void recalculateNormals(IMeshBuffer* pMeshBuffer, bool pSmooth, bool pAngleWeighted) const;

	virtual void recalculateTangents(IMeshBuffer* pMeshBuffer, bool pRecalculateNormals, bool pSmooth, bool pAngleWeighted) const;

	virtual void makePlanarTextureMapping(IMeshBuffer* pMeshBuffer, f32 pResolution = 0.001f) const;

	virtual void makePlanarTextureMapping(IMeshBuffer* pMeshBuffer, f32 pResolutionS, f32 pResolutionT, u8 pAxis, const core::vector3df& pOffset) const;

	virtual bool copyVertices(IVertexBuffer* pSrcBuffer, IVertexBuffer* pDestBuffer, bool pCopyCustomAttribute) const;
	
	virtual s32 getPolyCount(IMesh* pMesh) const;

	virtual s32 getPolyCount(IAnimatedMesh* pMesh) const;

	virtual IAnimatedMesh* createAnimatedMesh(IMesh* pMesh, E_ANIMATED_MESH_TYPE pType) const;

	virtual IMesh* createForsythOptimizedMesh(const IMesh* pMesh) const;
};

} // end namespace scene
} // end namespace irr


#endif

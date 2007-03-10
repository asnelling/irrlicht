// Copyright (C) 2002-2007 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_ANIMATED_MESH_MD3_H_INCLUDED__
#define __C_ANIMATED_MESH_MD3_H_INCLUDED__

#include "IAnimatedMeshMD3.h"
#include "IReadFile.h"
#include "IFileSystem.h"
#include "irrArray.h"
#include "irrString.h"
#include "SMesh.h"
#include "SMeshBuffer.h"

namespace irr
{
namespace scene
{


	class CAnimatedMeshMD3 : public IAnimatedMeshMD3
	{
	public:

		//! constructor
		CAnimatedMeshMD3();

		//! destructor
		virtual ~CAnimatedMeshMD3();

		//! loads a quake3 md3 file
		virtual bool loadModelFile( u32 modelIndex, io::IReadFile* file);

		// IAnimatedMeshMD3
		virtual void setInterpolationShift ( u32 shift, u32 loopMode );
		virtual SMD3Mesh * getOriginalMesh ();
		virtual SMD3QuaterionTagList *getTagList(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop);

		//IAnimatedMesh
		virtual s32 getFrameCount();
		virtual IMesh* getMesh(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop);
		virtual const core::aabbox3d<f32>& getBoundingBox() const;
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

	private:
        //! animates one frame
        inline void Animate (u32 frame);

		video::SMaterial Material;

		//! hold original compressed MD3 Info
		SMD3Mesh *Mesh;

		u32 IPolShift;
		u32 LoopMode;
		f32 Scaling;

		//! Cache Info
		struct SCacheInfo
		{
			SCacheInfo ( s32 frame = -1, s32 start = -1, s32 end = -1 )
				:	Frame ( frame ), startFrameLoop ( start ),
					endFrameLoop ( end ) {}
					
			bool operator == ( const SCacheInfo &other ) const
			{
				return 0 == memcmp ( this, &other, sizeof ( SCacheInfo ) );
			}
			s32 Frame;
			s32 startFrameLoop;
			s32 endFrameLoop;
		};
		SCacheInfo Current;

		//! return a Mesh per frame
		SMesh MeshIPol;
		SMD3QuaterionTagList TagListIPol;

		IMeshBuffer * createMeshBuffer ( const SMD3MeshBuffer *source );

		void buildVertexArray ( u32 frameA, u32 frameB, f32 interpolate,
								const SMD3MeshBuffer * source,
								SMeshBuffer * dest
							);

		void buildTagArray ( u32 frameA, u32 frameB, f32 interpolate );

		void getNormal ( core::vector3df & out, u32 i, u32 j )
		{
			f32 lng = i * 2.0f * core::PI / 255.0f;
			f32 lat = j * 2.0f * core::PI / 255.0f;
			out.X = cosf ( lat ) * sinf ( lng );
			out.Y = sinf ( lat ) * sinf ( lng );
			out.Z = cos ( lng );
		}

	};

} // end namespace scene
} // end namespace irr

#endif


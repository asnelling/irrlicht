// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_X_LOADER_

#include "CXMeshFileLoader.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! Constructor
CXMeshFileLoader::CXMeshFileLoader(scene::ISceneManager* smgr)
: SceneManager(smgr)
{

}



//! destructor
CXMeshFileLoader::~CXMeshFileLoader()
{

}



//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CXMeshFileLoader::isALoadableFileExtension(const c8* filename)
{
	return strstr(filename, ".x")!=0;
}



//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IUnknown::drop() for more information.
IAnimatedMesh* CXMeshFileLoader::createMesh(irr::io::IReadFile* f)
{

	if (!f)
		return 0;

	file = f;
	AnimatedMesh = new CSkinnedMesh();

	Buffers = &AnimatedMesh->getMeshBuffers();
	AllJoints = &AnimatedMesh->getAllJoints();

	if ( load() )
	{
		AnimatedMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
	}

	return AnimatedMesh;
}


bool CXMeshFileLoader::load()
{




}






} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_X_LOADER_


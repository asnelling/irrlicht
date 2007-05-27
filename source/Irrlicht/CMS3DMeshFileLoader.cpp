// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MS3D_LOADER_

#include "CMS3DMeshFileLoader.h"
#include "CAnimatedMeshMS3D.h"

namespace irr
{
namespace scene
{

//! Constructor
CMS3DMeshFileLoader::CMS3DMeshFileLoader(video::IVideoDriver *driver) 
: Driver(driver)
{
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CMS3DMeshFileLoader::isALoadableFileExtension(const c8* filename)
{
	return strstr(filename, ".ms3d")!=0;
}



//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IUnknown::drop() for more information.
IAnimatedMesh* CMS3DMeshFileLoader::createMesh(irr::io::IReadFile* file)
{
	IAnimatedMesh* msh = 0;

	// This method loads a mesh if it cans.
	// Someday I will have to refactor this, and split the DefaultMeshFormatloader
	// into one loader for every format.

	bool success = false;

	msh = new CAnimatedMeshMS3D(Driver);
	success = ((CAnimatedMeshMS3D*)msh)->loadFile(file);
	if (success)
		return msh;

	msh->drop();

	return 0;
}

} // end namespace scene
} // end namespace irr

#endif

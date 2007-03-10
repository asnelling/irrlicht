// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OBJ_MESH_FILE_LOADER_H_INCLUDED__
#define __C_OBJ_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"
#include "irrString.h"
#include "SMesh.h"
#include "SMeshBuffer.h"

namespace irr
{
namespace scene
{

//! Meshloader capable of loading 3ds meshes.
class COBJMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	COBJMeshFileLoader(io::IFileSystem* fs, video::IVideoDriver* driver);

	//! destructor
	virtual ~COBJMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const c8* fileName);

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(irr::io::IReadFile* file);

private:

	struct SObjMtl
	{
		SObjMtl() : pMeshbuffer(0), illumination(0) {
			this->pMeshbuffer = new SMeshBuffer();
			this->pMeshbuffer->Material.Shininess = 0.0f;
			this->pMeshbuffer->Material.AmbientColor = video::SColorf(0.2f, 0.2f, 0.2f, 1.0f).toSColor();
			this->pMeshbuffer->Material.DiffuseColor = video::SColorf(0.8f, 0.8f, 0.8f, 1.0f).toSColor();
			this->pMeshbuffer->Material.SpecularColor = video::SColorf(1.0f, 1.0f, 1.0f, 1.0f).toSColor();
		};
		SObjMtl(SObjMtl& o) : pMeshbuffer(o.pMeshbuffer) { o.pMeshbuffer->grab(); };

		~SObjMtl() { 	};

		scene::SMeshBuffer *pMeshbuffer;
		core::stringc name;
		c8 illumination;
	};

	struct SObjGroup
	{
		SObjGroup() {};
		SObjGroup(SObjGroup& o) {};
		~SObjGroup() { 	};

		core::stringc name;
	};

	// returns a pointer to the first printable character available in the buffer
	const c8* goFirstWord(const c8* buf, const c8* const pBufEnd);
	// returns a pointer to the first printable character after the first non-printable
	const c8* goNextWord(const c8* buf, const c8* const pBufEnd);
	// returns a pointer to the next printable character after the first line break
	const c8* goNextLine(const c8* buf, const c8* const pBufEnd);
	// copies the current word from the inBuf to the outBuf
	u32 copyWord(c8* outBuf, const c8* inBuf, u32 outBufLength, const c8* const pBufEnd);
	// copies the current line from the inBuf to the outBuf
	u32 copyLine(c8* outBuf, const c8* inBuf, u32 outBufLength, const c8* const pBufEnd);
	// combination of goNextWord followed by copyWord
	const c8* goAndCopyNextWord(c8* outBuf, const c8* inBuf, u32 outBufLength, const c8* const pBufEnd);

	void readMTL(const c8* pFileName, core::stringc relPath);
	//! Read RGB color
	const c8* readColor(const c8* pBufPtr, video::SColor& color, const c8* const pBufEnd);
	//! Read 3d vector of floats
	const c8* readVec3(const c8* pBufPtr, core::vector3df& vec, const c8* const pBufEnd);
	//! Read 2d vector of floats
	const c8* readVec2(const c8* pBufPtr, core::vector2df& vec, const c8* const pBufEnd);
	//! Read boolean value represented as 'on' or 'off'
	const c8* readBool(const c8* pBufPtr, bool& tf, const c8* const pBufEnd);
	SObjMtl * findMtl(const c8* pMtlName);
	SObjGroup * findGroup(const c8* pGroupName);
	SObjGroup * findOrAddGroup(const c8* pGroupName);

	// reads and convert to integer the vertex indices in a line of obj file's face statement
	// -1 for the index if it doesn't exist
	// indices are changed to 0-based index instead of 1-based from the obj file
	bool retrieveVertexIndices(c8* pVertexData, s32* Idx, const c8* pBufEnd);

	void cleanUp();

	io::IFileSystem* FileSystem;
	video::IVideoDriver* Driver;

	core::array<SObjMtl*> materials;
	core::array<SObjGroup*> groups;
	SMesh* Mesh;
};

} // end namespace scene
} // end namespace irr

#endif


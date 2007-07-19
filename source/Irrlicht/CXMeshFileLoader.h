// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_X_MESH_FILE_LOADER_H_INCLUDED__
#define __C_X_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IReadFile.h"

#include "IVideoDriver.h"
#include "irrString.h"

#include "CSkinnedMesh.h"



namespace irr
{
namespace scene
{
class IMeshManipulator;

//! Meshloader capable of loading x meshes.
class CXMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CXMeshFileLoader(scene::ISceneManager* smgr);

	//! destructor
	virtual ~CXMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const c8* fileName);

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(irr::io::IReadFile* file);

private:

	bool load();

	bool readFileIntoMemory();

	bool parseFile();


	bool parseDataObject();

	bool parseDataObjectTemplate();

	bool parseDataObjectFrame(CSkinnedMesh::SJoint& joint);

	bool parseDataObjectTransformationMatrix(core::matrix4 &mat);


	bool parseUnknownDataObject();



	//! places pointer to next begin of a token, and ignores comments
	void findNextNoneWhiteSpace();

	//! places pointer to next begin of a token, which must be a number,
	// and ignores comments
	void findNextNoneWhiteSpaceNumber();

	//! returns next parseable token. Returns empty string if no token there
	core::stringc getNextToken();

	//! reads header of dataobject including the opening brace.
	//! returns false if error happened, and writes name of object
	//! if there is one
	bool readHeadOfDataObject(core::stringc* outname=0);

	//! checks for two following semicolons, returns false if they are not there
	bool checkForTwoFollowingSemicolons();

	//! reads a x file style string
	bool getNextTokenAsString(core::stringc& out);


	void readUntilEndOfLine();


	u16 readBinWord();
	u32 readBinDWord();
	s32 readInt();
	f32 readFloat();
	bool readVector2(core::vector2df& vec);
	bool readVector3(core::vector3df& vec);
	bool readRGB(video::SColorf& color);
	bool readRGBA(video::SColorf& color);



















	video::IVideoDriver* Driver;

	core::array<CSkinnedMesh::SSkinMeshBuffer*> *Buffers;
	core::array<CSkinnedMesh::SJoint*> *AllJoints;

	ISceneManager*	SceneManager;
	CSkinnedMesh*	AnimatedMesh;
	io::IReadFile*	file;





	s32 MajorVersion;
	s32 MinorVersion;
	bool binary;
	s32 binaryNumCount;

	c8* Buffer;
	s32 Size;
	c8 FloatSize;
	const c8* P;
	c8* End;

	bool ErrorHappened;



	CSkinnedMesh::SJoint *m_pgCurFrame;



};

} // end namespace scene
} // end namespace irr

#endif


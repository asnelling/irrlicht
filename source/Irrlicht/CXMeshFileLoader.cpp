// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"



#ifdef _IRR_COMPILE_WITH_X_LOADER_

#include "CXMeshFileLoader.h"
#include "os.h"

#include "fast_atof.h"
#include "coreutil.h"


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

	if (!readFileIntoMemory())
		return false;

	if (!parseFile())
		return false;



	return true;

}



//! Reads file into memory
bool CXMeshFileLoader::readFileIntoMemory()
{
	s32 Size = file->getSize();
	if (Size < 12)
	{
		os::Printer::log("X File is too small.", ELL_WARNING);
		return false;
	}

	Buffer = new c8[Size];

	//! read all into memory
	file->seek(0); // apparently sometimes files have been read already, so reset it
	if (file->read(Buffer, Size) != Size)
	{
		os::Printer::log("Could not read from x file.", ELL_WARNING);
		return false;
	}

	End = Buffer + Size;

	//! check header "xof "
	if (strncmp(Buffer, "xof ", 4)!=0)
	{
		os::Printer::log("Not an x file, wrong header.", ELL_WARNING);
		return false;
	}

	//! read minor and major version, e.g. 0302 or 0303
	c8 tmp[3];
	tmp[2] = 0x0;
	tmp[0] = Buffer[4];
	tmp[1] = Buffer[5];
	MajorVersion = strtol(tmp, (char**) &P, 10);

	tmp[0] = Buffer[6];
	tmp[1] = Buffer[7];
	MinorVersion = strtol(tmp, (char**) &P, 10);

	//! read format
	if (strncmp(&Buffer[8], "txt ", 4) ==0)
		binary = false;
	else if (strncmp(&Buffer[8], "bin ", 4) ==0)
		binary = true;
	else
	{
		os::Printer::log("Only uncompressed x files currently supported.", ELL_WARNING);
		return false;
	}
	binaryNumCount=0;

	//! read float size
	if (strncmp(&Buffer[12], "0032", 4) ==0)
		FloatSize = 4;
	else if (strncmp(&Buffer[12], "0064", 4) ==0)
		FloatSize = 8;
	else
	{
		os::Printer::log("Float size not supported.", ELL_WARNING);
		return false;
	}

	P = &Buffer[16];

	readUntilEndOfLine();

	return true;
}



//! Parses the file
bool CXMeshFileLoader::parseFile()
{
	u32 u32Idx;
	while(parseDataObject())
	{
		// loop
	}

	return true;
}



//! Parses the next Data object in the file
bool CXMeshFileLoader::parseDataObject()
{
	core::stringc objectName = getNextToken();

	if (objectName.size() == 0)
		return false;

	// parse specific object

	if (objectName == "template")
		return parseDataObjectTemplate();
	else
	if (objectName == "Frame")
	{
		m_pgCurFrame = AnimatedMesh->createJoint(0);
		return parseDataObjectFrame( *m_pgCurFrame );
	}
	else
	if (objectName == "Mesh")
	{
		// some meshes have no frames at all
		//m_pgCurFrame = AnimatedMesh->createJoint(0);

		//m_pgCurFrame->Meshes.push_back(SXMesh());
		//return parseDataObjectMesh(m_pgCurFrame->Meshes.getLast());

		SXMesh *Mesh=new SXMesh;

		Mesh->Buffer=AnimatedMesh->createBuffer();

		return parseDataObjectMesh ( *Mesh );

	}
	else
	if (objectName == "AnimationSet")
	{
		/*
		AnimationSets.push_back(SXAnimationSet());
		return parseDataObjectAnimationSet(AnimationSets.getLast());
		*/
	}
	else
	if (objectName == "Material")
	{
		/*
		// template materials now available thanks to joeWright
		TemplateMaterials.push_back(SXTemplateMaterial());
		TemplateMaterials.getLast().Name = getNextToken();
		return parseDataObjectMaterial(TemplateMaterials.getLast().Material);
		*/
	}

	os::Printer::log("Unknown data object in x file", objectName.c_str());

	return parseUnknownDataObject();
}



bool CXMeshFileLoader::parseDataObjectTemplate()
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading template");
#endif

	// parse a template data object. Currently not stored.
	core::stringc name = getNextToken();

	// ignore left delimiter
	if (getNextToken() != "{")
	{
		os::Printer::log("Left delimiter in template data object missing.",
			name.c_str(), ELL_ERROR);
		return false;
	}

	// read GUID
	core::stringc guid = getNextToken();

	// read and ignore data members
	while(true)
	{
		core::stringc s = getNextToken();

		if (s == "}")
			break;

		if (s.size() == 0)
			return false;
	}

	return true;
}



bool CXMeshFileLoader::parseDataObjectFrame(CSkinnedMesh::SJoint& joint)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading frame");
#endif

	// A coordinate frame, or "frame of reference." The Frame template
	// is open and can contain any object. The Direct3D extensions (D3DX)
	// mesh-loading functions recognize Mesh, FrameTransformMatrix, and
	// Frame template instances as child objects when loading a Frame
	// instance.



	if (!readHeadOfDataObject(&joint.Name))
	{
		os::Printer::log("No opening brace in Frame found in x file", ELL_WARNING);
		return false;
	}

	// Now inside a frame.
	// read tokens until closing brace is reached.

	while(true)
	{
		core::stringc objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Frame in x file.", ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // frame finished
		}
		else
		if (objectName == "Frame")
		{
			CSkinnedMesh::SJoint *Childjoint=AnimatedMesh->createJoint(&joint);

			if (!parseDataObjectFrame(*Childjoint))
				return false;
		}
		else
		if (objectName == "FrameTransformMatrix")
		{
			if (!parseDataObjectTransformationMatrix(joint.LocalMatrix))
				return false;
		}
		else
		if (objectName == "Mesh")
		{
			/*
			frame.Meshes.push_back(SXMesh());
			if (!parseDataObjectMesh(frame.Meshes.getLast()))
				return false;
			*/
			SXMesh *Mesh=new SXMesh;

			Mesh->Buffer=AnimatedMesh->createBuffer();

			return parseDataObjectMesh ( *Mesh );
		}
		else
		{
			os::Printer::log("Unknown data object in frame in x file", objectName.c_str());
			if (!parseUnknownDataObject())
				return false;
		}

	}

	return true;
}


bool CXMeshFileLoader::parseDataObjectTransformationMatrix(core::matrix4 &mat)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading Transformation Matrix");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Transformation Matrix found in x file", ELL_WARNING);
		return false;
	}

	if (binary)
	{
		// read matrix in binary format
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: Mesh: Expecting float list (for matrix)", ELL_WARNING);
			return false;
		}

		if (readBinDWord() != 0x10)
		{
			os::Printer::log("Binary X: Mesh: Should be 16 floats in matrix", ELL_WARNING);
			return false;
		}
	}

	for (s32 i=0; i<4; ++i)
		for (s32 j=0; j<4; ++j)
			mat(i,j)=readFloat();

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Transformation Matrix found in x file", ELL_WARNING);
		return false;
	}

	if (getNextToken() != "}")
	{
		os::Printer::log("No closing brace in Transformation Matrix found in x file", ELL_WARNING);
		return false;
	}

	return true;
}



bool CXMeshFileLoader::parseDataObjectMesh(SXMesh &mesh)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading mesh");
#endif

	core::stringc name;

	if (!readHeadOfDataObject(&name))
	{
		os::Printer::log("No opening brace in Mesh found in x file", ELL_WARNING);
		return false;
	}

	// read vertex count
	s32 nVertices = readInt();


	CSkinnedMesh::SSkinMeshBuffer &MeshBuffer=*mesh.Buffer;

	// read vertices
	MeshBuffer.Vertices_Standard.set_used(nVertices); //luke: change




	s32 count=0;
	if (binary)
	{
		// read vertices in binary format
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: Mesh: Expecting float list (for vertices)", ELL_WARNING);
			return false;
		}
		count = readBinDWord();
		if (count != (nVertices * 3))
		{
			os::Printer::log("Binary X: Mesh: Value count not matching vertices count", ELL_WARNING);
			return false;
		}
	}

	for (s32 n=0; n<nVertices; ++n)
		readVector3(MeshBuffer.Vertices_Standard[n].Pos);




	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Vertex Array found in x file", ELL_WARNING);
		return false;
	}

	// read faces
	s32 nFaces = readInt();

	MeshBuffer.Indices.set_used(nFaces * 3);
	mesh.IndexCountPerFace.set_used(nFaces);

	core::array<s32> polygonfaces;
	s32 currentIndex = 0;

	for (s32 k=0; k<nFaces; ++k)
	{
		s32 fcnt = readInt();

		if (fcnt != 3)
		{
			if (fcnt < 3)
			{
				os::Printer::log("Invalid face count (<3) found in Mesh x file reader.", ELL_WARNING);
				return false;
			}

			// read face indices
			polygonfaces.set_used(fcnt);
			s32 triangles = (fcnt-2);
			MeshBuffer.Indices.set_used(MeshBuffer.Indices.size() + ((triangles*3)-3));
			mesh.IndexCountPerFace[k] = triangles * 3;

			for (int f=0; f<fcnt; ++f)
				polygonfaces[f] = readInt();

			for (s32 jk=0; jk<triangles; ++jk)
			{
				MeshBuffer.Indices[currentIndex++] = polygonfaces[0];
				MeshBuffer.Indices[currentIndex++] = polygonfaces[jk+1];
				MeshBuffer.Indices[currentIndex++] = polygonfaces[jk+2];
			}

			// TODO: change face indices in material list
		}
		else
		{
			MeshBuffer.Indices[currentIndex++] = readInt();
			MeshBuffer.Indices[currentIndex++] = readInt();
			MeshBuffer.Indices[currentIndex++] = readInt();
			mesh.IndexCountPerFace[k] = 3;
		}
	}

	if (binary && binaryNumCount)
	{
		os::Printer::log("Binary X: Mesh: Integer count mismatch", ELL_WARNING);
		return false;
	}
	else if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Face Array found in x file", ELL_WARNING);
		return false;
	}

	// here, other data objects may follow

	while(true)
	{
		core::stringc objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Mesh in x file.", ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // mesh finished
		}
		else
		if (objectName == "MeshNormals")
		{
			if (!parseDataObjectMeshNormals(mesh))
				return false;
		}
		else
		if (objectName == "MeshTextureCoords")
		{
			if (!parseDataObjectMeshTextureCoords(mesh))
				return false;
		}
		else
		if (objectName == "MeshVertexColors")
		{
			if (!parseDataObjectMeshVertexColors(mesh))
				return false;
		}
		else
		if (objectName == "MeshMaterialList")
		{
			if (!parseDataObjectMeshMaterialList(mesh))
					return false;
		}
		else
		if (objectName == "VertexDuplicationIndices")
		{
			// we'll ignore vertex duplication indices
			// TODO: read them
			if (!parseUnknownDataObject())
				return false;
		}
		/*
		else
		if (objectName == "XSkinMeshHeader")
		{
			if (!parseDataObjectSkinMeshHeader(mesh.SkinMeshHeader))
				return false;
		}
		else
		if (objectName == "SkinWeights")
		{
			mesh.SkinWeights.push_back(SXSkinWeight());
			if (!parseDataObjectSkinWeights(mesh.SkinWeights.getLast()))
				return false;
		}
		*/
		else
		{
			os::Printer::log("Unknown data object in mesh in x file", objectName.c_str());
			if (!parseUnknownDataObject())
				return false;
		}
	}

	return true;

}


bool CXMeshFileLoader::parseDataObjectMeshNormals(SXMesh &mesh)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading mesh normals");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Normals found in x file", ELL_WARNING);
		return false;
	}

	CSkinnedMesh::SSkinMeshBuffer &MeshBuffer=*mesh.Buffer;


	// read count
	s32 nNormals;
	s32 count;
	nNormals = readInt();
	//normals.set_used(nNormals);

	// read normals
	if (binary)
	{
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: MeshNormals: Expecting float list", ELL_WARNING);
			return false;
		}
		count = readBinDWord();
		if (count != nNormals * 3)
		{
			os::Printer::log("Binary X: MeshNormals: Value count not equal to normal count", ELL_WARNING);
			return false;
		}
	}
	for (s32 i=0; i<nNormals; ++i)
		readVector3(MeshBuffer.Vertices_Standard[i].Normal);

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Normals Array found in x file", ELL_WARNING);
		return false;
	}


	core::array<s32> normalIndices;

	s32 triangulatedIndexCount=MeshBuffer.Indices.size();


	// read face normal indices
	s32 nFNormals = readInt();

	normalIndices.set_used(triangulatedIndexCount);


	s32 normalidx = 0;
	core::array<s32> polygonfaces;
	for (s32 k=0; k<nFNormals; ++k)
	{

		s32 fcnt = readInt();
		s32 triangles = fcnt - 2;
		s32 indexcount = triangles * 3;

		if (indexcount != mesh.IndexCountPerFace[k])
		{
			os::Printer::log("Not matching normal and face index count found in x file", ELL_WARNING);
			return false;
		}


		if (indexcount == 3)
		{
			// default, only one triangle in this face
			for (s32 h=0; h<3; ++h)
				normalIndices[normalidx++] = readInt();
		}
		else
		{
			// multiple triangles in this face
			polygonfaces.set_used(fcnt);

			for (s32 h=0; h<fcnt; ++h)
				polygonfaces[h] = readInt();

			for (s32 jk=0; jk<triangles; ++jk)
			{
				normalIndices[normalidx++] = polygonfaces[0];
				normalIndices[normalidx++] = polygonfaces[jk+1];
				normalIndices[normalidx++] = polygonfaces[jk+2];
			}
		}

	}


	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Face Normals Array found in x file", ELL_WARNING);
		return false;
	}
	if (getNextToken() != "}")
	{
		os::Printer::log("No closing brace in Mesh Normals found in x file", ELL_WARNING);
		return false;
	}

	return true;
}


bool CXMeshFileLoader::parseDataObjectMeshTextureCoords(SXMesh &mesh)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading mesh texture coordinates");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Texture Coordinates found in x file", ELL_WARNING);
		return false;
	}

	CSkinnedMesh::SSkinMeshBuffer &MeshBuffer=*mesh.Buffer;

	s32 nCoords;
	u32 count;
	nCoords = readInt();
	if (binary)
	{
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: MeshTextureCoords: Expecting float list", ELL_WARNING);
			return false;
		}
		count = readBinDWord();
	}
	//textureCoords.set_used(nCoords);

	for (s32 i=0; i<nCoords; ++i)
		readVector2(MeshBuffer.Vertices_Standard[i].TCoords);

	if (!checkForTwoFollowingSemicolons())
	{
		os::Printer::log("No finishing semicolon in Mesh Texture Coordinates Array found in x file", ELL_WARNING);
		return false;
	}

	if (getNextToken() != "}")
	{
		os::Printer::log("No closing brace in Mesh Texture Coordinates Array found in x file", ELL_WARNING);
		return false;
	}

	return true;
}




bool CXMeshFileLoader::parseDataObjectMeshVertexColors(SXMesh &mesh)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading mesh vertex colors");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace for Mesh Vertex Colors found in x file", ELL_WARNING);
		return false;
	}

	s32 nColors;
	u32 count;
	nColors = readInt();
	if (binary)
	{
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: MeshVertexColors: Expecting float list", ELL_WARNING);
			return false;
		}
		count = readBinDWord();
	}
	//vertexColors.set_used(nColors);

	for (s32 i=0; i<nColors; ++i)
	{
		u32 Index=readInt();
		if (Index>=mesh.Buffer->Vertices_Standard.size() )
			{
				os::Printer::log("index value in parseDataObjectMeshVertexColors out of bounds", ELL_WARNING);
				return false;
			}
		readRGBA(mesh.Buffer->Vertices_Standard[i].Color);
	}

	if (getNextToken() != ";")
	{
		os::Printer::log("No finishing semicolon in Mesh Vertex Colors Array found in x file", ELL_WARNING);
		return false;
	}

	if (getNextToken() != "}")
	{
		os::Printer::log("No closing brace in Mesh Texture Coordinates Array found in x file", ELL_WARNING);
		return false;
	}

	return true;
}



bool CXMeshFileLoader::parseDataObjectMeshMaterialList(SXMesh &mesh)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading mesh material list");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Material List found in x file", ELL_WARNING);
		return false;
	}




	// read material count
	readInt();

	// read non triangulated face material index count
	s32 nFaceIndices = readInt();

	// read non triangulated face indices

	core::array<s32> nonTriFaceIndices;
	nonTriFaceIndices.set_used(nFaceIndices);

	for (s32 i=0; i<nFaceIndices; ++i)
		nonTriFaceIndices[i] = readInt();

	// create triangulated face indices

	if (nFaceIndices != (s32)mesh.IndexCountPerFace.size())
	{
		os::Printer::log("Index count per face not equal to face material index count in x file.", ELL_WARNING);
		return false;
	}

	mesh.FaceIndices.set_used( mesh.Buffer->Indices.size() / 3);
	s32 triangulatedindex = 0;
	for (s32 tfi=0; tfi<nFaceIndices; ++tfi)
		for (s32 k=0; k<mesh.IndexCountPerFace[tfi]/3; ++k)
			mesh.FaceIndices[triangulatedindex++] = nonTriFaceIndices[tfi];

	// in version 03.02, the face indices end with two semicolons.
	// commented out version check, as version 03.03 exported from blender also has 2 semicolons
	if (!binary) // && MajorVersion == 3 && MinorVersion <= 2)
	{
		if (P[0] == ';')
			++P;
	}

	// read following data objects

	while(true)
	{
		core::stringc objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Mesh Material list in x file.", ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // material list finished
		}
		else
		if (objectName == "{")
		{
			// template materials now available thanks to joeWright
			objectName = getNextToken();
			for (u32 i=0; i<TemplateMaterials.size(); ++i)
				if (TemplateMaterials[i].Name == objectName)
					mesh.Materials.push_back(TemplateMaterials[i].Material);
			getNextToken(); // skip }
		}
		else
		if (objectName == "Material")
		{
			mesh.Materials.push_back(video::SMaterial());
			if (!parseDataObjectMaterial(mesh.Materials.getLast()))
				return false;
			mesh.Buffer->Material=mesh.Materials.getLast();
		}
		else
		if (objectName == ";")
		{
			// ignore
		}
		else
		{
			os::Printer::log("Unknown data object in material list in x file", objectName.c_str());
			if (!parseUnknownDataObject())
				return false;
		}
	}
	return true;
}



bool CXMeshFileLoader::parseDataObjectMaterial(video::SMaterial& material)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: Reading mesh material");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Mesh Material found in x file", ELL_WARNING);
		return false;
	}

	u32 count = 0;
	if (binary)
	{
		if (readBinWord() != 7)
		{
			os::Printer::log("Binary X: Material: Expecting float list", ELL_WARNING);
			return false;
		}
		count = readBinDWord();
		if (count != 11)
		{
			os::Printer::log("Binary X: Material: Float list length not equal to 11", ELL_WARNING);
			return false;
		}
	}

	// read RGBA
	readRGBA(material.DiffuseColor);

	// read power
	//material.Power = readFloat();
	readFloat();

	// read specular
	readRGB(material.SpecularColor);

	// read emissive
	readRGB(material.EmissiveColor);

	// read other data objects
	while(true)
	{
		core::stringc objectName = getNextToken();

		if (objectName.size() == 0)
		{
			os::Printer::log("Unexpected ending found in Mesh Material in x file.", ELL_WARNING);
			return false;
		}
		else
		if (objectName == "}")
		{
			break; // material finished
		}
		else
		if (objectName.equals_ignore_case("TextureFilename"))
		{
			// some exporters write "TextureFileName" instead.
			core::stringc TextureFileName;
			if (!parseDataObjectTextureFilename(TextureFileName))
				return false;

			TextureFileName=stripPathFromString(file->getFileName(),true) + stripPathFromString(TextureFileName,false);

			material.Textures[0]=SceneManager->getVideoDriver()->getTexture ( TextureFileName.c_str() );



		}
		else
		{
			os::Printer::log("Unknown data object in material in x file", objectName.c_str());
			if (!parseUnknownDataObject())
				return false;
		}

	}

	return true;
}








bool CXMeshFileLoader::parseDataObjectTextureFilename(core::stringc& texturename)
{
#ifdef _XREADER_DEBUG
	os::Printer::log("CXFileReader: reading texture filename");
#endif

	if (!readHeadOfDataObject())
	{
		os::Printer::log("No opening brace in Texture filename found in x file", ELL_WARNING);
		return false;
	}

	if (!getNextTokenAsString(texturename))
	{
		os::Printer::log("Unknown syntax while reading texture filename string in x file", ELL_WARNING);
		return false;
	}

	if (getNextToken() != "}")
	{
		os::Printer::log("No closing brace in Texture filename found in x file", ELL_WARNING);
		return false;
	}

	return true;
}





bool CXMeshFileLoader::parseUnknownDataObject()
{
	// find opening delimiter
	while(true)
	{
		core::stringc t = getNextToken();

		if (t.size() == 0)
			return false;

		if (t == "{")
			break;
	}

	s32 counter = 1;

	// parse until closing delimiter

	while(counter)
	{
		core::stringc t = getNextToken();

		if (t.size() == 0)
			return false;

		if (t == "{")
			++counter;
		else
		if (t == "}")
			--counter;
	}

	return true;
}



//! checks for two following semicolons, returns false if they are not there
bool CXMeshFileLoader::checkForTwoFollowingSemicolons()
{
	if (binary)
		return true;

	for (s32 k=0; k<2; ++k)
	{
		findNextNoneWhiteSpace();
		if (P[0] != ';')
			return false;
		++P;
	}

	return true;
}


//! reads header of dataobject including the opening brace.
//! returns false if error happened, and writes name of object
//! if there is one
bool CXMeshFileLoader::readHeadOfDataObject(core::stringc* outname)
{
	core::stringc nameOrBrace = getNextToken();
	if (nameOrBrace != "{")
	{
		if (outname)
			(*outname) = nameOrBrace;

		if (nameOrBrace.size() != 0 &&
			nameOrBrace[nameOrBrace.size()-1] == '{')
		{
			(*outname) = nameOrBrace.subString(0, nameOrBrace.size()-1);
			return true;
		}


		if (getNextToken() != "{")
			return false;
	}

	return true;
}


//! returns next parseable token. Returns empty string if no token there
core::stringc CXMeshFileLoader::getNextToken()
{
	core::stringc s;

	// process binary-formatted file
	if (binary)
	{
		// in binary mode it will only return NAME and STRING token
		// and (correctly) skip over other tokens.

		s16 tok = readBinWord();
		s32 len;

		// standalone tokens
		switch (tok) {
			case 1:
				// name token
				len = readBinDWord();
				s = core::stringc(P, len);
				P += len;
				return s;
			case 2:
				// string token
				len = readBinDWord();
				s = core::stringc(P, len);
				P += (len + 2);
				return s;
			case 3:
				// integer token
				P += 4;
				return "<integer>";
			case 5:
				// GUID token
				P += 16;
				return "<guid>";
			case 6:
				len = readBinDWord();
				P += (len * 4);
				return "<int_list>";
			case 7:
				len = readBinDWord();
				P += (len * FloatSize);
				return "<flt_list>";
			case 0x0a:
				return "{";
			case 0x0b:
				return "}";
			case 0x0c:
				return "(";
			case 0x0d:
				return ")";
			case 0x0e:
				return "[";
			case 0x0f:
				return "]";
			case 0x10:
				return "<";
			case 0x11:
				return ">";
			case 0x12:
				return ".";
			case 0x13:
				return ",";
			case 0x14:
				return ";";
			case 0x1f:
				return "template";
			case 0x28:
				return "WORD";
			case 0x29:
				return "DWORD";
			case 0x2a:
				return "FLOAT";
			case 0x2b:
				return "DOUBLE";
			case 0x2c:
				return "CHAR";
			case 0x2d:
				return "UCHAR";
			case 0x2e:
				return "SWORD";
			case 0x2f:
				return "SDWORD";
			case 0x30:
				return "void";
			case 0x31:
				return "string";
			case 0x32:
				return "unicode";
			case 0x34:
				return "array";
		}
	}
	// process text-formatted file
	else
	{
		findNextNoneWhiteSpace();

		if (P >= End)
			return s;

		while(P < End && !core::isspace(P[0]))
		{
			s.append(P[0]);
			++P;
		}
	}
	return s;
}


//! places pointer to next begin of a token, which must be a number,
// and ignores comments
void CXMeshFileLoader::findNextNoneWhiteSpaceNumber()
{
	if (binary)
		return;

	while(true)
	{
		while((P < End) && (P[0] != '-') && (P[0] != '.') &&
			!( core::isdigit(P[0])))
			++P;

		if (P >= End)
			return;

		// check if this is a comment
		if ((P[0] == '/' && P[1] == '/') || P[0] == '#')
			readUntilEndOfLine();
		else
			break;
	}

}

// places pointer to next begin of a token, and ignores comments
void CXMeshFileLoader::findNextNoneWhiteSpace()
{
	if (binary)
		return;

	while(true)
	{
		while(P < End && (P[0]==' ' || P[0]=='\n' || P[0]=='\r' || P[0]=='\t'))
			++P;

		if (P >= End)
			return;

		// check if this is a comment
		if ((P[0] == '/' && P[1] == '/') ||
			P[0] == '#')
			readUntilEndOfLine();
		else
			break;
	}
}


//! reads a x file style string
bool CXMeshFileLoader::getNextTokenAsString(core::stringc& out)
{
	if (binary)
	{
		out=getNextToken();
		return true;
	}
	findNextNoneWhiteSpace();

	if (P >= End)
		return false;

	if (P[0] != '"')
		return false;
	++P;

	while(P < End && P[0]!='"')
	{
		out.append(P[0]);
		++P;
	}

	if ( P[1] != ';' || P[0] != '"')
		return false;
	P+=2;

	return true;
}


void CXMeshFileLoader::readUntilEndOfLine()
{
	if (binary)
		return;

	while(P < End)
	{
		if (P[0] == '\n')
		{
			++P;
			return;
		}

		++P;
	}
}









u16 CXMeshFileLoader::readBinWord()
{
	u8 *Q = (u8 *)P;
	u16 tmp = 0;
	tmp = Q[0] + (Q[1] << 8);
	P += 2;
	return tmp;
}

u32 CXMeshFileLoader::readBinDWord()
{
	u8 *Q = (u8 *)P;
	u32 tmp = 0;
	tmp = Q[0] + (Q[1] << 8) + (Q[2] << 16) + (Q[3] << 24);
	P += 4;
	return tmp;
}

s32 CXMeshFileLoader::readInt()
{
	if (binary)
	{
		if (!binaryNumCount)
		{
			readBinWord(); // 0x06
			binaryNumCount=readBinDWord(); // 0x0001
		}
		--binaryNumCount;
		return readBinDWord();
	}
	else
	{
		f32 ftmp;
		findNextNoneWhiteSpaceNumber();
		P = core::fast_atof_move(P, ftmp);
		return (s32)ftmp;
	}
}

f32 CXMeshFileLoader::readFloat()
{
	if (binary)
	{
		if (FloatSize == 8)
		{
			char tmp[8];
			memcpy(tmp, P, 8);
			P += 8;
			return (f32)(*(f64 *)tmp);
		}
		else
		{
			char tmp[4];
			memcpy(tmp, P, 4);
			P += 4;
			return *(f32 *)tmp;
		}
	}
	findNextNoneWhiteSpaceNumber();
	f32 ftmp;
	P = core::fast_atof_move(P, ftmp);
	return ftmp;
}

// read 2-dimensional vector. Stops at semicolon after second value for text file format
bool CXMeshFileLoader::readVector2(core::vector2df& vec)
{
	vec.X = readFloat();
	vec.Y = readFloat();
	return true;
}

// read 3-dimensional vector. Stops at semicolon after third value for text file format
bool CXMeshFileLoader::readVector3(core::vector3df& vec)
{
	vec.X = readFloat();
	vec.Y = readFloat();
	vec.Z = readFloat();
	return true;
}

// read color without alpha value. Stops after second semicolon after blue value
bool CXMeshFileLoader::readRGB(video::SColorf& color)
{
	color.r = readFloat();
	color.g = readFloat();
	color.b = readFloat();
	color.a = 1.0f;
	return checkForTwoFollowingSemicolons();
}


// read color with alpha value. Stops after second semicolon after blue value
bool CXMeshFileLoader::readRGBA(video::SColorf& color)
{
	color.r = readFloat();
	color.g = readFloat();
	color.b = readFloat();
	color.a = readFloat();
	return checkForTwoFollowingSemicolons();
}




// read color without alpha value. Stops after second semicolon after blue value
bool CXMeshFileLoader::readRGB(video::SColor& color)
{
	color.setRed( (u32)(readFloat()*255)) ;
	color.setGreen( (u32)(readFloat()*255)) ;
	color.setBlue( (u32)(readFloat()*255)) ;
	color.setAlpha( 255 );
	return checkForTwoFollowingSemicolons();
}


// read color with alpha value. Stops after second semicolon after blue value
bool CXMeshFileLoader::readRGBA(video::SColor& color)
{
	color.setRed( (u32)(readFloat()*255)) ;
	color.setGreen( (u32)(readFloat()*255)) ;
	color.setBlue( (u32)(readFloat()*255)) ;
	color.setAlpha( (u32)(readFloat()*255)) ;
	return checkForTwoFollowingSemicolons();
}



core::stringc CXMeshFileLoader::stripPathFromString(core::stringc string, bool returnPath)
{
	s32 slashIndex=string.findLast('/'); // forward slash
	s32 backSlash=string.findLast('\\'); // back slash

	if (backSlash>slashIndex) slashIndex=backSlash;

	if (slashIndex==-1)//no slashes found
		if (returnPath)
			return core::stringc(); //no path to return
		else
			return string;

	if (returnPath)
		return string.subString(0, slashIndex + 1);
	else
		return string.subString(slashIndex+1, string.size() - (slashIndex+1));
}





} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_X_LOADER_


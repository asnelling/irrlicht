// Copyright (C) 2002-2006 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSkinnedMesh.h"
#include "IBoneSceneNode.h"
#include "IAnimatedMeshSceneNode.h"
#include "os.h"


////////////////////// TEMP /////////////////////////////
#include "iostream"
using namespace std;
/////////////////////////////////////////////////////////



namespace irr
{
namespace scene
{


//! constructor
CSkinnedMesh::CSkinnedMesh()
: HasAnimation(0), AnimationFrames(0), lastAnimatedFrame(0), lastSkinnedFrame(0), AnimateNormals(0),
	InterpolationMode(EIM_LINEAR), Buffers(), RootJoints(), AllJoints()
{

	#ifdef _DEBUG
	setDebugName("CSkinnedMesh");
	#endif

}


//! destructor
CSkinnedMesh::~CSkinnedMesh()
{
	s32 n;

	for (n=AllJoints.size()-1;n>=0;--n)
	{
		//delete AllJoints[n];
		AllJoints.erase(n);
	}

	for (n=Buffers.size()-1;n>=0;--n)
	{
		//delete Buffers[n];
		Buffers.erase(n);
	}

}


//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
s32 CSkinnedMesh::getFrameCount()
{
	return (s32)AnimationFrames;
}

//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CSkinnedMesh::getMesh(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	//animate(frame,startFrameLoop, endFrameLoop);
	if (frame==-1) return this;

	animateMesh((f32)frame, 1.0f);
	buildAll_LocalAnimatedMatrices();
	buildAll_GlobalAnimatedMatrices();
	skinMesh();
	return this;
}



//--------------------------------------------------------------------------
//								Keyframe Animation
//--------------------------------------------------------------------------

//! Animates this mesh's joints based on frame input
//! blend: {0-old position, 1-New position}
void CSkinnedMesh::animateMesh(f32 frame, f32 blend)
{
	if ( !HasAnimation || lastAnimatedFrame==frame)
		return;

	lastAnimatedFrame=frame;

	if (blend<=0) return; //No need to animate


	for (u32 i=0; i<AllJoints.size(); ++i)
	{
		//To Bitplane: The joints can be animated here with no input from there parents, but for setAnimationMode extra check are needed to their parents


		SJoint *Joint = AllJoints[i];

		//hmmm, sure I blend the position/scale/rotation or matrixes...
		//position/scale/rotation is probably the best method has animation data less likely to be blended over
		//top of user data, it's the under way around

		core::vector3df oldPosition = Joint->_Animatedposition;
		core::vector3df oldScale = Joint->_Animatedscale;
		core::quaternion oldRotation = Joint->_Animatedrotation;

		core::vector3df position =oldPosition;
		core::vector3df scale =oldScale;
		core::quaternion rotation =oldRotation;

		//Should be from IJointSceneNode
		s32 positionHint, scaleHint, rotationHint;
		positionHint=scaleHint=rotationHint=-1; //Todo: supply function with real hints


		getFrameData(frame, Joint,
					position, positionHint,
					scale, scaleHint,
					rotation, rotationHint);

		if (blend==1.0f)
		{
			//No blending need:
			Joint->_Animatedposition = position;
			Joint->_Animatedscale = scale;
			Joint->_Animatedrotation = rotation;
		}
		else
		{
			//Blend animation:

			f32 invBlend=1-blend;

			Joint->_Animatedposition = (position * blend) + (oldPosition* invBlend );
			Joint->_Animatedscale = (scale * blend) + (oldScale* invBlend );
			Joint->_Animatedrotation.slerp(oldRotation, rotation, blend);

		}

		//Node:
		//_LocalAnimatedMatrix needs to be built at some point, but this function maybe called lots of times for
		//one render (to play two animations at the same time) _LocalAnimatedMatrix only needs to be built once.
		//a call to buildAllLocalAnimatedMatrices is needed before skinning the mesh, and before the user gets the joints to move

	}

}

void CSkinnedMesh::buildAll_LocalAnimatedMatrices()
{
	for (u32 i=0; i<AllJoints.size(); ++i)
	{
		SJoint *Joint = AllJoints[i];

		//Could be faster:

		if (Joint->PositionKeys.size() ||Joint->ScaleKeys.size() || Joint->RotationKeys.size())
		{
			Joint->_LocalAnimatedMatrix.makeIdentity();
			Joint->_LocalAnimatedMatrix.setTranslation(Joint->_Animatedposition);
			Joint->_LocalAnimatedMatrix*=Joint->_Animatedrotation.getMatrix();

			if (Joint->ScaleKeys.size())
			{
				//Joint->_LocalAnimatedMatrix.setScale(Joint->_Animatedscale);
				core::matrix4 scaleMatrix;
				scaleMatrix.setScale(Joint->_Animatedscale);
				Joint->_LocalAnimatedMatrix *= scaleMatrix;
			}
		}
		else
		{
			Joint->_LocalAnimatedMatrix=Joint->LocalMatrix;
		}

	}
}

void CSkinnedMesh::buildAll_GlobalAnimatedMatrices(SJoint *Joint, SJoint *ParentJoint)
{

	if (!Joint)
	{
		for (u32 i=0; i<RootJoints.size(); ++i)
			buildAll_GlobalAnimatedMatrices(RootJoints[i], 0);
		return;
	}
	else
	{
		// Find global matrix...
		if (!ParentJoint)
			Joint->_GlobalAnimatedMatrix = Joint->_LocalAnimatedMatrix;
		else
			Joint->_GlobalAnimatedMatrix = ParentJoint->_GlobalAnimatedMatrix * Joint->_LocalAnimatedMatrix;
	}

	for (u32 j=0; j<Joint->Children.size(); ++j)
		buildAll_GlobalAnimatedMatrices(Joint->Children[j], Joint);
}

void CSkinnedMesh::getFrameData(f32 frame, SJoint *Joint,
								core::vector3df &position, s32 &positionHint,
								core::vector3df &scale, s32 &scaleHint,
								core::quaternion &rotation, s32 &rotationHint)
{


	s32 foundPositionIndex = -1;
	s32 foundScaleIndex = -1;
	s32 foundRotationIndex = -1;


	if (Joint->PositionKeys.size())
	{
		foundPositionIndex = -1;

		//Test the Hints...
		if (positionHint>=0 && positionHint < (s32)Joint->PositionKeys.size())
		{
			//check this hint
			if (Joint->PositionKeys[positionHint].frame>=frame &&
						(positionHint+1 >= (s32)Joint->PositionKeys.size()-1 || Joint->PositionKeys[positionHint+1].frame<frame))
				foundPositionIndex=positionHint;

			//check the next index
			else if ( (positionHint+1 < (s32)Joint->PositionKeys.size()-1 && Joint->PositionKeys[positionHint+1].frame>=frame) &&
						(positionHint+2 >=(s32)Joint->PositionKeys.size()-1 || Joint->PositionKeys[positionHint+2].frame<frame))
				{
					foundPositionIndex=positionHint+1;
					positionHint++;
				}
		}

		//The hint test failed, do a full scan...
		if (foundPositionIndex==-1)
		{
			for (u32 i=0; i<Joint->PositionKeys.size(); ++i)
			{
				if (Joint->PositionKeys[i].frame >= frame) //Keys should to be sorted by frame
				{
					foundPositionIndex=i;
					positionHint=i;
					break;
				}
			}
		}

		//Do interpolation...
		if (foundPositionIndex!=-1)
		{
			if (InterpolationMode==EIM_CONSTANT || foundPositionIndex==0)
			{
				position = Joint->PositionKeys[foundPositionIndex].position;
			}
			else if (InterpolationMode==EIM_LINEAR)
			{
				SPositionKey *KeyA = &Joint->PositionKeys[foundPositionIndex];
				SPositionKey *KeyB = &Joint->PositionKeys[foundPositionIndex-1];

				f32 fd1 = frame-KeyA->frame;
				f32 fd2 = KeyB->frame-frame;
				position = ((KeyB->position-KeyA->position)/(fd1+fd2))*fd1 + KeyA->position;
			}
		}
	}

	//-------------------------------------------------------------------------

	if (Joint->ScaleKeys.size())
	{
		foundScaleIndex = -1;

		//Test the Hints...
		if (scaleHint>=0 && scaleHint < (s32)Joint->ScaleKeys.size())
		{
			//check this hint
			if (Joint->ScaleKeys[scaleHint].frame>=frame &&
						(scaleHint+1 >= (s32)Joint->ScaleKeys.size()-1 || Joint->ScaleKeys[scaleHint+1].frame<frame))
				foundScaleIndex=scaleHint;

			//check the next index
			else if ( (scaleHint+1 < (s32)Joint->ScaleKeys.size()-1 && Joint->ScaleKeys[scaleHint+1].frame>=frame) &&
						(scaleHint+2 >=(s32)Joint->ScaleKeys.size()-1 || Joint->ScaleKeys[scaleHint+2].frame<frame))
			{
				foundScaleIndex=scaleHint+1;
				scaleHint++;
			}
		}

		//The hint test failed, do a full scan...
		if (foundScaleIndex==-1)
		{
			for (u32 i=0; i<Joint->ScaleKeys.size(); ++i)
			{
				if (Joint->ScaleKeys[i].frame >= frame) //Keys should to be sorted by frame
				{
					foundScaleIndex=i;
					scaleHint=i;
					break;
				}
			}
		}

		//Do interpolation...
		if (foundScaleIndex!=-1)
		{
			if (InterpolationMode==EIM_CONSTANT || foundScaleIndex==0)
			{
				scale = Joint->ScaleKeys[foundScaleIndex].scale;
			}
			else if (InterpolationMode==EIM_LINEAR)
			{
				SScaleKey *KeyA = &Joint->ScaleKeys[foundScaleIndex];
				SScaleKey *KeyB = &Joint->ScaleKeys[foundScaleIndex-1];

				f32 fd1 = frame-KeyA->frame;
				f32 fd2 = KeyB->frame-frame;
				scale = ((KeyB->scale-KeyA->scale)/(fd1+fd2))*fd1 + KeyA->scale;
			}
		}
	}

	//-------------------------------------------------------------------------

	if (Joint->RotationKeys.size())
	{
		foundRotationIndex = -1;

		//Test the Hints...
		if (rotationHint>=0 && rotationHint < (s32)Joint->RotationKeys.size())
		{
			//check this hint
			if (Joint->RotationKeys[rotationHint].frame>=frame &&
						(rotationHint+1 >= (s32)Joint->RotationKeys.size()-1 || Joint->RotationKeys[rotationHint+1].frame<frame))
				foundRotationIndex=rotationHint;

			//check the next index
			else if ( (rotationHint+1 < (s32)Joint->RotationKeys.size()-1 && Joint->RotationKeys[rotationHint+1].frame>=frame) &&
						(rotationHint+2 >=(s32)Joint->RotationKeys.size()-1 || Joint->RotationKeys[rotationHint+2].frame<frame))
			{
				foundRotationIndex=rotationHint+1;
				rotationHint++;
			}
		}

		//The hint test failed, do a full scan...
		if (foundRotationIndex==-1)
		{
			for (u32 i=0; i<Joint->RotationKeys.size(); ++i)
			{
				if (Joint->RotationKeys[i].frame >= frame) //Keys should to be sorted by frame
				{
					foundRotationIndex=i;
					rotationHint=i;
					break;
				}
			}
		}

		//Do interpolation...
		if (foundRotationIndex!=-1)
		{
			if (InterpolationMode==EIM_CONSTANT || foundRotationIndex==0)
			{
				rotation = Joint->RotationKeys[foundRotationIndex].rotation;
			}
			else if (InterpolationMode==EIM_LINEAR)
			{
				SRotationKey *KeyA = &Joint->RotationKeys[foundRotationIndex];
				SRotationKey *KeyB = &Joint->RotationKeys[foundRotationIndex-1];

				f32 fd1 = frame-KeyA->frame;
				f32 fd2 = KeyB->frame - frame;
				f32 t = (1.0f/(fd1+fd2))*fd1;

				/*
				f32 t = 0;
				if (KeyA->frame!=KeyB->frame)
					t = (frame-KeyA->frame) / (KeyB->frame - KeyA->frame);
				*/

				rotation.slerp(KeyA->rotation, KeyB->rotation, t);

			}
		}
	}

}

//--------------------------------------------------------------------------
//								Software Skinning
//--------------------------------------------------------------------------


//! Preforms a software skin on this mesh based of joint positions
void CSkinnedMesh::skinMesh()
{
	//Software skin....


	//clear skinning helper array
	u32 i, j;
	for (i=0; i<Vertices_Moved.size(); ++i)
		for (j=0; j<Vertices_Moved[i].size(); ++j)
			Vertices_Moved[i][j]=false;

	//skin starting with the root joints
	for (i=0; i<RootJoints.size(); ++i)
		SkinJoint(RootJoints[i], 0);

}

void CSkinnedMesh::SkinJoint(SJoint *Joint, SJoint *ParentJoint)
{


	if (Joint->Weights.size())
	{
		//Find this joints pull on vertices...
		core::matrix4 JointVertexPull(core::matrix4::EM4CONST_NOTHING);
		JointVertexPull.setbyproduct(Joint->_GlobalAnimatedMatrix, Joint->_GlobalInversedMatrix);

		core::vector3df ThisVertexMove, ThisNormalMove;

		SWeight *Weight;

		//Skin Vertices Normals...

		//Skin Vertices Positions...
		for (u32 i=0; i<Joint->Weights.size(); ++i)
		{
			Weight=&Joint->Weights[i];

			// Pull this vertex...
			ThisVertexMove.X = JointVertexPull[0]*Weight->_StaticPos.X + JointVertexPull[4]*Weight->_StaticPos.Y + JointVertexPull[8]*Weight->_StaticPos.Z + JointVertexPull[12];
			ThisVertexMove.Y = JointVertexPull[1]*Weight->_StaticPos.X + JointVertexPull[5]*Weight->_StaticPos.Y + JointVertexPull[9]*Weight->_StaticPos.Z + JointVertexPull[13];
			ThisVertexMove.Z = JointVertexPull[2]*Weight->_StaticPos.X + JointVertexPull[6]*Weight->_StaticPos.Y + JointVertexPull[10]*Weight->_StaticPos.Z + JointVertexPull[14];

			if (AnimateNormals)
			{
				ThisNormalMove.X = JointVertexPull[0]*Weight->_StaticNormal.X + JointVertexPull[4]*Weight->_StaticNormal.Y + JointVertexPull[8]*Weight->_StaticNormal.Z;
				ThisNormalMove.Y = JointVertexPull[1]*Weight->_StaticNormal.X + JointVertexPull[5]*Weight->_StaticNormal.Y + JointVertexPull[9]*Weight->_StaticNormal.Z;
				ThisNormalMove.Z = JointVertexPull[2]*Weight->_StaticNormal.X + JointVertexPull[6]*Weight->_StaticNormal.Y + JointVertexPull[10]*Weight->_StaticNormal.Z;
			}


			if (! (*Weight->_Moved) )
			{
				(*Weight->_Moved) = true;

				Buffers[Weight->buffer_id]->getVertex(Weight->vertex_id)->Pos = (ThisVertexMove * Weight->strength);

				if (AnimateNormals) Buffers[Weight->buffer_id]->getVertex(Weight->vertex_id)->Normal = (ThisNormalMove * Weight->strength);

				//(*Weight->_Pos) = ThisVertexMove * Weight->strength;
			}
			else
			{
				Buffers[Weight->buffer_id]->getVertex(Weight->vertex_id)->Pos += (ThisVertexMove* Weight->strength);

				if (AnimateNormals) Buffers[Weight->buffer_id]->getVertex(Weight->vertex_id)->Normal += (ThisNormalMove * Weight->strength);

				//(*Weight->_Pos) += (ThisVertexMove * Weight->strength);

			}

		}

	}

	//Skin all children
	for (u32 j=0; j<Joint->Children.size(); ++j)
		SkinJoint(Joint->Children[j], Joint);

}

E_ANIMATED_MESH_TYPE CSkinnedMesh::getMeshType() const
{
	return EAMT_SKINNED;
}

//! Gets joint count.
s32 CSkinnedMesh::getJointCount() const
{
	return AllJoints.size();
}


//! Gets the name of a joint.
const c8* CSkinnedMesh::getJointName(s32 number) const
{
	if (number < 0 || number >= (s32)AllJoints.size())
		return 0;
	return AllJoints[number]->Name.c_str();
}



//! Gets a joint number from its name
s32 CSkinnedMesh::getJointNumber(const c8* name) const
{
	for (s32 i=0; i<(s32)AllJoints.size(); ++i)
		if (AllJoints[i]->Name == name)
			return i;

	return -1;
}



//! returns amount of mesh buffers.
u32 CSkinnedMesh::getMeshBufferCount() const
{
	return Buffers.size();
}


//! returns pointer to a mesh buffer
IMeshBuffer* CSkinnedMesh::getMeshBuffer(u32 nr) const
{
	if (nr < Buffers.size())
		return Buffers[nr];
	else
		return 0;
}

//! Returns pointer to a mesh buffer which fits a material
IMeshBuffer* CSkinnedMesh::getMeshBuffer(const video::SMaterial &material) const
{
	for (u32 i=0; i<Buffers.size(); ++i)
	{
		if (Buffers[i]->getMaterial() == material)
			return Buffers[i];
	}
	return 0;
}


//! returns an axis aligned bounding box
const core::aabbox3d<f32>& CSkinnedMesh::getBoundingBox() const
{
	return BoundingBox;
}

//! set user axis aligned bounding box
void CSkinnedMesh::setBoundingBox( const core::aabbox3df& box)
{
	BoundingBox = box;
}

//! sets a flag of all contained materials to a new value
void CSkinnedMesh::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
	for (s32 i=0; i<(s32)Buffers.size(); ++i)
		Buffers[i]->Material.setFlag(flag,newvalue);
}

//!Update Normals when Animating
//!False= Don't (default)
//!True= Update normals, slower
void CSkinnedMesh::updateNormalsWhenAnimating(bool on)
{
	AnimateNormals = on;
}

//!Sets Interpolation Mode
void CSkinnedMesh::setInterpolationMode(E_INTERPOLATION_MODE mode)
{
	InterpolationMode = mode;
}


core::array<CSkinnedMesh::SSkinMeshBuffer*> &CSkinnedMesh::getMeshBuffers()
{
	return Buffers;
}

core::array<CSkinnedMesh::SJoint*> &CSkinnedMesh::getAllJoints()
{
	return AllJoints;
}


void CSkinnedMesh::CalculateGlobalMatrixes(SJoint *Joint,SJoint *ParentJoint)
{
	if (!Joint && ParentJoint) // bit of protection from endless loops
		return;

	//Go thought the root bones
	if (!Joint)
	{
		for (u32 i=0; i<RootJoints.size(); ++i)
			CalculateGlobalMatrixes(RootJoints[i],0);
		return;
	}

	if (!ParentJoint)
		Joint->_GlobalMatrix = Joint->LocalMatrix;
	else
		Joint->_GlobalMatrix = ParentJoint->_GlobalMatrix * Joint->LocalMatrix;

	Joint->_LocalAnimatedMatrix=Joint->LocalMatrix;
	Joint->_GlobalAnimatedMatrix=Joint->_GlobalMatrix;

	Joint->_GlobalInversedMatrix = Joint->_GlobalMatrix;
	Joint->_GlobalInversedMatrix.makeInverse(); // slow


	for (u32 j=0; j<Joint->Children.size(); ++j)
		CalculateGlobalMatrixes(Joint->Children[j],Joint);
}



//! called by loader after populating with mesh and bone data
void CSkinnedMesh::finalize()
{



	u32 i=0,j=0,k=0;

	lastAnimatedFrame=-1;
	lastSkinnedFrame=-1;
	AnimateNormals=false;

	if (AllJoints.size() || RootJoints.size())
	{
		// populate AllJoints or RootJoints, depending on which is empty
		if (!RootJoints.size())
		{

			for(u32 CheckingIdx=0; CheckingIdx < AllJoints.size(); ++CheckingIdx)
			{

				bool foundParent=false;
				for(i=0; i < AllJoints.size(); ++i)
				{
					for(u32 n=0; n < AllJoints[i]->Children.size(); ++n)
					{
						if (AllJoints[i]->Children[n] == AllJoints[CheckingIdx])
							foundParent=true;
					}
				}

				if (!foundParent)
					RootJoints.push_back(AllJoints[CheckingIdx]);
			}
		}
		else
		{
			AllJoints=RootJoints;
		}
	}

	//Set array sizes...



	for (i=0; i<Buffers.size(); ++i)
	{
		Vertices_Moved.push_back( core::array<bool>() );
		Vertices_Moved[i].set_used(Buffers[i]->getVertexCount());
	}


	//Todo: optimise keys here...




	//Check for animation...
	HasAnimation = false;
	for(i=0;i<AllJoints.size();++i)
	{
		if (AllJoints[i]->PositionKeys.size() ||
			AllJoints[i]->ScaleKeys.size() ||
			AllJoints[i]->RotationKeys.size() )
		{
			HasAnimation = true;
		}
	}


	if (HasAnimation)
	{

		//--- Find the length of the animation ---
		AnimationFrames=0;
		for(i=0;i<AllJoints.size();++i)
		{
			if (AllJoints[i]->PositionKeys.size())
				if (AllJoints[i]->PositionKeys.getLast().frame > AnimationFrames)
					AnimationFrames=AllJoints[i]->PositionKeys.getLast().frame;

			if (AllJoints[i]->ScaleKeys.size())
				if (AllJoints[i]->ScaleKeys.getLast().frame > AnimationFrames)
					AnimationFrames=AllJoints[i]->ScaleKeys.getLast().frame;

			if (AllJoints[i]->RotationKeys.size())
				if (AllJoints[i]->RotationKeys.getLast().frame > AnimationFrames)
					AnimationFrames=AllJoints[i]->RotationKeys.getLast().frame;
		}


		//--- optimize and check keyframes ---
		for(i=0;i<AllJoints.size();++i)
		{
			core::array<SPositionKey> &PositionKeys =AllJoints[i]->PositionKeys;
			core::array<SScaleKey> &ScaleKeys = AllJoints[i]->ScaleKeys;
			core::array<SRotationKey> &RotationKeys = AllJoints[i]->RotationKeys;



			if (PositionKeys.size()>2)
				for(j=0;j<PositionKeys.size()-2;++j)
				{
					if (PositionKeys[j].position == PositionKeys[j+1].position && PositionKeys[j+1].position == PositionKeys[j+2].position)
					{
						PositionKeys.erase(j+1); //the middle key is unneeded
						j--;
					}
				}

		if (PositionKeys.size()>1)
			for(j=0;j<PositionKeys.size()-1;++j)
			{
				if (PositionKeys[j].frame >= PositionKeys[j+1].frame) //bad frame, unneed and may cause problems
				{
					PositionKeys.erase(j+1);
					j--;
				}
			}

		if (ScaleKeys.size()>2)
			for(j=0;j<ScaleKeys.size()-2;++j)
			{
				if (ScaleKeys[j].scale == ScaleKeys[j+1].scale && ScaleKeys[j+1].scale == ScaleKeys[j+2].scale)
				{
					ScaleKeys.erase(j+1); //the middle key is unneeded
					j--;
				}
			}
	if (ScaleKeys.size()>1)
			for(j=0;j<ScaleKeys.size()-1;++j)
			{
				if (ScaleKeys[j].frame >= ScaleKeys[j+1].frame) //bad frame, unneed and may cause problems
				{
					ScaleKeys.erase(j+1);
					j--;
				}
			}

		if (RotationKeys.size()>2)
			for(j=0;j<RotationKeys.size()-2;++j)
			{
				if (RotationKeys[j].rotation == RotationKeys[j+1].rotation && RotationKeys[j+1].rotation == RotationKeys[j+2].rotation)
				{
					RotationKeys.erase(j+1); //the middle key is unneeded
					j--;
				}
			}

		if (RotationKeys.size()>1)
			for(j=0;j<RotationKeys.size()-1;++j)
			{
				if (RotationKeys[j].frame >= RotationKeys[j+1].frame) //bad frame, unneed and may cause problems
				{
					RotationKeys.erase(j+1);
					j--;
				}
			}

		}





	}


	//Needed for animation and skinning...
	CalculateGlobalMatrixes(0,0);


	if (HasAnimation)
	{




		//check for bugs:
		for(i=0; i < AllJoints.size(); ++i)
		{
			SJoint *Joint = AllJoints[i];
			for (j=0; j<Joint->Weights.size(); ++j)
			{
				u16 buffer_id=Joint->Weights[j].buffer_id;
				u32 vertex_id=Joint->Weights[j].vertex_id;

				//check for invalid ids
				if (buffer_id>=Buffers.size())
				{
					os::Printer::log("Skinned Mesh: Weight buffer id too large");
					Joint->Weights[j].buffer_id = Joint->Weights[j].vertex_id =0;
				}
				else if (vertex_id>=Buffers[buffer_id]->getVertexCount())
				{
					os::Printer::log("Skinned Mesh: Weight vertex id too large");
					Joint->Weights[j].buffer_id = Joint->Weights[j].vertex_id =0;
				}

			}
		}

		//An array used in skinning

		for (i=0; i<Vertices_Moved.size(); ++i)
			for (j=0; j<Vertices_Moved[i].size(); ++j)
				Vertices_Moved[i][j] = false;

		// For skinning: cache weight values for speed

		for (i=0; i<AllJoints.size(); ++i)
		{
			SJoint *Joint = AllJoints[i];
			for (j=0; j<Joint->Weights.size(); ++j)
			{
				u32 vertex_id=Joint->Weights[j].vertex_id;
				u32 buffer_id=Joint->Weights[j].buffer_id;

				Joint->Weights[j]._Moved = &Vertices_Moved[buffer_id] [vertex_id];
				Joint->Weights[j]._StaticPos = Buffers[buffer_id]->getVertex(vertex_id)->Pos;
				Joint->Weights[j]._StaticNormal = Buffers[buffer_id]->getVertex(vertex_id)->Normal;

				//Joint->Weights[j]._Pos=&Buffers[buffer_id]->getVertex(vertex_id)->Pos;
			}
		}

		// normalize weights
		normalizeWeights();



	}


}


CSkinnedMesh::SSkinMeshBuffer *CSkinnedMesh::createBuffer()
{
	SSkinMeshBuffer *buffer=new SSkinMeshBuffer;
	Buffers.push_back(buffer);
	buffer->VertexType=video::EVT_STANDARD;
	return buffer;
}

CSkinnedMesh::SJoint *CSkinnedMesh::createJoint(SJoint *parent)
{
	SJoint *joint=new SJoint;

	AllJoints.push_back(joint);
	if (!parent)
	{
		//Add root joints to array in finalize()
	}
	else
	{
		//Set parent (Be careful  of the mesh loader also setting the parent)
		parent->Children.push_back(joint);
	}
	return joint;
}

CSkinnedMesh::SPositionKey *CSkinnedMesh::createPositionKey(SJoint *joint)
{
	if (!joint) return 0;
	SPositionKey *key;

	joint->PositionKeys.push_back(SPositionKey());
	key=&joint->PositionKeys.getLast();

	key->frame=0;
	return key;
}

CSkinnedMesh::SScaleKey *CSkinnedMesh::createScaleKey(SJoint *joint)
{
	if (!joint) return 0;
	SScaleKey *key;

	joint->ScaleKeys.push_back(SScaleKey());
	key=&joint->ScaleKeys.getLast();

	return key;
}

CSkinnedMesh::SRotationKey *CSkinnedMesh::createRotationKey(SJoint *joint)
{
	if (!joint) return 0;
	SRotationKey *key;

	joint->RotationKeys.push_back(SRotationKey());
	key=&joint->RotationKeys.getLast();

	return key;
}





CSkinnedMesh::SWeight *CSkinnedMesh::createWeight(SJoint *joint)
{
	if (!joint) return 0;

	joint->Weights.push_back(SWeight());

	SWeight *weight=&joint->Weights.getLast();

	//Could do stuff here...

	return weight;
}


void CSkinnedMesh::normalizeWeights()
{

	// node: unsure if weights ids are going to be used.


	// Normalise the weights on bones....

	u32 i,j;
	core::array< core::array<f32> > Vertices_TotalWeight;

	for (i=0; i<Buffers.size(); ++i)
	{
		Vertices_TotalWeight.push_back(core::array<f32>());
		Vertices_TotalWeight[i].set_used(Buffers[i]->getVertexCount());
	}


	for (i=0; i<Vertices_TotalWeight.size(); ++i)
		for (j=0; j<Vertices_TotalWeight[i].size(); ++j)
			Vertices_TotalWeight[i][j] = 0;

	for (i=0; i<AllJoints.size(); ++i)
	{
		SJoint *Joint=AllJoints[i];
		for (j=0; j<Joint->Weights.size(); ++j)
		{
			if (Joint->Weights[j].strength<=0)//Check for invalid weights
			{
				Joint->Weights.erase(j);
				j--;
			}
			else
			{
				Vertices_TotalWeight[ Joint->Weights[j].buffer_id ] [ Joint->Weights[j].vertex_id ] += Joint->Weights[j].strength;
			}
		}
	}

	for (i=0; i<AllJoints.size(); ++i)
	{
		SJoint *Joint=AllJoints[i];
		for (j=0; j< Joint->Weights.size(); ++j)
		{
			f32 total = Vertices_TotalWeight[ Joint->Weights[j].buffer_id ] [ Joint->Weights[j].vertex_id ];
			if (total != 0 && total != 1)
				Joint->Weights[j].strength /= total;
		}
	}

}


void CSkinnedMesh::recoverJointsFromMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes)
{
	//Note: This function works because of the way the b3d fomat nests nodes, other mesh loaders may need a different function
	for (s32 i=0;i<(s32)RootJoints.size();++i)
	{
		IBoneSceneNode* node=JointChildSceneNodes[i];
		SJoint *joint=RootJoints[i];
		node->setPosition( joint->_LocalAnimatedMatrix.getTranslation() );
		node->setRotation( joint->_LocalAnimatedMatrix.getRotationDegrees() );
		//node->setScale( B3dNode->LocalAnimatedMatrix.getScale() );
		//node->updateAbsolutePosition();//works because of nests nodes
	}
}

void CSkinnedMesh::tranferJointsToMesh(core::array<IBoneSceneNode*> &JointChildSceneNodes)
{
	for (s32 i=0;i<(s32)RootJoints.size();++i)
	{
		IBoneSceneNode* node=JointChildSceneNodes[i];
		SJoint *joint=RootJoints[i];
		joint->_LocalAnimatedMatrix.setTranslation( node->getPosition() );
		joint->_LocalAnimatedMatrix.setRotationDegrees( node->getRotation() );
		//B3dNode->LocalAnimatedMatrix.setScale( node->getScale() );
	}
	//Remove cache, temp...
	lastAnimatedFrame=-1;
	lastSkinnedFrame=-1;
}



void CSkinnedMesh::createJoints(core::array<IBoneSceneNode*> &JointChildSceneNodes,
	IAnimatedMeshSceneNode* AnimatedMeshSceneNode, ISceneManager* SceneManager)
{
	//Note: This function works because of the way the b3d format nests nodes, other mesh loaders may need a different function

	//createSkelton_Helper(SceneManager, JointChildSceneNodes,AnimatedMeshSceneNode,0,0,0);

}


void CSkinnedMesh::convertMeshToTangents()
{

	// now calculate tangents
	for (u32 b=0; b < Buffers.size(); ++b)
	{
		if (Buffers[b])
		{

			Buffers[b]->MoveTo_Tangents();

			s32 idxCnt = Buffers[b]->getIndexCount();

			u16* idx = Buffers[b]->getIndices();
			video::S3DVertexTangents* v =
				(video::S3DVertexTangents*)Buffers[b]->getVertices();

			for (s32 i=0; i<idxCnt; i+=3)
			{
				calculateTangents(
					v[idx[i+0]].Normal,
					v[idx[i+0]].Tangent,
					v[idx[i+0]].Binormal,
					v[idx[i+0]].Pos,
					v[idx[i+1]].Pos,
					v[idx[i+2]].Pos,
					v[idx[i+0]].TCoords,
					v[idx[i+1]].TCoords,
					v[idx[i+2]].TCoords);

				calculateTangents(
					v[idx[i+1]].Normal,
					v[idx[i+1]].Tangent,
					v[idx[i+1]].Binormal,
					v[idx[i+1]].Pos,
					v[idx[i+2]].Pos,
					v[idx[i+0]].Pos,
					v[idx[i+1]].TCoords,
					v[idx[i+2]].TCoords,
					v[idx[i+0]].TCoords);

				calculateTangents(
					v[idx[i+2]].Normal,
					v[idx[i+2]].Tangent,
					v[idx[i+2]].Binormal,
					v[idx[i+2]].Pos,
					v[idx[i+0]].Pos,
					v[idx[i+1]].Pos,
					v[idx[i+2]].TCoords,
					v[idx[i+0]].TCoords,
					v[idx[i+1]].TCoords);
			}
		}



	}


/*
	// For skinning
	for (s32 i=0; i < (s32)Nodes.size(); ++i)
	{
		SB3dNode *Node=Nodes[i];
		for (s32 j=0; j<(s32)Node->Bones.size(); ++j)
		{
			Node->Bones[j].pos = BaseVertices[Node->Bones[j].vertex_id]->Pos;
			Node->Bones[j].normal = BaseVertices[Node->Bones[j].vertex_id]->Normal;
			Node->Bones[j].vertex =
				AnimatedVertices_MeshBuffer[ Node->Bones[j].vertex_id ]->getVertex(
					AnimatedVertices_VertexID[ Node->Bones[j].vertex_id ] );
		}
	}
*/

}



void CSkinnedMesh::calculateTangents(
	core::vector3df& normal,
	core::vector3df& tangent,
	core::vector3df& binormal,
	core::vector3df& vt1, core::vector3df& vt2, core::vector3df& vt3, // vertices
	core::vector2df& tc1, core::vector2df& tc2, core::vector2df& tc3) // texture coords
{
	core::vector3df v1 = vt1 - vt2;
	core::vector3df v2 = vt3 - vt1;
	normal = v2.crossProduct(v1);
	normal.normalize();

	// binormal

	f32 deltaX1 = tc1.X - tc2.X;
	f32 deltaX2 = tc3.X - tc1.X;
	binormal = (v1 * deltaX2) - (v2 * deltaX1);
	binormal.normalize();

	// tangent

	f32 deltaY1 = tc1.Y - tc2.Y;
	f32 deltaY2 = tc3.Y - tc1.Y;
	tangent = (v1 * deltaY2) - (v2 * deltaY1);
	tangent.normalize();

	// adjust

	core::vector3df txb = tangent.crossProduct(binormal);
	if (txb.dotProduct(normal) < 0.0f)
	{
		tangent *= -1.0f;
		binormal *= -1.0f;
	}
}

/*

void CSkinnedMesh::createSkelton_Helper(ISceneManager* SceneManager, core::array<IBoneSceneNode*> &JointChildSceneNodes,
	IAnimatedMeshSceneNode* AnimatedMeshSceneNode, ISceneNode* ParentNode, SB3dNode *ParentB3dNode,SB3dNode *B3dNode)
{
	//Note: This function works because of the way the b3d fomat nests nodes, other mesh loaders may need a different function
	if (!ParentNode)
	{
		for (s32 i=0;i<(s32)RootNodes.size();++i)
		{
			B3dNode=RootNodes[i];
			ISceneNode* node = SceneManager->addEmptySceneNode(AnimatedMeshSceneNode);
			JointChildSceneNodes.push_back(node);
			for (s32 j=0;j<(s32)B3dNode->Nodes.size();++j)
				createSkelton_Helper(SceneManager,JointChildSceneNodes,AnimatedMeshSceneNode,node,B3dNode,B3dNode->Nodes[j]);
		}
	}
	else
	{
		ISceneNode* node = SceneManager->addEmptySceneNode(ParentNode);
		JointChildSceneNodes.push_back(node);
		for (s32 j=0;j<(s32)B3dNode->Nodes.size();++j)
			createSkelton_Helper(SceneManager,JointChildSceneNodes,AnimatedMeshSceneNode,node,B3dNode,B3dNode->Nodes[j]);
	}
}


*/


} // end namespace scene
} // end namespace irr



// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CMeshManipulator.h"
#include "SMesh.h"
#include "CMeshBuffer.h"
#include "SAnimatedMesh.h"
#include "os.h"
#include "irrMap.h"

namespace irr
{
namespace scene
{

void CMeshManipulator::flipSurfaces(scene::IMesh* mesh) const
{
	if (!mesh)
		return;

	const u32 bcount = mesh->getMeshBufferCount();
	for (u32 b=0; b<bcount; ++b)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(b);
		const u32 idxcnt = buffer->getIndexBuffer()->getIndexCount();
		if (buffer->getIndexBuffer()->getType() == video::EIT_16BIT)
		{
			u16* idx = reinterpret_cast<u16*>(buffer->getIndexBuffer()->getIndices());
			for (u32 i=0; i<idxcnt; i+=3)
			{
				const u16 tmp = idx[i+1];
				idx[i+1] = idx[i+2];
				idx[i+2] = tmp;
			}
		}
		else
		{
			u32* idx = reinterpret_cast<u32*>(buffer->getIndexBuffer()->getIndices());
			for (u32 i=0; i<idxcnt; i+=3)
			{
				const u32 tmp = idx[i+1];
				idx[i+1] = idx[i+2];
				idx[i+2] = tmp;
			}
		}
	}
}

void CMeshManipulator::setVertexColor(IMeshBuffer* meshBuffer, video::SColor color, bool onlyAlpha) const
{
	if(!meshBuffer)
		return;

	video::IVertexAttribute* attribute = meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeBySemantic(video::EVAS_COLOR);

	if(!attribute)
		return;

	u8* offset = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());
	offset += attribute->getOffset();

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexCount(); ++i)
	{
		video::SColor* vcolor = (video::SColor*)offset;

		if(onlyAlpha)
			(*vcolor).setAlpha(color.getAlpha());
		else
			*vcolor = color;

		offset += meshBuffer->getVertexBuffer()->getVertexSize();
	}
}

void CMeshManipulator::setVertexColorAlpha(IMeshBuffer* meshBuffer, s32 alpha) const
{
	setVertexColor(meshBuffer, video::SColor(alpha, 255, 255, 255), true);
}

void CMeshManipulator::setVertexColors(IMeshBuffer* meshBuffer, video::SColor color) const
{
	setVertexColor(meshBuffer, color, false);
}

void CMeshManipulator::scale(IMeshBuffer* meshBuffer, const core::vector3df& factor) const
{
	if(!meshBuffer)
		return;

	video::IVertexAttribute* attribute = meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeBySemantic(video::EVAS_POSITION);

	if(!attribute)
		return;

	u8* offset = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());
	offset += attribute->getOffset();

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexCount(); ++i)
	{
		core::vector3df* position = (core::vector3df*)offset;

		*position *= factor;

		offset += meshBuffer->getVertexBuffer()->getVertexSize();
	}
}

void CMeshManipulator::scaleTCoords(IMeshBuffer* meshBuffer, const core::vector2df& factor, u32 level) const
{
	if(!meshBuffer)
		return;

	if(level > 7)
		level = 7;

	for(u32 j = 0; j < level; ++j)
	{
		video::E_VERTEX_ATTRIBUTE_SEMANTIC semantic = video::EVAS_TEXCOORD0;
		s32 value = (s32)semantic;
		value += j;
		semantic = (video::E_VERTEX_ATTRIBUTE_SEMANTIC)value;

		video::IVertexAttribute* attribute = meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeBySemantic(semantic);

		if(!attribute)
			continue;

		u8* offset = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());
		offset += attribute->getOffset();

		for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexCount(); ++i)
		{
			core::vector2df* texCoord = (core::vector2df*)offset;

			*texCoord *= factor;

			offset += meshBuffer->getVertexBuffer()->getVertexSize();
		}
	}
}

void CMeshManipulator::transform(IMeshBuffer* meshBuffer, const core::matrix4& matrix) const
{
	if(!meshBuffer)
		return;

	video::IVertexAttribute* attribute = meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeBySemantic(video::EVAS_POSITION);

	if(!attribute)
		return;

	u8* offset = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());
	offset += attribute->getOffset();

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexCount(); ++i)
	{
		core::vector3df* position = (core::vector3df*)offset;

		matrix.transformVect(*position);

		offset += meshBuffer->getVertexBuffer()->getVertexSize();
	}
}

static inline core::vector3df getAngleWeight(const core::vector3df& v1,
		const core::vector3df& v2,
		const core::vector3df& v3)
{
	// Calculate this triangle's weight for each of its three vertices
	// start by calculating the lengths of its sides
	const f32 a = v2.getDistanceFromSQ(v3);
	const f32 asqrt = sqrtf(a);
	const f32 b = v1.getDistanceFromSQ(v3);
	const f32 bsqrt = sqrtf(b);
	const f32 c = v1.getDistanceFromSQ(v2);
	const f32 csqrt = sqrtf(c);

	// use them to find the angle at each vertex
	return core::vector3df(
		acosf((b + c - a) / (2.f * bsqrt * csqrt)),
		acosf((-b + c + a) / (2.f * asqrt * csqrt)),
		acosf((b - c + a) / (2.f * bsqrt * asqrt)));
}

void CMeshManipulator::recalculateNormals(IMeshBuffer* meshBuffer, bool smooth, bool angleWeighted) const
{
	if(!meshBuffer)
		return;

	// Check Descriptor format for required 2 components.

	int Found = 0;

	u8* Vertices = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());

	u8* positionOffset = Vertices;
	u8* normalOffset = Vertices;

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeCount(); ++i)
	{
		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
		{
			positionOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_NORMAL)
		{
			normalOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}
	}

	if(Found != 2)
		return;

	// Recalculate normals.

	core::vector3df* position0 = 0;
	core::vector3df* position1 = 0;
	core::vector3df* position2 = 0;
	core::vector3df* normal0 = 0;
	core::vector3df* normal1 = 0;
	core::vector3df* normal2 = 0;

	for(u32 i = 0; i < meshBuffer->getIndexBuffer()->getIndexCount(); i+=3)
	{
		position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		position1 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		position2 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));

		normal0 = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		normal1 = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		normal2 = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));

		core::vector3df normal = core::plane3d<f32>(*position0, *position1, *position2).Normal;

		core::vector3df weight(1.0f, 1.0f, 1.0f);

		if(angleWeighted)
			weight = getAngleWeight(*position0, *position1, *position2);

		*normal0 = normal * weight.X;
		*normal1 = normal * weight.Y;
		*normal2 = normal * weight.Z;
	}

	if(smooth)
	{
		for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexCount(); ++i)
		{
			normal0 = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * i);
			(*normal0).normalize();
		}
	}
}

void calculateTangents(
	core::vector3df& normal,
	core::vector3df& tangent,
	core::vector3df& binormal,
	const core::vector3df& vt1, const core::vector3df& vt2, const core::vector3df& vt3,
	const core::vector2df& tc1, const core::vector2df& tc2, const core::vector2df& tc3)
{
	// choose one of them:
	//#define USE_NVIDIA_GLH_VERSION // use version used by nvidia in glh headers
	#define USE_IRR_VERSION

#ifdef USE_IRR_VERSION

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

#endif // USE_IRR_VERSION

#ifdef USE_NVIDIA_GLH_VERSION

	tangent.set(0,0,0);
	binormal.set(0,0,0);

	core::vector3df v1(vt2.X - vt1.X, tc2.X - tc1.X, tc2.Y - tc1.Y);
	core::vector3df v2(vt3.X - vt1.X, tc3.X - tc1.X, tc3.Y - tc1.Y);

	core::vector3df txb = v1.crossProduct(v2);
	if ( !core::iszero ( txb.X ) )
	{
		tangent.X  = -txb.Y / txb.X;
		binormal.X = -txb.Z / txb.X;
	}

	v1.X = vt2.Y - vt1.Y;
	v2.X = vt3.Y - vt1.Y;
	txb = v1.crossProduct(v2);

	if ( !core::iszero ( txb.X ) )
	{
		tangent.Y  = -txb.Y / txb.X;
		binormal.Y = -txb.Z / txb.X;
	}

	v1.X = vt2.Z - vt1.Z;
	v2.X = vt3.Z - vt1.Z;
	txb = v1.crossProduct(v2);

	if ( !core::iszero ( txb.X ) )
	{
		tangent.Z  = -txb.Y / txb.X;
		binormal.Z = -txb.Z / txb.X;
	}

	tangent.normalize();
	binormal.normalize();

	normal = tangent.crossProduct(binormal);
	normal.normalize();

	binormal = tangent.crossProduct(normal);
	binormal.normalize();

	core::plane3d<f32> pl(vt1, vt2, vt3);

	if(normal.dotProduct(pl.Normal) < 0.0f )
		normal *= -1.0f;

#endif // USE_NVIDIA_GLH_VERSION
}

void CMeshManipulator::recalculateTangents(IMeshBuffer* meshBuffer, bool recalculateNormals, bool smooth, bool angleWeighted) const
{
	// TODO : Parameters support.

	if(!meshBuffer)
		return;

	// Check Descriptor format for required 5 components.

	int Found = 0;

	u8* Vertices = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());

	u8* positionOffset = Vertices;
	u8* normalOffset = Vertices;
	u8* texCoordOffset = Vertices;
	u8* tangentOffset = Vertices;
	u8* binormalOffset = Vertices;

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeCount(); ++i)
	{
		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
		{
			positionOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_NORMAL)
		{
			normalOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_TEXCOORD0)
		{
			texCoordOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_TANGENT)
		{
			tangentOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_BINORMAL)
		{
			binormalOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}
	}

	if(Found != 5)
		return;

	// Recalculate tangents.

	core::vector3df* normal = 0;
	core::vector3df* tangent = 0;
	core::vector3df* binormal = 0;
	core::vector3df* position0 = 0;
	core::vector3df* position1 = 0;
	core::vector3df* position2 = 0;
	core::vector2df* texCoord0 = 0;
	core::vector2df* texCoord1 = 0;
	core::vector2df* texCoord2 = 0;

	for(u32 i = 0; i < meshBuffer->getIndexBuffer()->getIndexCount(); i+=3)
	{
		// Grab pointers to first vertex.

		normal = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		tangent = (core::vector3df*)(tangentOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		binormal = (core::vector3df*)(binormalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		position1 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		position2 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		texCoord0 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		texCoord1 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		texCoord2 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));

		calculateTangents(
			*normal,
			*tangent,
			*binormal,
			*position0,
			*position1,
			*position2,
			*texCoord0,
			*texCoord1,
			*texCoord2);

		// Grab pointers to second vertex.

		normal = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		tangent = (core::vector3df*)(tangentOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		binormal = (core::vector3df*)(binormalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		position1 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		position2 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		texCoord0 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		texCoord1 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		texCoord2 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));

		calculateTangents(
			*normal,
			*tangent,
			*binormal,
			*position0,
			*position1,
			*position2,
			*texCoord0,
			*texCoord1,
			*texCoord2);

		// Grab pointers to third vertex.

		normal = (core::vector3df*)(normalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		tangent = (core::vector3df*)(tangentOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		binormal = (core::vector3df*)(binormalOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		position1 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		position2 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		texCoord0 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));
		texCoord1 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		texCoord2 = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));

		calculateTangents(
			*normal,
			*tangent,
			*binormal,
			*position0,
			*position1,
			*position2,
			*texCoord0,
			*texCoord1,
			*texCoord2);
	}
}

void CMeshManipulator::makePlanarTextureMapping(IMeshBuffer* meshBuffer, f32 resolution) const
{
	if(!meshBuffer)
		return;

	// Check Descriptor format for required 2 components.

	int Found = 0;

	u8* Vertices = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());

	u8* positionOffset = Vertices;
	u8* texCoordOffset = Vertices;

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeCount(); ++i)
	{
		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
		{
			positionOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_TEXCOORD0)
		{
			texCoordOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}
	}

	if(Found != 2)
		return;

	// Make mapping.

	core::vector3df* position0 = 0;
	core::vector3df* position1 = 0;
	core::vector3df* position2 = 0;
	core::vector2df* texCoord = 0;

	for(u32 i = 0; i < meshBuffer->getIndexBuffer()->getIndexCount(); i+=3)
	{
		position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+0));
		position1 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+1));
		position2 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+2));

		core::plane3df plane(*position0, *position1, *position2);
		plane.Normal.X = fabsf(plane.Normal.X);
		plane.Normal.Y = fabsf(plane.Normal.Y);
		plane.Normal.Z = fabsf(plane.Normal.Z);

		// Calculate planar mapping worldspace coordinates.

		if(plane.Normal.X > plane.Normal.Y && plane.Normal.X > plane.Normal.Z)
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = (*position0).Y * resolution;
				(*texCoord).Y = (*position0).Z * resolution;
			}
		}
		else if(plane.Normal.Y > plane.Normal.X && plane.Normal.Y > plane.Normal.Z)
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = (*position0).X * resolution;
				(*texCoord).Y = (*position0).Z * resolution;
			}
		}
		else
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position0 = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = (*position0).X * resolution;
				(*texCoord).Y = (*position0).Y * resolution;
			}
		}
	}
}

void CMeshManipulator::makePlanarTextureMapping(IMeshBuffer* meshBuffer, f32 resolutionS, f32 resolutionT, u8 axis, const core::vector3df& offset) const
{
	if(!meshBuffer)
		return;

	// Check Descriptor format for required 2 components.

	int Found = 0;

	u8* Vertices = static_cast<u8*>(meshBuffer->getVertexBuffer()->getVertices());

	u8* positionOffset = Vertices;
	u8* texCoordOffset = Vertices;

	for(u32 i = 0; i < meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttributeCount(); ++i)
	{
		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_POSITION)
		{
			positionOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}

		if(meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getSemantic() == video::EVAS_TEXCOORD0)
		{
			texCoordOffset += meshBuffer->getVertexBuffer()->getVertexDescriptor()->getAttribute(i)->getOffset();
			Found++;
		}
	}

	if(Found != 2)
		return;

	// Make mapping.

	core::vector3df* position = 0;
	core::vector2df* texCoord = 0;

	for(u32 i = 0; i < meshBuffer->getIndexBuffer()->getIndexCount(); i+=3)
	{
		// Calculate planar mapping worldspace coordinates.

		if(axis == 0)
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = 0.5f + ((*position).Z + offset.Z) * resolutionS;
				(*texCoord).Y = 0.5f - ((*position).Y + offset.Y) * resolutionT;
			}
		}
		else if(axis == 1)
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = 0.5f + ((*position).X + offset.X) * resolutionS;
				(*texCoord).Y = 1.0f - ((*position).Z + offset.Z) * resolutionT;
			}
		}
		else if(axis == 2)
		{
			for(u32 j = 0; j < 3; ++j)
			{
				position = (core::vector3df*)(positionOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));
				texCoord = (core::vector2df*)(texCoordOffset + meshBuffer->getVertexBuffer()->getVertexSize() * meshBuffer->getIndexBuffer()->getIndex(i+j));

				(*texCoord).X = 0.5f + ((*position).X + offset.X) * resolutionS;
				(*texCoord).Y = 0.5f - ((*position).Y + offset.Y) * resolutionT;
			}
		}
	}
}

bool CMeshManipulator::copyVertices(IVertexBuffer* srcBuffer, IVertexBuffer* dstBuffer, bool copyCustomAttribute) const
{
	if(!srcBuffer || !dstBuffer || !srcBuffer->getVertexDescriptor() || !dstBuffer->getVertexDescriptor() || srcBuffer->getVertexCount() == 0)
		return false;

	dstBuffer->set_used(srcBuffer->getVertexCount());

	for(u32 i = 0; i < dstBuffer->getVertexDescriptor()->getAttributeCount(); ++i)
	{
		video::E_VERTEX_ATTRIBUTE_SEMANTIC semantic = dstBuffer->getVertexDescriptor()->getAttribute(i)->getSemantic();

		video::IVertexAttribute* attribute = 0;

		if(semantic != video::EVAS_CUSTOM)
			attribute = srcBuffer->getVertexDescriptor()->getAttributeBySemantic(semantic);
		else if(copyCustomAttribute)
		{
			core::stringc name = dstBuffer->getVertexDescriptor()->getAttribute(i)->getName();

			for(u32 j = 0; j < srcBuffer->getVertexDescriptor()->getAttributeCount() && !attribute; ++j)
				if(name == srcBuffer->getVertexDescriptor()->getAttribute(j)->getName())
					attribute = srcBuffer->getVertexDescriptor()->getAttribute(j);
		}

		if(!attribute || dstBuffer->getVertexDescriptor()->getAttribute(i)->getElementCount() != attribute->getElementCount() ||
			dstBuffer->getVertexDescriptor()->getAttribute(i)->getType() != attribute->getType())
		{
			attribute = 0;
		}

		if(attribute)
		{
			u8* VerticesSource = static_cast<u8*>(srcBuffer->getVertices());
			u8* VerticesDestination = static_cast<u8*>(dstBuffer->getVertices());

			VerticesSource += attribute->getOffset();
			VerticesDestination += dstBuffer->getVertexDescriptor()->getAttribute(i)->getOffset();

			for(u32 j = 0; j < srcBuffer->getVertexCount(); ++j)
			{
				memcpy(VerticesDestination, VerticesSource, attribute->getTypeSize() * attribute->getElementCount());

				VerticesSource += srcBuffer->getVertexSize();
				VerticesDestination += dstBuffer->getVertexSize();
			}
		}
	}

	return true;
}

s32 CMeshManipulator::getPolyCount(IMesh* mesh) const
{
	if (!mesh)
		return 0;

	s32 trianglecount = 0;

	for (u32 g=0; g<mesh->getMeshBufferCount(); ++g)
		trianglecount += mesh->getMeshBuffer(g)->getIndexBuffer()->getIndexCount() / 3;

	return trianglecount;
}

s32 CMeshManipulator::getPolyCount(IAnimatedMesh* mesh) const
{
	if (mesh && mesh->getFrameCount() != 0)
		return getPolyCount(mesh->getMesh(0));

	return 0;
}

IAnimatedMesh* CMeshManipulator::createAnimatedMesh(IMesh* mesh, E_ANIMATED_MESH_TYPE type) const
{
	return new SAnimatedMesh(mesh, type);
}

namespace
{

struct vcache
{
	core::array<u32> tris;
	float score;
	s16 cachepos;
	u16 NumActiveTris;
};

struct tcache
{
	u16 ind[3];
	float score;
	bool drawn;
};

const u16 cachesize = 32;

float FindVertexScore(vcache *v)
{
	const float CacheDecayPower = 1.5f;
	const float LastTriScore = 0.75f;
	const float ValenceBoostScale = 2.0f;
	const float ValenceBoostPower = 0.5f;
	const float MaxSizeVertexCache = 32.0f;

	if (v->NumActiveTris == 0)
	{
		// No tri needs this vertex!
		return -1.0f;
	}

	float Score = 0.0f;
	int CachePosition = v->cachepos;
	if (CachePosition < 0)
	{
		// Vertex is not in FIFO cache - no score.
	}
	else
	{
		if (CachePosition < 3)
		{
			// This vertex was used in the last triangle,
			// so it has a fixed score.
			Score = LastTriScore;
		}
		else
		{
			// Points for being high in the cache.
			const float Scaler = 1.0f / (MaxSizeVertexCache - 3);
			Score = 1.0f - (CachePosition - 3) * Scaler;
			Score = powf(Score, CacheDecayPower);
		}
	}

	// Bonus points for having a low number of tris still to
	// use the vert, so we get rid of lone verts quickly.
	float ValenceBoost = powf(v->NumActiveTris,
				-ValenceBoostPower);
	Score += ValenceBoostScale * ValenceBoost;

	return Score;
}

/*
	A specialized LRU cache for the Forsyth algorithm.
*/

class f_lru
{

public:
	f_lru(vcache *v, tcache *t): vc(v), tc(t)
	{
		for (u16 i = 0; i < cachesize; i++)
		{
			cache[i] = -1;
		}
	}

	// Adds this vertex index and returns the highest-scoring triangle index
	u32 add(u16 vert, bool updatetris = false)
	{
		bool found = false;

		// Mark existing pos as empty
		for (u16 i = 0; i < cachesize; i++)
		{
			if (cache[i] == vert)
			{
				// Move everything down
				for (u16 j = i; j; j--)
				{
					cache[j] = cache[j - 1];
				}

				found = true;
				break;
			}
		}

		if (!found)
		{
			if (cache[cachesize-1] != -1)
				vc[cache[cachesize-1]].cachepos = -1;

			// Move everything down
			for (u16 i = cachesize - 1; i; i--)
			{
				cache[i] = cache[i - 1];
			}
		}

		cache[0] = vert;

		u32 highest = 0;
		float hiscore = 0;

		if (updatetris)
		{
			// Update cache positions
			for (u16 i = 0; i < cachesize; i++)
			{
				if (cache[i] == -1)
					break;

				vc[cache[i]].cachepos = i;
				vc[cache[i]].score = FindVertexScore(&vc[cache[i]]);
			}

			// Update triangle scores
			for (u16 i = 0; i < cachesize; i++)
			{
				if (cache[i] == -1)
					break;

				const u16 trisize = vc[cache[i]].tris.size();
				for (u16 t = 0; t < trisize; t++)
				{
					tcache *tri = &tc[vc[cache[i]].tris[t]];

					tri->score =
						vc[tri->ind[0]].score +
						vc[tri->ind[1]].score +
						vc[tri->ind[2]].score;

					if (tri->score > hiscore)
					{
						hiscore = tri->score;
						highest = vc[cache[i]].tris[t];
					}
				}
			}
		}

		return highest;
	}

private:
	s32 cache[cachesize];
	vcache *vc;
	tcache *tc;
};

} // end anonymous namespace

/**
Vertex cache optimization according to the Forsyth paper:
http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html

The function is thread-safe (read: you can optimize several meshes in different threads)

\param mesh Source mesh for the operation.  */
IMesh* CMeshManipulator::createForsythOptimizedMesh(const IMesh* mesh) const
{
	// TODO : re-implementation of this method.

	return 0;
	/*if (!mesh)
		return 0;

	SMesh *newmesh = new SMesh();
	newmesh->BoundingBox = mesh->getBoundingBox();

	const u32 mbcount = mesh->getMeshBufferCount();

	for (u32 b = 0; b < mbcount; ++b)
	{
		const IMeshBuffer *mb = mesh->getMeshBuffer(b);

		if (mb->getIndexBuffer()->getType() != video::EIT_16BIT)
		{
			os::Printer::log("Cannot optimize a mesh with 32bit indices", ELL_ERROR);
			newmesh->drop();
			return 0;
		}

		const u32 icount = mb->getIndexCount();
		const u32 tcount = icount / 3;
		const u32 vcount = mb->getVertexCount();
		const u16 *ind = mb->getIndices();

		vcache *vc = new vcache[vcount];
		tcache *tc = new tcache[tcount];

		f_lru lru(vc, tc);

		// init
		for (u16 i = 0; i < vcount; i++)
		{
			vc[i].score = 0;
			vc[i].cachepos = -1;
			vc[i].NumActiveTris = 0;
		}

		// First pass: count how many times a vert is used
		for (u32 i = 0; i < icount; i += 3)
		{
			vc[ind[i]].NumActiveTris++;
			vc[ind[i + 1]].NumActiveTris++;
			vc[ind[i + 2]].NumActiveTris++;

			const u32 tri_ind = i/3;
			tc[tri_ind].ind[0] = ind[i];
			tc[tri_ind].ind[1] = ind[i + 1];
			tc[tri_ind].ind[2] = ind[i + 2];
		}

		// Second pass: list of each triangle
		for (u32 i = 0; i < tcount; i++)
		{
			vc[tc[i].ind[0]].tris.push_back(i);
			vc[tc[i].ind[1]].tris.push_back(i);
			vc[tc[i].ind[2]].tris.push_back(i);

			tc[i].drawn = false;
		}

		// Give initial scores
		for (u16 i = 0; i < vcount; i++)
		{
			vc[i].score = FindVertexScore(&vc[i]);
		}
		for (u32 i = 0; i < tcount; i++)
		{
			tc[i].score =
					vc[tc[i].ind[0]].score +
					vc[tc[i].ind[1]].score +
					vc[tc[i].ind[2]].score;
		}

		switch(mb->getVertexType())
		{
			case video::EVT_STANDARD:
			{
				video::S3DVertex *v = (video::S3DVertex *) mb->getVertices();

				SMeshBuffer *buf = new SMeshBuffer();
				buf->Material = mb->getMaterial();

				buf->Vertices.reallocate(vcount);
				buf->Indices.reallocate(icount);

				core::map<const video::S3DVertex, const u16> sind; // search index for fast operation
				typedef core::map<const video::S3DVertex, const u16>::Node snode;

				// Main algorithm
				u32 highest = 0;
				u32 drawcalls = 0;
				for (;;)
				{
					if (tc[highest].drawn)
					{
						bool found = false;
						float hiscore = 0;
						for (u32 t = 0; t < tcount; t++)
						{
							if (!tc[t].drawn)
							{
								if (tc[t].score > hiscore)
								{
									highest = t;
									hiscore = tc[t].score;
									found = true;
								}
							}
						}
						if (!found)
							break;
					}

					// Output the best triangle
					u16 newind = buf->Vertices.size();

					snode *s = sind.find(v[tc[highest].ind[0]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[0]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[0]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[1]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[1]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[1]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[2]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[2]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[2]], newind);
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					vc[tc[highest].ind[0]].NumActiveTris--;
					vc[tc[highest].ind[1]].NumActiveTris--;
					vc[tc[highest].ind[2]].NumActiveTris--;

					tc[highest].drawn = true;

					for (u16 j = 0; j < 3; j++)
					{
						vcache *vert = &vc[tc[highest].ind[j]];
						for (u16 t = 0; t < vert->tris.size(); t++)
						{
							if (highest == vert->tris[t])
							{
								vert->tris.erase(t);
								break;
							}
						}
					}

					lru.add(tc[highest].ind[0]);
					lru.add(tc[highest].ind[1]);
					highest = lru.add(tc[highest].ind[2], true);
					drawcalls++;
				}

				buf->setBoundingBox(mb->getBoundingBox());
				newmesh->addMeshBuffer(buf);
				buf->drop();
			}
			break;
			case video::EVT_2TCOORDS:
			{
				video::S3DVertex2TCoords *v = (video::S3DVertex2TCoords *) mb->getVertices();

				SMeshBufferLightMap *buf = new SMeshBufferLightMap();
				buf->Material = mb->getMaterial();

				buf->Vertices.reallocate(vcount);
				buf->Indices.reallocate(icount);

				core::map<const video::S3DVertex2TCoords, const u16> sind; // search index for fast operation
				typedef core::map<const video::S3DVertex2TCoords, const u16>::Node snode;

				// Main algorithm
				u32 highest = 0;
				u32 drawcalls = 0;
				for (;;)
				{
					if (tc[highest].drawn)
					{
						bool found = false;
						float hiscore = 0;
						for (u32 t = 0; t < tcount; t++)
						{
							if (!tc[t].drawn)
							{
								if (tc[t].score > hiscore)
								{
									highest = t;
									hiscore = tc[t].score;
									found = true;
								}
							}
						}
						if (!found)
							break;
					}

					// Output the best triangle
					u16 newind = buf->Vertices.size();

					snode *s = sind.find(v[tc[highest].ind[0]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[0]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[0]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[1]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[1]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[1]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[2]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[2]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[2]], newind);
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					vc[tc[highest].ind[0]].NumActiveTris--;
					vc[tc[highest].ind[1]].NumActiveTris--;
					vc[tc[highest].ind[2]].NumActiveTris--;

					tc[highest].drawn = true;

					for (u16 j = 0; j < 3; j++)
					{
						vcache *vert = &vc[tc[highest].ind[j]];
						for (u16 t = 0; t < vert->tris.size(); t++)
						{
							if (highest == vert->tris[t])
							{
								vert->tris.erase(t);
								break;
							}
						}
					}

					lru.add(tc[highest].ind[0]);
					lru.add(tc[highest].ind[1]);
					highest = lru.add(tc[highest].ind[2]);
					drawcalls++;
				}

				buf->setBoundingBox(mb->getBoundingBox());
				newmesh->addMeshBuffer(buf);
				buf->drop();

			}
			break;
			case video::EVT_TANGENTS:
			{
				video::S3DVertexTangents *v = (video::S3DVertexTangents *) mb->getVertices();

				SMeshBufferTangents *buf = new SMeshBufferTangents();
				buf->Material = mb->getMaterial();

				buf->Vertices.reallocate(vcount);
				buf->Indices.reallocate(icount);

				core::map<const video::S3DVertexTangents, const u16> sind; // search index for fast operation
				typedef core::map<const video::S3DVertexTangents, const u16>::Node snode;

				// Main algorithm
				u32 highest = 0;
				u32 drawcalls = 0;
				for (;;)
				{
					if (tc[highest].drawn)
					{
						bool found = false;
						float hiscore = 0;
						for (u32 t = 0; t < tcount; t++)
						{
							if (!tc[t].drawn)
							{
								if (tc[t].score > hiscore)
								{
									highest = t;
									hiscore = tc[t].score;
									found = true;
								}
							}
						}
						if (!found)
							break;
					}

					// Output the best triangle
					u16 newind = buf->Vertices.size();

					snode *s = sind.find(v[tc[highest].ind[0]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[0]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[0]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[1]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[1]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[1]], newind);
						newind++;
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					s = sind.find(v[tc[highest].ind[2]]);

					if (!s)
					{
						buf->Vertices.push_back(v[tc[highest].ind[2]]);
						buf->Indices.push_back(newind);
						sind.insert(v[tc[highest].ind[2]], newind);
					}
					else
					{
						buf->Indices.push_back(s->getValue());
					}

					vc[tc[highest].ind[0]].NumActiveTris--;
					vc[tc[highest].ind[1]].NumActiveTris--;
					vc[tc[highest].ind[2]].NumActiveTris--;

					tc[highest].drawn = true;

					for (u16 j = 0; j < 3; j++)
					{
						vcache *vert = &vc[tc[highest].ind[j]];
						for (u16 t = 0; t < vert->tris.size(); t++)
						{
							if (highest == vert->tris[t])
							{
								vert->tris.erase(t);
								break;
							}
						}
					}

					lru.add(tc[highest].ind[0]);
					lru.add(tc[highest].ind[1]);
					highest = lru.add(tc[highest].ind[2]);
					drawcalls++;
				}

				buf->setBoundingBox(mb->getBoundingBox());
				newmesh->addMeshBuffer(buf);
				buf->drop();
			}
			break;
		}

		delete [] vc;
		delete [] tc;

	} // for each meshbuffer

	return newmesh;*/
}

} // end namespace scene
} // end namespace irr


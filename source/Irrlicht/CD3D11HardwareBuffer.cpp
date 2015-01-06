// Copyright (C) 2002-2009 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#include "CD3D11HardwareBuffer.h"
#include "CD3D11Driver.h"
#include "os.h"

namespace irr
{
namespace video
{

CD3D11HardwareBuffer::CD3D11HardwareBuffer(scene::IIndexBuffer* indexBuffer, CD3D11Driver* driver) :
	IHardwareBuffer(scene::EHM_NEVER, 0, 0, EHBT_NONE, EDT_OPENGL), Driver(driver), Context(0), Buffer(0),
	RemoveFromArray(true), LinkedBuffer(0)
{
#ifdef _DEBUG
	setDebugName("CD3D11HardwareBuffer");
#endif

	Device = Driver->getExposedVideoData().D3D11.D3DDev11;

	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext(&Context);
	}

	Type = EHBT_INDEX;

	if (indexBuffer)
	{
		if (update(indexBuffer->getHardwareMappingHint(), indexBuffer->getIndexSize()*indexBuffer->getIndexCount(), indexBuffer->getIndices()))
		{
			indexBuffer->setHardwareBuffer(this);
			LinkedBuffer = indexBuffer;
		}
	}
}

CD3D11HardwareBuffer::CD3D11HardwareBuffer(scene::IVertexBuffer* vertexBuffer, CD3D11Driver* driver) :
	IHardwareBuffer(scene::EHM_NEVER, 0, 0, EHBT_NONE, EDT_OPENGL), Driver(driver), Context(0), Buffer(0),
	RemoveFromArray(true), LinkedBuffer(0)
{
#ifdef _DEBUG
	setDebugName("CD3D11HardwareBuffer");
#endif

	Device = Driver->getExposedVideoData().D3D11.D3DDev11;

	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext(&Context);
	}

	Type = EHBT_VERTEX;

	if (vertexBuffer)
	{
		if (update(vertexBuffer->getHardwareMappingHint(), vertexBuffer->getVertexSize()*vertexBuffer->getVertexCount(), vertexBuffer->getVertices()))
		{
			vertexBuffer->setHardwareBuffer(this);
			LinkedBuffer = vertexBuffer;
		}
	}
}

CD3D11HardwareBuffer::~CD3D11HardwareBuffer()
{
	if (RemoveFromArray)
	{
		for (u32 i = 0; i < Driver->HardwareBuffer.size(); ++i)
		{
			if (Driver->HardwareBuffer[i] == this)
			{
				Driver->HardwareBuffer[i] = 0;
				break;
			}
		}
	}

	if (LinkedBuffer)
	{
		switch (Type)
		{
		case EHBT_INDEX:
			((scene::IIndexBuffer*)LinkedBuffer)->setHardwareBuffer(0, true);
			break;
		case EHBT_VERTEX:
			((scene::IVertexBuffer*)LinkedBuffer)->setHardwareBuffer(0, true);
			break;
		default:
			break;
		}
	}

	if (Buffer)
		Buffer->Release();

	if (Context)
		Context->Release();

	if (Device)
		Device->Release();
}

bool CD3D11HardwareBuffer::update(const scene::E_HARDWARE_MAPPING mapping, const u32 size, const void* data)
{
	u32 oldSize = Size;

	Mapping = mapping;
	Size = size;

	if (Size == 0 || !data || !Driver)
		return false;

	UINT bindFlags = 0;

	switch (Type)
	{
	case EHBT_INDEX:
		bindFlags = D3D11_BIND_INDEX_BUFFER;
		break;
	case EHBT_VERTEX:
		bindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
		break;
	default:
		return false;
	}

	if (Buffer && oldSize < Size)
	{
		Buffer->Release();
		Buffer = 0;
	}

	bool status = true;

	if (!Buffer)
	{
		D3D11_BUFFER_DESC bufferDescription;
		bufferDescription.Usage = D3D11_USAGE_DEFAULT;
		bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDescription.ByteWidth = Size;
		bufferDescription.MiscFlags = 0;
		bufferDescription.CPUAccessFlags = 0;
		bufferDescription.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = data;

		Device->CreateBuffer(&bufferDescription, &subresourceData, &Buffer);

		if (!Buffer)
			status = false;
	}
	else
	{
		D3D11_BOX box;
		box.left = 0;
		box.top = 0;
		box.front = 0;
		box.right = Size;
		box.bottom = 1;
		box.back = 1;
		Context->UpdateSubresource(Buffer, 0, &box, data, 0, 0);
	}

	return status;
}

ID3D11Buffer* CD3D11HardwareBuffer::getBuffer() const
{
	return Buffer;
}

void CD3D11HardwareBuffer::removeFromArray(bool status)
{
	RemoveFromArray = status;
}

}
}

#endif

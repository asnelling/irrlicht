
#ifndef __C_DIRECTX11_HARDWARE_BUFFER_H_INCLUDED__
#define __C_DIRECTX11_HARDWARE_BUFFER_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "IHardwareBuffer.h"
#include "IIndexBuffer.h"
#include "IVertexBuffer.h"

#include <d3d11.h>

namespace irr
{
namespace video
{

class CD3D11Driver;

class CD3D11HardwareBuffer : public IHardwareBuffer
{
public:
	CD3D11HardwareBuffer(scene::IIndexBuffer* indexBuffer, CD3D11Driver* driver);
	CD3D11HardwareBuffer(scene::IVertexBuffer* vertexBuffer, CD3D11Driver* driver);
	~CD3D11HardwareBuffer();

	bool update(const scene::E_HARDWARE_MAPPING mapping, const u32 size, const void* data) _IRR_OVERRIDE_;

	ID3D11Buffer* getBuffer() const;
	void removeFromArray(bool status);

private:
	CD3D11Driver* Driver;
	ID3D11DeviceContext* Context;
	ID3D11Device* Device;

	ID3D11Buffer* Buffer;
	bool RemoveFromArray;

	void* LinkedBuffer;
};

}
}

#endif
#endif
#endif
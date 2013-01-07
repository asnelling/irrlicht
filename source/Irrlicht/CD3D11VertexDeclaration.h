
#ifndef __C_DIRECTX11_VERTEX_DECLARATION_H_INCLUDED__
#define __C_DIRECTX11_VERTEX_DECLARATION_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#ifdef _IRR_WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <D3D11.h>
#endif

#include "IReferenceCounted.h"

#include "irrArray.h"
#include "irrMap.h"
#include "S3DVertex.h"
#include "SVertexElement.h"

namespace irr
{
namespace video
{

class IMaterialRenderer;
class CD3D11Driver;

class CD3D11VertexDeclaration : public virtual IReferenceCounted
{
public:

	CD3D11VertexDeclaration(CD3D11Driver* driver, core::array<SVertexElement>& elements, E_VERTEX_TYPE type);
	~CD3D11VertexDeclaration();

	//! Get declaration structure 
	//D3D11_INPUT_ELEMENT_DESC* getInputAssemblyDeclaration() const;
	//D3D11_SO_DECLARATION_ENTRY* getStreamOutputDeclaration() const;
	const core::array<D3D11_INPUT_ELEMENT_DESC>& getInputAssemblyDeclaration() const;
	const core::array<D3D11_SO_DECLARATION_ENTRY>& getStreamOutputDeclaration() const;
	
	//! Get size of declaration
	u32 size() const { return Size; }

	//! 
	E_VERTEX_TYPE getType() const { return VertexType; }

	//! Get input layout
	ID3D11InputLayout* getInputLayout(IMaterialRenderer* renderer);

	//! Get vertex pitch
	u32 getVertexPitch() const;

private:
	core::array<D3D11_INPUT_ELEMENT_DESC> IAElements;
	mutable core::array<D3D11_SO_DECLARATION_ENTRY> SOElements;

	u32 Size;
	u32 VertexPitch;
	ID3D11Device* Device;
	ID3D11DeviceContext* ImmediateContext;
	
	typedef core::map<u32, ID3D11InputLayout*> LayoutMap;
	typedef core::map<u32, ID3D11InputLayout*>::Iterator LayoutIterator;
	typedef core::map<u32, ID3D11InputLayout*>::Node LayoutMapNode;
	LayoutMap layoutMap;
	
	CD3D11Driver* Driver;
	E_VERTEX_TYPE VertexType;

	//! Parse semantic
	LPCSTR getSemanticName(E_VERTEX_ELEMENT_SEMANTIC semantic) const;

	DXGI_FORMAT getFormat(E_VERTEX_ELEMENT_TYPE type) const;
};
}
}

#endif
#endif
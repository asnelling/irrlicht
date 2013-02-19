#include "CD3D11CallBridge.h"

#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#include "os.h"
#include "CD3D11VertexDeclaration.h"
#include "CD3D11Driver.h"
#include "CD3D11Texture.h"

#include <d3d11.h>

namespace irr
{
namespace video
{

CD3D11CallBridge::CD3D11CallBridge(ID3D11Device* device, CD3D11Driver* driver)
	: Context(NULL), Device(device), Driver(driver), InputLayout(NULL),
	VsShader(NULL), PsShader(NULL), GsShader(NULL), HsShader(NULL), DsShader(NULL), CsShader(NULL),
	Topology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED), VType(EVT_STANDARD), Renderer(NULL)
{
	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext( &Context );
	}

	ZeroMemory(CurrentTextures, sizeof(CurrentTextures[0]) * MATERIAL_MAX_TEXTURES);
	ZeroMemory(SamplerStates, sizeof(SamplerStates[0]) * MATERIAL_MAX_TEXTURES);
}

CD3D11CallBridge::~CD3D11CallBridge()
{
	// release blend states
	core::map<SD3D11_BLEND_DESC, ID3D11BlendState*>::Iterator bldIt = BlendMap.getIterator();
	while (!bldIt.atEnd())
	{
		if (bldIt->getValue()) 
			bldIt->getValue()->Release();
		bldIt++;
	}
	BlendMap.clear();

	// release rasterizer states
	core::map<SD3D11_RASTERIZER_DESC, ID3D11RasterizerState*>::Iterator rasIt = RasterizerMap.getIterator();
	while (!rasIt.atEnd())
	{
		if (rasIt->getValue()) 
			rasIt->getValue()->Release();
		rasIt++;
	}
	RasterizerMap.clear();

	// release depth stencil states
	core::map<SD3D11_DEPTH_STENCIL_DESC, ID3D11DepthStencilState*>::Iterator dsIt = DepthStencilMap.getIterator();
	while (!dsIt.atEnd())
	{
		if (dsIt->getValue()) 
			dsIt->getValue()->Release();
		dsIt++;
	}
	DepthStencilMap.clear();

	// release sampler states
	core::map<SD3D11_SAMPLER_DESC, ID3D11SamplerState*>::Iterator samIt = SamplerMap.getIterator();
	while (!samIt.atEnd())
	{
		if (samIt->getValue()) 
			samIt->getValue()->Release();
		samIt++;
	}
	SamplerMap.clear();

	// clear vertex declarations
	core::map<E_VERTEX_TYPE, CD3D11VertexDeclaration*>::Iterator it = DeclarationMap.getIterator();
	while(!it.atEnd())
	{
		if(it->getValue())
			it->getValue()->drop();
		it++;
	}
	DeclarationMap.clear();

	if(Context)
		Context->Release();

	if(Device)
		Device->Release();
}

void CD3D11CallBridge::setVertexShader(SShader* shader)
{
	if(VsShader != shader)
	{
		VsShader = shader;

		Context->VSSetShader((ID3D11VertexShader*)VsShader->shader, NULL, 0);

		const u32 size = VsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->VSSetConstantBuffers(i, 1, &VsShader->buffers[i]->data);
	}		
}

void CD3D11CallBridge::setPixelShader(SShader* shader)
{
	if(PsShader != shader)
	{
		PsShader = shader;
	
		Context->PSSetShader((ID3D11PixelShader*)PsShader->shader, NULL, 0);

		const u32 size = PsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->PSSetConstantBuffers(i, 1, &PsShader->buffers[i]->data);
	}
}

void CD3D11CallBridge::setGeometryShader(SShader* shader)
{
	if(GsShader != shader)
	{
		GsShader = shader;

		Context->GSSetShader((ID3D11GeometryShader*)GsShader->shader, NULL, 0);

		const u32 size = GsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->GSSetConstantBuffers(i, 1, &GsShader->buffers[i]->data);
	}
}

void CD3D11CallBridge::setHullShader(SShader* shader)
{
	if(HsShader != shader)
	{
		HsShader = shader;

		Context->HSSetShader((ID3D11HullShader*)HsShader->shader, NULL, 0);

		const u32 size = HsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->HSSetConstantBuffers(i, 1, &HsShader->buffers[i]->data);
	}
}

void CD3D11CallBridge::setDomainShader(SShader* shader)
{
	if(DsShader != shader)
	{
		DsShader = shader;

		Context->DSSetShader((ID3D11DomainShader*)DsShader->shader, NULL, 0);

		const u32 size = DsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->DSSetConstantBuffers(i, 1, &DsShader->buffers[i]->data);
	}
}

void CD3D11CallBridge::setComputeShader(SShader* shader)
{
	if(CsShader != shader)
	{
		CsShader = shader;

		Context->CSSetShader((ID3D11ComputeShader*)CsShader->shader, NULL, 0);

		const u32 size = CsShader->buffers.size();
		for(u32 i = 0; i < size; ++i)
			Context->CSSetConstantBuffers(i, 1, &CsShader->buffers[i]->data);
	}
}

void CD3D11CallBridge::setDepthStencilState(const SD3D11_DEPTH_STENCIL_DESC& depthStencilDesc)
{
	if(DepthStencilDesc != depthStencilDesc)
	{
		DepthStencilDesc = depthStencilDesc;

		ID3D11DepthStencilState* state = NULL;
		core::map<SD3D11_DEPTH_STENCIL_DESC, ID3D11DepthStencilState*>::Node* dsIt = DepthStencilMap.find(DepthStencilDesc);

		if (dsIt)
		{
			state = dsIt->getValue();
		}
		else	// if not found, create and insert into map
		{
			if (SUCCEEDED(Device->CreateDepthStencilState(&DepthStencilDesc, &state)))
			{
				DepthStencilMap.insert(DepthStencilDesc, state);
			}
			else
			{
				os::Printer::log("Failed to create depth stencil state", ELL_ERROR);
				return;
			}
		}

		Context->OMSetDepthStencilState(state, 1);
	}
}

void CD3D11CallBridge::setRasterizerState(const SD3D11_RASTERIZER_DESC& rasterizerDesc)
{
	if(RasterizerDesc != rasterizerDesc)
	{
		RasterizerDesc = rasterizerDesc;

		// Rasterizer state
		ID3D11RasterizerState* state = 0;
		core::map<SD3D11_RASTERIZER_DESC, ID3D11RasterizerState*>::Node* rasIt = RasterizerMap.find(RasterizerDesc);
		if (rasIt)
		{
			state = rasIt->getValue();
		}
		else	// if not found, create and insert into map
		{
			if (SUCCEEDED(Device->CreateRasterizerState(&RasterizerDesc, &state)))
			{
				RasterizerMap.insert(RasterizerDesc, state);
			}
			else
			{
				os::Printer::log("Failed to create rasterizer state", ELL_ERROR);
				return;
			}
		}

		Context->RSSetState(state);
	}
}

void CD3D11CallBridge::setBlendState(const SD3D11_BLEND_DESC& blendDesc)
{
	if(BlendDesc != blendDesc)
	{
		BlendDesc = blendDesc;

		ID3D11BlendState* state = 0;
		core::map<SD3D11_BLEND_DESC, ID3D11BlendState*>::Node* bldIt = BlendMap.find(BlendDesc);

		if (bldIt)
		{
			state = bldIt->getValue();
		}
		else	// if not found, create and insert into map
		{
			if (SUCCEEDED(Device->CreateBlendState(&BlendDesc, &state)))
			{
				BlendMap.insert( BlendDesc, state );
			}
			else
			{
				os::Printer::log("Failed to create blend state", ELL_ERROR);
				return;
			}
		}

		Context->OMSetBlendState(state, 0, 0xffffffff);
	}
}


void CD3D11CallBridge::setShaderResources(SD3D11_SAMPLER_DESC samplerDesc[], ITexture* currentTextures[])
{
	bool resetSamplers = false;
	bool resetViews = false;

	ID3D11ShaderResourceView* shaderViews[MATERIAL_MAX_TEXTURES] = { NULL };

	for(u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i)
	{
		if(SamplerDesc[i] != samplerDesc[i])
		{
			SamplerDesc[i] = samplerDesc[i];

			SamplerStates[i] = getSamplerState(i);
			resetSamplers = true;
		}

		if(CurrentTextures[i] != currentTextures[i])
		{
			CurrentTextures[i] = currentTextures[i];

			resetViews = true;
		}

		if(CurrentTextures[i] != NULL)
			shaderViews[i] = ((CD3D11Texture*)CurrentTextures[i])->getShaderResourceView();
	}

	if(resetViews)
		Context->PSSetShaderResources(0, MATERIAL_MAX_TEXTURES, shaderViews);

	if(resetSamplers)
		Context->PSSetSamplers(0, MATERIAL_MAX_TEXTURES, SamplerStates);
}

void CD3D11CallBridge::setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if(Topology != topology)
	{
		Topology = topology;

		Context->IASetPrimitiveTopology(Topology);
	}
}

void CD3D11CallBridge::setInputLayout(E_VERTEX_TYPE vType, IMaterialRenderer* renderer)
{
	if(VType != vType || Renderer != renderer)
	{
		VType = vType;
		Renderer = renderer;

		core::map<E_VERTEX_TYPE, CD3D11VertexDeclaration*>::Node* declNode = DeclarationMap.find(VType);
		if (declNode)
		{
			Context->IASetInputLayout(declNode->getValue()->getInputLayout(Renderer));
		}
	}
}

E_VERTEX_TYPE CD3D11CallBridge::registerVertexType(core::array<SVertexElement>& elements)
{
	CD3D11VertexDeclaration* decl = new CD3D11VertexDeclaration(Driver, elements, (E_VERTEX_TYPE)DeclarationMap.size());
	DeclarationMap.insert(decl->getType(), decl);

	return decl->getType();
}

core::map<E_VERTEX_TYPE, CD3D11VertexDeclaration*>::Node* CD3D11CallBridge::getDeclarationNode(E_VERTEX_TYPE vType)
{
	return DeclarationMap.find(vType);
}

ID3D11SamplerState* CD3D11CallBridge::getSamplerState(u32 idx)
{
	// Depth stencil state
	ID3D11SamplerState* state = 0;
	core::map<SD3D11_SAMPLER_DESC, ID3D11SamplerState*>::Node* samIt = SamplerMap.find( SamplerDesc[idx] );
	if (samIt)
	{
		state = samIt->getValue();
	}
	else	// if not found, create and insert into map
	{
		if (SUCCEEDED(Device->CreateSamplerState( &SamplerDesc[idx], &state )))
		{
			SamplerMap.insert( SamplerDesc[idx], state );
		}
		else
		{
			os::Printer::log("Failed to create sampler state", ELL_ERROR);
		}
	}

	return state;
}

}
}

#endif
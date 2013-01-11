#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_11_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE

#include "CD3D11MaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"
#include "IMaterialRendererServices.h"
#include "IVideoDriver.h"
#include "os.h"
#include "irrString.h"

namespace irr
{
namespace video
{

CD3D11ShaderMaterialRenderer::CD3D11ShaderMaterialRenderer( ID3D11Device* device, IVideoDriver* driver, IShaderConstantSetCallBack* callback, IMaterialRenderer* base, s32 userData)
	: Driver(driver), Device(device), Context(0), BaseMaterial(base), CallBack(callback)
{
#ifdef _DEBUG
	setDebugName("CD3D11ShaderMaterialRenderer");
#endif

	if (Device)
	{
		Device->AddRef();
		Device->GetImmediateContext( &Context );
	}	

	if (BaseMaterial)
		BaseMaterial->grab();

	if (CallBack)
		CallBack->grab();
}

CD3D11ShaderMaterialRenderer::~CD3D11ShaderMaterialRenderer()
{
	if (BaseMaterial)
		BaseMaterial->drop();
	BaseMaterial = NULL;

	if(Context)
		Context->Release();
	Context = NULL;
	
	if(Device)
		Device->Release();
	Device = NULL;

	if (CallBack)
		CallBack->drop();
	CallBack = NULL;
}

bool CD3D11ShaderMaterialRenderer::OnRender( IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype )
{
	if(BaseMaterial)
		BaseMaterial->OnRender(service, vtxtype);

	// call callback to set shader constants
	if (CallBack)
		CallBack->OnSetConstants(service, UserData);

	return true;
}

void CD3D11ShaderMaterialRenderer::OnSetMaterial( const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services )
{
	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (BaseMaterial)
			BaseMaterial->OnSetMaterial(material, material, resetAllRenderstates, services);
	}

	//let callback know used material
	if (CallBack)
		CallBack->OnSetMaterial(material);

	services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void CD3D11ShaderMaterialRenderer::OnUnsetMaterial()
{
	if (BaseMaterial)
		BaseMaterial->OnUnsetMaterial();
}

bool CD3D11ShaderMaterialRenderer::isTransparent() const
{
	return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

}
}

#endif
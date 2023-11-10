#include "Decal.h"
#include "InputLayout.h"
#include "Material.h"
#include "BindableManager.h"

Engine::Decal::Decal()
{
	SetRenderLayer(RENDER_LAYER::DECAL);

	FindAndAddBind<VertexShader>("DecalVS");
	FindAndAddBind<PixelShader>("DecalPS");
	FindAndAddBind<InputLayout>("Standard");

	std::shared_ptr<Material> pMaterial = StaticFindBindable<Material>("Material");

	AddChild(pMaterial->Clone());
}

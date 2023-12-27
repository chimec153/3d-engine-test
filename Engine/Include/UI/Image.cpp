#include "Image.h"
#include "../Bindable/Mesh.h"
#include "../Bindable/Topology.h"

namespace Engine
{
	Image::Image(const std::string& strTexture, const Vector2& vUVStart, const Vector2& vUVEnd) :
		UIControl()
	{
		SetBindableType(BINDABLE_TYPE::UI_IMAGE);
		SetRenderLayer(RENDER_LAYER::ALPHA);

		std::vector<VertexStandard> vecVertex(4);
		std::vector<unsigned int> vecIndex = { 0,1,2,2,1,3 };

		for (int i = 0; i < 2; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				vecVertex[i * 2 + j].pos.x = i;
				vecVertex[i * 2 + j].pos.y = 2 - j - 1;

				vecVertex[i * 2 + j].uv.x = i * (vUVEnd.x - vUVStart.x) + vUVStart.x;
				vecVertex[i * 2 + j].uv.y = j * (vUVEnd.y - vUVStart.y) + vUVStart.y;
			}
		}

		CreateBindable<Mesh>("mesh", vecVertex, vecIndex);
		FindAndAddBind<Topology>("TriangleList");
	}

}
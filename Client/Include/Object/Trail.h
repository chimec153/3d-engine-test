#pragma once
#include "Bindable\Drawable.h"

namespace Engine
{
    class Mesh;
}

namespace Client
{
    class Trail :
        public Engine::Drawable
    {
    public:
        Trail(int iCount);
        virtual ~Trail() = default;

    private:
        std::vector<Engine::VertexStandard> m_vecVertex;
        std::vector<unsigned int> m_vecIndex;
        std::shared_ptr<Engine::Mesh> m_pMesh;

    public:
        void SetAllPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom);
        void SetPosition(const Engine::Vector3& vTop, const Engine::Vector3& vBottom);

    public:
        virtual bool Init() override;
    };

}
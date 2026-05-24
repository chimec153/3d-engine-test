#pragma once
#include "../Component/Component.h"
#include "../Types.h"
namespace Engine
{
    // Phase E5 — Box migrated from Drawable to Component shell. Currently
    // dead at runtime. Static vertex/index data and CreateTextureVertex<T>
    // / GetTextureIndex helpers are preserved for future GameObject +
    // MeshRenderer setups that want a unit cube.
    class ENGINE_DLL Box :
        public Component
    {
        friend class Scene;

    public:
        Box();
        Box(const Box& box);
    public:
        virtual ~Box() noexcept override;

    private:
        static std::vector<VertexTexture> vertex;
        static std::vector<unsigned int> index;

    public:
        virtual void Update(float fDeltaTime) override;
        virtual std::shared_ptr<Component> Clone() override;

    public:
        void SetDefaultVertexAndIndex();
        void SetTextureVertexAndIndex();

    public:
        template <typename T>
        static std::vector<T> CreateTextureVertex()
        {
            std::vector<T> vecVertex(24);

            for (int i = 0; i < 3; ++i)
            {
                vecVertex[i * 8].pos.x = vertex[0].pos.x;
                vecVertex[i * 8].pos.y = vertex[0].pos.y;
                vecVertex[i * 8].pos.z = vertex[0].pos.z;
                vecVertex[i * 8 + 1].pos.x = vertex[1].pos.x;
                vecVertex[i * 8 + 1].pos.y = vertex[1].pos.y;
                vecVertex[i * 8 + 1].pos.z = vertex[1].pos.z;
                vecVertex[i * 8 + 2].pos.x = vertex[2].pos.x;
                vecVertex[i * 8 + 2].pos.y = vertex[2].pos.y;
                vecVertex[i * 8 + 2].pos.z = vertex[2].pos.z;
                vecVertex[i * 8 + 3].pos.x = vertex[3].pos.x;
                vecVertex[i * 8 + 3].pos.y = vertex[3].pos.y;
                vecVertex[i * 8 + 3].pos.z = vertex[3].pos.z;
                vecVertex[i * 8 + 4].pos.x = vertex[4].pos.x;
                vecVertex[i * 8 + 4].pos.y = vertex[4].pos.y;
                vecVertex[i * 8 + 4].pos.z = vertex[4].pos.z;
                vecVertex[i * 8 + 5].pos.x = vertex[5].pos.x;
                vecVertex[i * 8 + 5].pos.y = vertex[5].pos.y;
                vecVertex[i * 8 + 5].pos.z = vertex[5].pos.z;
                vecVertex[i * 8 + 6].pos.x = vertex[6].pos.x;
                vecVertex[i * 8 + 6].pos.y = vertex[6].pos.y;
                vecVertex[i * 8 + 6].pos.z = vertex[6].pos.z;
                vecVertex[i * 8 + 7].pos.x = vertex[7].pos.x;
                vecVertex[i * 8 + 7].pos.y = vertex[7].pos.y;
                vecVertex[i * 8 + 7].pos.z = vertex[7].pos.z;
            }

            vecVertex[0].uv.x = 1.f;
            vecVertex[0].uv.y = 1.f;
            vecVertex[1].uv.x = 0.f;
            vecVertex[1].uv.y = 1.f;
            vecVertex[2].uv.x = 1.f;
            vecVertex[2].uv.y = 0.f;
            vecVertex[3].uv.x = 0.f;
            vecVertex[3].uv.y = 0.f;
            vecVertex[4].uv.x = 1.f;
            vecVertex[4].uv.y = 0.f;
            vecVertex[5].uv.x = 0.f;
            vecVertex[5].uv.y = 0.f;
            vecVertex[6].uv.x = 1.f;
            vecVertex[6].uv.y = 1.f;
            vecVertex[7].uv.x = 0.f;
            vecVertex[7].uv.y = 1.f;

            vecVertex[8].uv.x = 3.f / 3.f;
            vecVertex[8].uv.y = 2.f / 4.f;
            vecVertex[9].uv.x = 0.f / 3.f;
            vecVertex[9].uv.y = 2.f / 4.f;
            vecVertex[10].uv.x = 2.f / 3.f;
            vecVertex[10].uv.y = 2.f / 4.f;
            vecVertex[11].uv.x = 1.f / 3.f;
            vecVertex[11].uv.y = 2.f / 4.f;
            vecVertex[12].uv.x = 3.f / 3.f;
            vecVertex[12].uv.y = 1.f / 4.f;
            vecVertex[13].uv.x = 0.f / 3.f;
            vecVertex[13].uv.y = 1.f / 4.f;
            vecVertex[14].uv.x = 2.f / 3.f;
            vecVertex[14].uv.y = 1.f / 4.f;
            vecVertex[15].uv.x = 1.f / 3.f;
            vecVertex[15].uv.y = 1.f / 4.f;

            vecVertex[16].uv.x = 2.f / 3.f;
            vecVertex[16].uv.y = 3.f / 4.f;
            vecVertex[17].uv.x = 1.f / 3.f;
            vecVertex[17].uv.y = 3.f / 4.f;
            vecVertex[18].uv.x = 2.f / 3.f;
            vecVertex[18].uv.y = 2.f / 4.f;
            vecVertex[19].uv.x = 1.f / 3.f;
            vecVertex[19].uv.y = 2.f / 4.f;
            vecVertex[20].uv.x = 2.f / 3.f;
            vecVertex[20].uv.y = 4.f / 4.f;
            vecVertex[21].uv.x = 1.f / 3.f;
            vecVertex[21].uv.y = 4.f / 4.f;
            vecVertex[22].uv.x = 2.f / 3.f;
            vecVertex[22].uv.y = 1.f / 4.f;
            vecVertex[23].uv.x = 1.f / 3.f;
            vecVertex[23].uv.y = 1.f / 4.f;

            return vecVertex;
        }

        static std::vector<unsigned int> GetTextureIndex();

        // Composite — fills outVerts and outInds with the textured unit
        // cube. Partial functions stay for sites that already pass them
        // inline as rvalue arguments (BindableManager's "Box" registration).
        template <typename T>
        static void BuildMesh(std::vector<T>& outVerts,
                              std::vector<unsigned int>& outInds)
        {
            outVerts = CreateTextureVertex<T>();
            outInds  = GetTextureIndex();
        }
    };

}

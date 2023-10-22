#pragma once

#include "Bindable.h"
#include "../Types.h"
#include "PixelCBuffer.h"

namespace Engine
{
    template <typename T>
    class VertexCBuffer;

    class ENGINE_DLL Material :
        public Bindable
    {
    public:
        Material();
        Material(const std::shared_ptr<class PixelCBuffer<MATERIAL>>& pBuffer);
        Material(const Material& material);
        virtual ~Material() override = default;

    private:
        MATERIAL m_tMaterial;
        std::shared_ptr<class VertexCBuffer<MATERIAL>>    m_pVertexCBuffer;
        std::shared_ptr<class PixelCBuffer<MATERIAL>>    m_pPixelCBuffer;

    public:
        void SetDiffuseColor(float r, float g, float b, float w);
        void SetAmbientColor(float r, float g, float b, float w);
        void SetSpecularColor(float r, float g, float b, float w);
        void SetDiffuseColor(const Vector4& color);
        void SetAmbientColor(const Vector4& color);
        void SetSpecularColor(const Vector4& color);
        void SetEmissiveColor(const Vector4& color);
        void SetShininess(float fShininess);
        void SetReflectivity(float fReflectivity);
        void SetRandomColor();
        const MATERIAL& GetMaterial()   const;
        void SetMaterial(const MATERIAL& mtrl);
        void SetContainerIndex(int iIndex);
        void SetRoughnessX(float x);
        void SetRoughnessY(float y);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };

}
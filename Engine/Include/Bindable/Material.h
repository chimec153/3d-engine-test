#pragma once

#include "Bindable.h"
#include "../Types.h"
#include <array>

namespace Engine
{
    template <typename T>
    class ConstantBuffer;
    class Texture;

    class ENGINE_DLL Material :
        public Bindable
    {
    public:
        // Fixed-slot texture container owned by the Material. Index N maps
        // to kMaterialSlotRegisters[N] (the actual t-register in shared.hlsl).
        // nullptr at any index means "no texture for this slot" — Material::Bind
        // pushes a null SRV to that t-register so HLSL `GetDimensions(...)`
        // reports (0,0) and the shader takes its uniform-fallback branch
        // (e.g. g_vMaterialRoughness for slot 4). Critically, this also
        // ensures no stale SRV from the previous mesh leaks into this draw.
        //
        // Layout chosen to match the per-mesh texture slots in
        // anisotropic_microfacet.hlsl:
        //   0 → t0  Diffuse
        //   1 → t1  Normal
        //   2 → t2  Specular
        //   3 → t3  Emissive
        //   4 → t6  Roughness
        //   5 → t8  AO
        //   6 → t9  Metalness
        static constexpr int kMaterialSlotCount = 7;
        static const int kMaterialSlotRegisters[kMaterialSlotCount];

    public:
        Material();
        Material(const std::shared_ptr<class ConstantBuffer<MATERIAL>>& pBuffer);
        Material(const Material& material);
        virtual ~Material() override = default;

    private:
        MATERIAL m_tMaterial;
        std::shared_ptr<class ConstantBuffer<MATERIAL>>    m_pConstantBuffer;

        // Slot N → kMaterialSlotRegisters[N]'s t-register.
        std::array<std::shared_ptr<Texture>, kMaterialSlotCount> m_vecTexture;

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
        void SetRoughnessX(float x);
        void SetRoughnessY(float y);
        void UsePaperBurn();

        // UE의 머티리얼 Shading Model 드롭다운 + MID 스칼라 파라미터에 대응.
        // ShadingModel: 0=DefaultLit(기존 PBR), 1=Toon(밴드 NDotL+하드 림), 2=Unlit
        // HitFlash: xyz=색, w=강도(0..1). PS에서 baseColor와 lerp.
        // TickHitFlash: 강도를 dt*decay만큼 감쇠 — 게임 측 매 프레임 호출용.
        void SetShadingModel(int iModel);
        int  GetShadingModel() const;
        void SetHitFlash(const Vector3& vColor, float fIntensity);
        void SetHitFlashIntensity(float fIntensity);
        float GetHitFlashIntensity() const;
        void TickHitFlash(float fDeltaTime, float fDecayPerSecond = 6.f);

        // Texture slot access. `iSlotIdx` is the Material slot index (0~6),
        // NOT the underlying t-register. Use SlotRegisterToIndex to map
        // from a t-register if needed.
        void SetTexture(int iSlotIdx, const std::shared_ptr<class Texture>& pTexture);
        std::shared_ptr<class Texture> GetTexture(int iSlotIdx) const;
        static int SlotRegisterToIndex(int iRegister);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual void Bind() override;
        virtual std::shared_ptr<Bindable> Clone() override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
        // Legacy reader for the pre-textures-on-material .mesh format.
        // Skips the 7-slot texture block that the new format appends —
        // old files have nothing past the 80-byte MATERIAL struct.
        // Only Mesh::Load's old-format branch should call this.
        void LoadLegacy(FILE* pFile);
    };

}
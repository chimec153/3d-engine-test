#pragma once
#include "Collider.h"
namespace Engine
{
    class ENGINE_DLL ColliderLine :
        public Collider
    {
        friend class BindableManager<ColliderLine>;
        friend class Drawable;

    public:
        ColliderLine();
        ColliderLine(const ColliderLine& line);
        virtual ~ColliderLine() override = default;

    private:
        Vector3 m_vStartOffset;
        Vector3 m_vEndOffset;
        LINECOLLIDERINFO    m_tInfo;

    public:
        const LINECOLLIDERINFO& GetInfo()   const;
        void SetStartOffset(const Vector3& vOffset);
        void SetEndOffset(const Vector3& vOffset);
        void SetStartOffset(float x, float y, float z);
        void SetEndOffset(float x, float y, float z);

    public:
        virtual void Update(float fDeltaTime) override;
        virtual bool Collision(class Collider* pDest, float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;
        virtual std::shared_ptr<Bindable> Clone() override;
    };

}
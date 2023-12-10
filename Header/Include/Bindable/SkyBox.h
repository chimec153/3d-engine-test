#pragma once
#include "Drawable.h"
namespace Engine
{
    class ENGINE_DLL SkyBox :
        public Drawable
    {
    public:
        SkyBox();
        SkyBox(const TCHAR* pTexturePath, const std::string& strKey = TEXTURE_PATH);
        virtual ~SkyBox() override = default;

    public:
        virtual bool Init() override;
        virtual void Update(float fDeltaTime) override;
        virtual void PreDraw(float fDeltaTime) override;

    public:
        virtual void Save(FILE* pFile) override;
        virtual void Load(FILE* pFile) override;
    };
}
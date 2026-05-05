#pragma once
#include "../Component/Component.h"
namespace Engine
{
    // Phase B.4 — SoundBindable migrated from Bindable to Component.
    // Pure CPU 3D-audio attachment (uses owner Drawable's Transform for
    // listener-relative position). No GPU bindings.
    class ENGINE_DLL SoundBindable :
        public Component
    {
    public:
        SoundBindable(const std::string& strSound);
        SoundBindable(const SoundBindable& tBindable);
        virtual ~SoundBindable() override = default;

    private:
        std::shared_ptr<class Sound> m_pSound;

    public:
        void Play();
        void Stop();
        void Resume();
        void Toggle();

    public:
        virtual void Update(float fDeltaTime) override;
    public:
        virtual std::shared_ptr<Component> Clone() override;
    };
}
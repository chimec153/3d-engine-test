#pragma once
#include "Bindable.h"
namespace Engine
{
    class ENGINE_DLL SoundBindable :
        public Bindable
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
        virtual std::shared_ptr<Bindable> Clone() override;
    };
}
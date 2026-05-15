#pragma once

#include <functional>
#include <vector>

namespace Engine
{
    // Central registry of `BindableManager<T>::DestroyInst` callbacks so the
    // editor / client can release every Bindable-cached D3D resource in one
    // call before tearing down Graphics.
    //
    // Why this exists: BindableManager<T> is a per-type template singleton,
    // and there is no compile-time way to enumerate every T the program
    // instantiates. Without this registry the singletons would outlive the
    // D3D11 device, leaving hundreds of "live object" warnings on shutdown.
    // Each BindableManager<T>::GetInst now registers its own destroyer on
    // first use, and `DestroyAll` invokes them in registration order.
    class ENGINE_DLL BindableRegistry
    {
    private:
        // Function-local static so initialisation order across translation
        // units doesn't matter — the first registrar gets a fresh vector.
        static std::vector<std::function<void()>>& Destroyers()
        {
            static std::vector<std::function<void()>> vec;
            return vec;
        }

    public:
        static void Register(std::function<void()> fn)
        {
            Destroyers().push_back(std::move(fn));
        }

        // Call every registered destroyer, then clear the list. Safe to
        // call multiple times (the vector is empty after the first call).
        static void DestroyAll()
        {
            auto& vec = Destroyers();
            for (auto& fn : vec)
            {
                if (fn) fn();
            }
            vec.clear();
        }
    };
}

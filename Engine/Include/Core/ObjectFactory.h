#pragma once

#include "Macro.h"
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Engine
{
    class Scene;
    class GameObject;

    // Cross-DLL type registry. Methods live in Engine.dll so every loaded
    // module shares one map. Game.dll calls Register() on DllMain-time static
    // init; Editor.exe (which never sees the concrete game types) looks up
    // by string and creates objects via stored lambdas.
    //
    // Hot-reload not supported: unloading the registering DLL leaves dangling
    // function pointers in the map.
    class ENGINE_DLL SceneFactory
    {
    public:
        using Ctor = std::function<Scene*()>;
        static void Register(const std::string& name, Ctor ctor);
        static Scene* Create(const std::string& name);
        static std::vector<std::string> ListAll();
    };

    class ENGINE_DLL GameObjectFactory
    {
    public:
        using Ctor = std::function<GameObject*()>;
        static void Register(const std::string& name, Ctor ctor);
        static GameObject* Create(const std::string& name);
        static std::vector<std::string> ListAll();
    };
}

// TYPE = fully qualified type (e.g. Client::GameScene)
// NAME = bare identifier used as registry key AND token suffix
#define REGISTER_SCENE(TYPE, NAME) \
    namespace { \
        static const bool _reg_scene_##NAME = []() { \
            Engine::SceneFactory::Register(#NAME, [](){ return dbg_new TYPE(); }); \
            return true; \
        }(); \
    }

#define REGISTER_GAMEOBJECT(TYPE, NAME) \
    namespace { \
        static const bool _reg_go_##NAME = []() { \
            Engine::GameObjectFactory::Register(#NAME, [](){ return dbg_new TYPE(); }); \
            return true; \
        }(); \
    }

// Use when the GameObject lacks a default constructor. CTOR_EXPR must be an
// expression producing a `new YourType(...)`.
#define REGISTER_GAMEOBJECT_EX(NAME, CTOR_EXPR) \
    namespace { \
        static const bool _reg_go_##NAME = []() { \
            Engine::GameObjectFactory::Register(#NAME, [](){ return CTOR_EXPR; }); \
            return true; \
        }(); \
    }

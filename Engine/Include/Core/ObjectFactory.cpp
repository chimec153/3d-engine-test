#include "ObjectFactory.h"

namespace Engine
{
    static std::unordered_map<std::string, SceneFactory::Ctor>& SceneMap()
    {
        static std::unordered_map<std::string, SceneFactory::Ctor> map;
        return map;
    }

    void SceneFactory::Register(const std::string& name, Ctor ctor)
    {
        SceneMap()[name] = std::move(ctor);
    }

    Scene* SceneFactory::Create(const std::string& name)
    {
        auto it = SceneMap().find(name);
        if (it == SceneMap().end()) return nullptr;
        return it->second();
    }

    std::vector<std::string> SceneFactory::ListAll()
    {
        std::vector<std::string> v;
        v.reserve(SceneMap().size());
        for (auto& kv : SceneMap()) v.push_back(kv.first);
        return v;
    }

    static std::unordered_map<std::string, GameObjectFactory::Ctor>& GameObjectMap()
    {
        static std::unordered_map<std::string, GameObjectFactory::Ctor> map;
        return map;
    }

    void GameObjectFactory::Register(const std::string& name, Ctor ctor)
    {
        GameObjectMap()[name] = std::move(ctor);
    }

    GameObject* GameObjectFactory::Create(const std::string& name)
    {
        auto it = GameObjectMap().find(name);
        if (it == GameObjectMap().end()) return nullptr;
        return it->second();
    }

    std::vector<std::string> GameObjectFactory::ListAll()
    {
        std::vector<std::string> v;
        v.reserve(GameObjectMap().size());
        for (auto& kv : GameObjectMap()) v.push_back(kv.first);
        return v;
    }
}

#pragma once
#include "Bindable.h"
#include <string>
namespace Engine
{
    class ENGINE_DLL Shader :
        public Bindable
    {
    public:
        Shader(const TCHAR* pFilePath, const char* pEntry);
        virtual ~Shader()   override = default;

    private:
        CPtr<ID3DBlob> pBlob;
        std::unique_ptr<TCHAR[]> m_pFullPath;
        std::unique_ptr<char[]> m_pEntry;

    public:
        ID3DBlob* GetBlob()  const;
        // Compile m_pFullPath:m_pEntry into pBlob. pOutError == nullptr keeps the
        // original behaviour (assert + halt on a compile error — the startup
        // contract). When a buffer is supplied the error is written there and
        // the call returns false WITHOUT asserting, so the editor's hot-reload
        // can surface a typo instead of killing the process.
        bool LoadShaderFile(const char* pTarget, std::string* pOutError = nullptr);
        const std::unique_ptr<char[]>& GetEntry()  const;

        // Hot-reload: recompile from the stored source path + entry and rebuild
        // the native shader object in place. On a compile error returns false
        // and fills outError, leaving the previously-compiled shader intact (the
        // old native object is never released on failure). Subclasses expose
        // their profile + native-object creation via the two hooks below; the
        // default GetTarget() ("") marks a shader type as not hot-reloadable.
        bool Recompile(std::string& outError);

    public:
        virtual void LoadShader() = 0;
        // Declared AFTER LoadShader so these append to the end of the vtable,
        // preserving existing slot indices (ABI-safe for modules that only call
        // the older virtuals).
        virtual const char* GetTarget() const { return ""; }
        virtual void CreateFromBlob() {}
    };

    // Hot-reload every registered VS/PS/GS/CS from its source file and reset the
    // bind cache. Must run engine-side (BindableManager::GetMap is an inline
    // member that isn't exported), so the editor calls this single exported
    // entry point. Appends per-shader compile errors to outLog (overwriting it
    // with a one-line summary first); returns the number of compile failures.
    ENGINE_DLL int RecompileAllShaders(std::string& outLog);
}
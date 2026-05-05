#include "Mesh.h"
#include "../GpuResources.h"
#include "../RenderQueue.h"
#include "../../Core/PathManager.h"
#include "../../Core/Graphics.h"        // immediate context for bone-buffer update
#include "../../Bindable/FbxLoader.h"
#include "../../Bindable/Drawable.h"   // for static SetTangent helper
#include "../../Bindable/Animation.h"  // engine compute-shader skinning
#include "../../Animation/Sequence.h"
#include "../../Resource/ResourceManager.h"
#include "../../Types.h"

#include <fbxsdk/scene/geometry/fbxlayer.h>
#include <array>
#include <string>
#include <vector>
#include <cstring>

using namespace DirectX;

namespace Engine::RenderV2::Drawables
{
	namespace mesh_detail
	{
		// 2x2 RGBA fallback. Used wherever the FBX doesn't supply a texture
		// for a slot the engine shader expects (e.g., emissive). Color is
		// either neutral grey or normal-map-default depending on slot.
		std::array<uint8_t, 2 * 2 * 4> MakeFlatRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		{
			std::array<uint8_t, 2 * 2 * 4> px{};
			for (int i = 0; i < 4; ++i)
			{
				px[i * 4 + 0] = r;
				px[i * 4 + 1] = g;
				px[i * 4 + 2] = b;
				px[i * 4 + 3] = a;
			}
			return px;
		}

		std::wstring ToWide(const std::string& s)
		{
			if (s.empty()) return {};
			int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
			std::wstring w(n > 0 ? n - 1 : 0, L'\0');
			if (n > 0) MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, w.data(), n);
			return w;
		}

		// Pull a texture path of the requested FBX type from the loader.
		std::wstring FindTexturePath(const std::vector<FbxLoader::TEXTUREINFO>& texs,
		                             fbxsdk::FbxLayerElement::EType wanted)
		{
			for (const auto& t : texs)
				if (t.type == wanted)
					return ToWide(t.strFullPath);
			return {};
		}

		std::shared_ptr<TextureRes> LoadOrFallback(ID3D11Device* device,
		                                           const std::wstring& path,
		                                           uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		{
			auto tex = std::make_shared<TextureRes>();
			if (!path.empty() && tex->LoadFromFile(device, path.c_str()))
				return tex;
			auto px = MakeFlatRGBA(r, g, b, a);
			tex->CreateFromMemory(device, 2, 2, px.data());
			return tex;
		}
	}

	XMMATRIX Mesh::TransformComp::World() const
	{
		XMMATRIX s = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX r = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX t = XMMatrixTranslation(position.x, position.y, position.z);
		return s * r * t;
	}

	Mesh::Mesh() = default;
	Mesh::~Mesh() = default;

	void Mesh::SetPosition(const XMFLOAT3& pos)   { if (m_transform) m_transform->position = pos; }
	void Mesh::SetScale(float u)                  { if (m_transform) m_transform->scale = { u, u, u }; }
	void Mesh::SetRotation(const XMFLOAT3& r)     { if (m_transform) m_transform->rotation = r; }

	bool Mesh::Init(ID3D11Device* device, const wchar_t* fbxFile,
	                const std::string& skeletonTag, const std::string& sequenceTag)
	{
		using namespace mesh_detail;

		m_transform = std::make_shared<TransformComp>();
		// Engine FbxLoader's matConvert (Y/Z swap) leaves loaded FBX assets
		// upside-down relative to the rendered camera convention. Apply a
		// 180° X-axis pitch by default; callers can override via SetRotation
		// after Init if their content already accounts for the flip.
		m_transform->rotation = { 3.14159265f, 0.0f, 0.0f };

		// --- 1. Load FBX or OBJ ------------------------------------------
		// Reuses the engine's FbxLoader (handles both formats — FBX through
		// the SDK, OBJ via its own parser). Extension dispatch.
		FbxLoader loader;
		if (!loader.Init()) return false;
		const size_t fnLen = wcslen(fbxFile);
		const wchar_t* ext = (fnLen >= 4) ? (fbxFile + fnLen - 4) : L"";
		bool loaded = false;
		if (_wcsicmp(ext, L".obj") == 0)
		{
			if (!loader.LoadOBJ(fbxFile, MESH_PATH))
			{
				return false;
			}
			loaded = !loader.GetVertexData(0).empty();
		}
		else
		{
			loaded = loader.LoadFile(fbxFile, MESH_PATH);
		}
		if (!loaded) return false;

		std::vector<VertexStandard>& srcVerts = loader.GetVertexData(0);   // mutable: tangent fill below
		const auto& srcIndexGroups = loader.GetIndexData(0);
		if (srcVerts.empty() || srcIndexGroups.empty() || srcIndexGroups[0].empty())
			return false;
		const std::vector<unsigned int>& srcIndices = srcIndexGroups[0];

		// Walking.fbx ships without tangents. NormalShader.hlsl computes
		// lighting in tangent space, so zero tangents collapse to a NaN/0
		// path that produces uniform shading. Use the engine's existing
		// CPU tangent generator (UV-gradient based, normalises + handedness).
		if (!loader.IsCalculatedTangent())
			Engine::Drawable::SetTangent(srcVerts, srcIndices);

		// Sanity-check normal/tangent — uniform shading often = zero normals.
		{
			int zeroN = 0, zeroT = 0;
			float nMin = 1e30f, nMax = -1e30f, tMin = 1e30f, tMax = -1e30f;
			for (const auto& v : srcVerts)
			{
				float nL = sqrtf(v.normal.x*v.normal.x + v.normal.y*v.normal.y + v.normal.z*v.normal.z);
				float tL = sqrtf(v.tangent.x*v.tangent.x + v.tangent.y*v.tangent.y + v.tangent.z*v.tangent.z);
				if (nL < 1e-4f) ++zeroN;
				if (tL < 1e-4f) ++zeroT;
				if (nL < nMin) nMin = nL; if (nL > nMax) nMax = nL;
				if (tL < tMin) tMin = tL; if (tL > tMax) tMax = tL;
			}
			char buf[256];
			sprintf_s(buf, "[RenderV2::Mesh] normalLen[%.3f..%.3f] tangentLen[%.3f..%.3f] "
			               "zeroN=%d zeroT=%d (of %zu)\n",
			          nMin, nMax, tMin, tMax, zeroN, zeroT, srcVerts.size());
			::OutputDebugStringA(buf);
		}

		// --- 2. Shaders — engine's NormalShader.hlsl, reused as-is --------
		const TCHAR* shaderDir = CPathManager::GetInst()->FindPath(SHADER_PATH);
		std::wstring shaderPath = (shaderDir ? shaderDir : L"") + std::wstring(L"NormalShader.hlsl");

		// VSSkin reads position through up to 4 weighted bone matrices.
		// Phase 2.4d-1: identity matrices uploaded each frame → output is
		// identical to the unskinned VS path (regression-safe baseline).
		auto vs = std::make_shared<VertexShaderRes>();
		if (!vs->LoadFromFile(device, shaderPath.c_str(), "VSSkin")) return false;

		auto ps = std::make_shared<PixelShaderRes>();
		if (!ps->LoadFromFile(device, shaderPath.c_str(), "PS")) return false;

		// VS_SkinIn = { tangent, blendIndices, pos, normal, blendWeight, uv } —
		// matches VertexStandard's actual layout for direct interleaving.
		const D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
			{"TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(VertexStandard, tangent),       D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(VertexStandard, blendIndecies), D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(VertexStandard, pos),           D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(VertexStandard, normal),        D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(VertexStandard, blendWeight),   D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(VertexStandard, uv),            D3D11_INPUT_PER_VERTEX_DATA, 0},
		};
		auto layout = std::make_shared<InputLayoutRes>();
		if (!layout->Create(device, layoutDesc, _countof(layoutDesc),
		                    vs->Bytecode().data(), vs->Bytecode().size())) return false;

		// --- 3. Buffers ---------------------------------------------------
		auto vb = std::make_shared<VertexBufferRes>();
		if (!vb->Create(device, srcVerts.data(), srcVerts.size() * sizeof(VertexStandard))) return false;

		auto ib = std::make_shared<IndexBufferRes>();
		if (!ib->Create(device, srcIndices.data(), srcIndices.size() * sizeof(unsigned int))) return false;

		auto cbTransform = std::make_shared<ConstantBufferRes>();
		if (!cbTransform->Create(device, sizeof(EngineTransformCB))) return false;

		auto cbMaterial = std::make_shared<ConstantBufferRes>();
		if (!cbMaterial->Create(device, sizeof(EngineMaterialCB))) return false;

		// --- Skeleton + animation (Phase 2.4d-2) -------------------------
		// Extract per-bone keyframes from sequence 0 and inverse bind poses
		// from the skeleton. Submit() interpolates per-bone TRS each frame
		// and uploads to the GPU StructuredBuffer at t30.
		const auto& skel = loader.GetSkeleton(0);
		m_boneCount = skel.vecBone.empty() ? 1 : static_cast<int>(skel.vecBone.size());

		// Inverse bind pose per bone (Engine::Matrix is row-major float[16]).
		m_invBindPose.assign(m_boneCount, XMMatrixIdentity());
		for (int b = 0; b < (int)skel.vecBone.size(); ++b)
		{
			XMFLOAT4X4 f;
			memcpy(&f, skel.vecBone[b].matInvBindPose.f, sizeof(float) * 16);
			m_invBindPose[b] = XMLoadFloat4x4(&f);
		}

		// Sequence 0's keyframes — convert FbxAMatrix → XMMATRIX once.
		const auto& seqs = loader.GetSequences(0);
		m_anim.assign(m_boneCount, {});
		if (!seqs.empty())
		{
			const auto& seq = seqs[0];
			for (const auto& boneFrames : seq.vecBoneKeyFrame)
			{
				int idx = boneFrames.iBoneIndex;
				if (idx < 0 || idx >= m_boneCount) continue;
				BoneAnim& a = m_anim[idx];
				a.times.reserve(boneFrames.vecKeyFrame.size());
				a.matrices.reserve(boneFrames.vecKeyFrame.size());
				for (const auto& kf : boneFrames.vecKeyFrame)
				{
					a.times.push_back((float)kf.dTime);
					const auto& m = kf.matTransform;
					a.matrices.push_back(XMMATRIX(
						(float)m[0][0], (float)m[0][1], (float)m[0][2], (float)m[0][3],
						(float)m[1][0], (float)m[1][1], (float)m[1][2], (float)m[1][3],
						(float)m[2][0], (float)m[2][1], (float)m[2][2], (float)m[2][3],
						(float)m[3][0], (float)m[3][1], (float)m[3][2], (float)m[3][3]));
					if (a.times.back() > m_animMaxTime) m_animMaxTime = a.times.back();
				}
			}
		}

		// Bone buffer — sized once, refreshed per frame.
		m_boneMatrices.assign(m_boneCount, {});
		XMMATRIX I = XMMatrixIdentity();
		for (auto& m : m_boneMatrices) XMStoreFloat4x4(&m, I);

		auto boneBuf = std::make_shared<StructuredBufferRes>();
		if (!boneBuf->Create(device, sizeof(XMFLOAT4X4), m_boneCount)) return false;
		{
			ID3D11DeviceContext* ctxImm = nullptr;
			device->GetImmediateContext(&ctxImm);
			boneBuf->Update(ctxImm, m_boneMatrices.data(), m_boneMatrices.size() * sizeof(XMFLOAT4X4));
			ctxImm->Release();
		}

		// --- 4. Material from FBX -----------------------------------------
		// NormalShader's ambient term is `g_vDiffuseColor * T * g_vAmbientColor`
		// and the directional-light term adds `T * max(NdotL, 0)` on top.
		// FBX materials often have ambient = (0,0,0) (fully black on shadow
		// side) OR we previously forced (1,1,1) (saturates light → no shading
		// variation). 0.3 strikes a Phong-style balance: dim side stays at
		// 30% of texture, lit side reaches ~100%.
		EngineMaterialCB mat = {};
		const auto& mats = loader.GetMaterials(0);
		auto nonZero3 = [](float a, float b, float c) { return a > 0 || b > 0 || c > 0; };
		if (!mats.empty())
		{
			const auto& m = mats[0].tMaterial;
			mat.vDiffuseColor   = nonZero3(m.diffuseColor.x, m.diffuseColor.y, m.diffuseColor.z)
				? DirectX::XMFLOAT4{ m.diffuseColor.x, m.diffuseColor.y, m.diffuseColor.z, 1.0f }
				: DirectX::XMFLOAT4{ 1, 1, 1, 1 };
			mat.vAmbientColor   = { 0.3f, 0.3f, 0.3f, 1.0f };  // baseline for shadow side
			mat.vSpecularColor  = nonZero3(m.specularColor.x, m.specularColor.y, m.specularColor.z)
				? DirectX::XMFLOAT4{ m.specularColor.x, m.specularColor.y, m.specularColor.z, 1.0f }
				: DirectX::XMFLOAT4{ 0.3f, 0.3f, 0.3f, 1.0f };
			mat.vEmissiveColor  = { m.emissiveColor.x, m.emissiveColor.y, m.emissiveColor.z, 1.0f };
			mat.fMaterialSpecPower = (m.fSpecPower > 0.0f) ? m.fSpecPower : 32.0f;
			mat.fMaterialFraction  = m.fFraction;
		}
		else
		{
			mat.vDiffuseColor      = { 1, 1, 1, 1 };
			mat.vAmbientColor      = { 0.3f, 0.3f, 0.3f, 1 };
			mat.vSpecularColor     = { 0.3f, 0.3f, 0.3f, 1 };
			mat.fMaterialSpecPower = 32.0f;
		}

		// Material is per-mesh — write once via Map and let RenderQueue
		// handle re-binding (it skips Update when materialCB pointer stays
		// the same as the previous draw command).
		{
			ID3D11DeviceContext* devCtx = nullptr;
			device->GetImmediateContext(&devCtx);
			D3D11_MAPPED_SUBRESOURCE mapped = {};
			if (devCtx && SUCCEEDED(devCtx->Map(cbMaterial->Handle(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			{
				memcpy(mapped.pData, &mat, sizeof(mat));
				devCtx->Unmap(cbMaterial->Handle(), 0);
			}
			if (devCtx) devCtx->Release();
		}

		// --- 5. Textures (t0=diffuse, t1=normal, t2=specular, t3=emissive) -
		const auto& texs = loader.GetTextures(0);
		std::wstring diffusePath  = FindTexturePath(texs, fbxsdk::FbxLayerElement::eTextureDiffuse);
		std::wstring normalPath   = FindTexturePath(texs, fbxsdk::FbxLayerElement::eTextureNormalMap);
		std::wstring specularPath = FindTexturePath(texs, fbxsdk::FbxLayerElement::eTextureSpecular);
		std::wstring emissivePath = FindTexturePath(texs, fbxsdk::FbxLayerElement::eTextureEmissive);

		auto texDiffuse  = LoadOrFallback(device, diffusePath,  255, 255, 255, 255); // white fallback
		auto texNormal   = LoadOrFallback(device, normalPath,   128, 128, 255, 255); // flat normal (0,0,1) in tangent space
		auto texSpecular = LoadOrFallback(device, specularPath, 64,  64,  64,  255); // mid-grey
		auto texEmissive = LoadOrFallback(device, emissivePath, 0,   0,   0,   255); // black

		auto smp = std::make_shared<SamplerRes>();
		if (!smp->CreateLinearWrap(device)) return false;

		// --- 6. Proxy -----------------------------------------------------
		m_proxy.vs            = vs;
		m_proxy.ps            = ps;
		m_proxy.inputLayout   = layout;
		m_proxy.vertexBuffer  = vb;
		m_proxy.indexBuffer   = ib;
		m_proxy.perObjectCB   = cbTransform;
		m_proxy.materialCB    = cbMaterial;
		m_proxy.texturePS0    = texDiffuse;
		m_proxy.texturePS1    = texNormal;
		m_proxy.texturePS2    = texSpecular;
		m_proxy.texturePS3    = texEmissive;
		m_proxy.samplerPS0    = smp;
		m_proxy.boneBufferVS  = boneBuf;
		m_proxy.vertexStride  = sizeof(VertexStandard);
		m_proxy.indexFormat   = IndexFormat::UInt32;
		m_proxy.topology      = Topology::TriangleList;
		m_proxy.indexCount    = static_cast<uint32_t>(srcIndices.size());
		m_proxy.vsId          = 2;   // distinct from primitives' vsId=1
		m_proxy.psId          = 2;
		m_proxy.layer         = 0;

		// Optional: engine Animation (compute-shader skinning). Skeleton +
		// sequence must already be registered via ResourceManager before
		// Mesh::Init runs. If the tag is missing the Animation falls back
		// off (bind would otherwise crash on null compute buffers); the V2
		// CPU path still runs so the model stays visible (just unanimated).
		if (!skeletonTag.empty())
		{
			auto anim = std::make_shared<Engine::Animation>();
			anim->SetSkeleton(skeletonTag);
			if (!anim->GetSkeleton())
			{
				char buf[256];
				sprintf_s(buf, "[RenderV2::Mesh] skeletonTag '%s' not registered — Animation disabled\n",
				          skeletonTag.c_str());
				::OutputDebugStringA(buf);
			}
			else
			{
				// Check ResourceManager directly first — Animation's
				// FindAndAddSequence asserts on missing tag rather than
				// returning gracefully.
				if (!sequenceTag.empty())
				{
					auto pSeq = Engine::ResourceManager::GetInst()->FindSequence(sequenceTag);
					if (pSeq)
					{
						anim->FindAndAddSequence(sequenceTag);
						anim->ChangeSequence(sequenceTag);
					}
					else
					{
						char buf[256];
						sprintf_s(buf, "[RenderV2::Mesh] sequenceTag '%s' not registered\n",
						          sequenceTag.c_str());
						::OutputDebugStringA(buf);
					}
				}
				m_engineAnimation = anim;
			}
		}

		m_ready = true;
		return true;
	}

	void Mesh::Update(float dt)
	{
		if (!m_transform) return;
		m_transform->rotation.y += dt * 0.4f;

		if (m_engineAnimation)
		{
			// Engine path advances time + (next frame) compute pose.
			m_engineAnimation->Update(dt);
		}
		else if (m_animMaxTime > 0.0f)
		{
			// V2 fallback CPU path (current — produces upside-down anim).
			m_animTime += dt;
			if (m_animTime >= m_animMaxTime)
				m_animTime = fmodf(m_animTime, m_animMaxTime);
		}
	}

	void Mesh::Submit(RenderQueue& queue, const FrameInfo& frame)
	{
		if (!m_ready) return;

		// Two paths:
		// (1) Engine Animation present  → its compute shader runs at flush
		//     time (preDraw hook), binds FinalBuffer at t30. We don't bind
		//     our own bone buffer.
		// (2) Fallback (no Animation)   → CPU TRS evaluation → upload
		//     identity-ish bones to our own StructuredBuffer at t30.
		const bool useEngineAnim = (m_engineAnimation != nullptr);

		if (!useEngineAnim)
		{
			for (int b = 0; b < m_boneCount; ++b)
			{
				const BoneAnim& a = m_anim[b];
				XMMATRIX bone;
				if (a.matrices.empty())
				{
					bone = XMMatrixIdentity();
				}
				else if (a.matrices.size() == 1)
				{
					bone = a.matrices[0];
				}
				else
				{
					size_t i = 0;
					while (i + 1 < a.times.size() && a.times[i + 1] < m_animTime) ++i;
					size_t j = (i + 1 < a.times.size()) ? i + 1 : i;
					float t0 = a.times[i], t1 = a.times[j];
					float alpha = (t1 > t0) ? (m_animTime - t0) / (t1 - t0) : 0.0f;
					if (alpha < 0.0f) alpha = 0.0f; else if (alpha > 1.0f) alpha = 1.0f;

					XMVECTOR s0, r0, p0, s1, r1, p1;
					XMMatrixDecompose(&s0, &r0, &p0, a.matrices[i]);
					XMMatrixDecompose(&s1, &r1, &p1, a.matrices[j]);
					XMVECTOR s = XMVectorLerp(s0, s1, alpha);
					XMVECTOR r = XMQuaternionSlerp(r0, r1, alpha);
					XMVECTOR p = XMVectorLerp(p0, p1, alpha);
					bone = XMMatrixAffineTransformation(s, XMVectorZero(), r, p);
				}

				XMMATRIX final = m_invBindPose[b] * bone;
				XMStoreFloat4x4(&m_boneMatrices[b], XMMatrixTranspose(final));
			}

			auto* bbRes = static_cast<StructuredBufferRes*>(m_proxy.boneBufferVS.get());
			if (bbRes)
			{
				ID3D11DeviceContext* ctxImm = nullptr;
				Graphics::GetInst()->GetDevice()->GetImmediateContext(&ctxImm);
				bbRes->Update(ctxImm, m_boneMatrices.data(),
				              m_boneMatrices.size() * sizeof(XMFLOAT4X4));
				ctxImm->Release();
			}
		}

		XMMATRIX world      = m_transform->World();
		XMMATRIX worldView  = world * frame.view;
		XMMATRIX wvp        = world * frame.viewProj;

		// HLSL row-vector × column-major matrix convention requires upload
		// transpose (we store row-major in C++).
		XMStoreFloat4x4(&m_cbData.matTransform, XMMatrixTranspose(wvp));
		XMStoreFloat4x4(&m_cbData.matWorldView, XMMatrixTranspose(worldView));
		XMStoreFloat4x4(&m_cbData.matWorld,     XMMatrixTranspose(world));
		XMStoreFloat4x4(&m_cbData.matView,      XMMatrixTranspose(frame.view));
		XMStoreFloat4x4(&m_cbData.matProj,      XMMatrixTranspose(frame.proj));
		XMMATRIX I = XMMatrixIdentity();
		XMStoreFloat4x4(&m_cbData.matLightWVP, I);
		XMStoreFloat4x4(&m_cbData.matJoint,    I);
		m_cbData.iTransformJointSocket = 0;

		DrawCommand cmd  = m_proxy.BuildCommand(&m_cbData, sizeof(m_cbData));
		cmd.materialData = nullptr;   // material was uploaded once at init
		cmd.materialSize = 0;

		if (useEngineAnim)
		{
			// Engine Animation owns t30 — drop our identity bone buffer.
			cmd.boneBufferVS = nullptr;
			// Bind() runs at flush time (compute pass + state). Capture
			// shared_ptr so the Animation outlives the queued command.
			std::shared_ptr<Engine::Animation> anim = m_engineAnimation;
			cmd.preDraw = [anim]() { if (anim) anim->Bind(); };
		}

		queue.Submit(cmd);
	}
}

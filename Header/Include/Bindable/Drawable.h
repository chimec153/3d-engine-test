#pragma once
#include "../Core/Ref.h"
#include "../Core/Ptr.h"
#include "../Types.h"
#include "../Core/Window.h"
#include "Bindable.h"
#include "FbxLoader.h"

namespace Engine
{
	template <typename T>
	class ConstantBuffer;
	template <typename T>
	class ConstantBuffer;

	class ENGINE_DLL Drawable :
		public Bindable
	{
		friend class Scene;

	public:
		Drawable();
		Drawable(const Drawable& drawable);
		virtual ~Drawable() override;

	private:
		std::shared_ptr<class Transform> m_pTransform;
		std::shared_ptr<class Material> m_pMaterial;
		std::vector<std::shared_ptr<class Texture>>  m_vecTexture;
		std::shared_ptr<class Mesh> m_pMesh;
		std::shared_ptr<class VertexShader> m_pVertexShader;
		std::shared_ptr<class PixelShader> m_pPixelShader;
		std::shared_ptr<class Collider> m_pCollider;
		std::shared_ptr<class Animation> m_pAnimation;
		std::shared_ptr<class Agent>  m_pAgent;
		RENDER_LAYER m_eRenderLayer;
		size_t m_iInstanceKey;

	public:
		void SetTransform(const std::shared_ptr<class Transform>& pTransform);
		void AddChild(const std::vector<std::shared_ptr<class Bindable>>& bind);
		std::shared_ptr<class Transform> GetTransform()    const;
		std::shared_ptr<class Material> GetMaterial()    const;
		void SetMaterial(const std::shared_ptr<Material>& pMaterial);
		virtual void AddChild(const class std::shared_ptr<class Bindable>& pChild) override;
		void AddDrawable(const class std::shared_ptr<class Bindable>& pChild);
		virtual void GetInstData(char* pData, int iSize) const;
		void AddTexture(const std::shared_ptr<Texture>& pTexture);
		const std::vector<std::shared_ptr<Texture>>& GetTextures() const;
		const std::shared_ptr<Mesh>& GetMesh() const;
		void SetMesh(const std::shared_ptr<Mesh>& pBuffer);
		const std::shared_ptr<VertexShader>& GetVertexShader() const;
		void SetVertexShader(const std::shared_ptr<VertexShader>& pShader);
		const std::shared_ptr<PixelShader>& GetPixelShader() const;
		void SetPixelShader(const std::shared_ptr<PixelShader>& pShader);
		void SetCollider(const std::shared_ptr<Collider>& pCollider);
		void SetAnimation(std::shared_ptr<Animation> pAnimation);
		std::shared_ptr<Animation> GetAnimation()	const;
		void SetRenderLayer(RENDER_LAYER eLayer);
		RENDER_LAYER GetRenderLayer()	const;
		void AddSeqeunces(const std::vector<FbxLoader::SEQUENCE>& vecSequences, const std::string& strSeq = "");
		void SetAgent(std::shared_ptr<Engine::Agent> pAgent);
		void Move(const Engine::Vector3& pos);
		std::shared_ptr<Agent> GetAgent()	const;
		size_t GetInstanceKey()	const;
		void UpdateInstanceKey();

	public:
		virtual bool Init() override;
		virtual void Start() override;
		virtual void Input(float fDeltaTime) override;
		virtual void Update(float fDeltaTime) override;
		virtual void Collision(float fDeltaTime) override;
		virtual void PreDraw(float fDeltaTime) override;
		virtual void Bind() override;
		virtual void DrawShadow();
		virtual std::shared_ptr<Bindable> Clone();

	public:
		void BindExceptShader();
		void PostBindExceptShader();
		void PostBind();
		void BindChild();

	public:
		template <typename T, typename P>
		static void SetNormals(std::vector<T>& vecVertex, const std::vector<P>& vecIndex)
		{
			// triangle List
			assert(vecIndex.size() % 3 == 0);

			for (int i = 0; i < vecVertex.size(); ++i)
			{
				vecVertex[i].normal = Vector3();
			}

			for (int i = 0; i < vecIndex.size() / 3; ++i)
			{
				unsigned int iIndex = vecIndex[i * 3];
				unsigned int iIndex2 = vecIndex[i * 3 + 1];
				unsigned int iIndex3 = vecIndex[i * 3 + 2];
				const Vector3& p0 = vecVertex[iIndex].pos;
				const Vector3& p1 = vecVertex[iIndex2].pos;
				const Vector3& p2 = vecVertex[iIndex3].pos;

				const Vector3& n = (p1 - p0).Cross(p2 - p0);

				for (int j = 0; j < 3; ++j)
				{
					vecVertex[vecIndex[i * 3 + j]].normal += n;
				}
			}

			for (int i = 0; i < vecVertex.size(); ++i)
			{
				float fLength = vecVertex[i].normal.Length();

				if (!fLength) {
					continue;
				}

				vecVertex[i].normal /= fLength;
			}
		}

		template <typename T, typename P>
		static void SetNormals(std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<P>>& vecIndex)
		{
			assert(vecVertex.size() == vecIndex.size());

			for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
			{
				SetNormals(vecVertex[i], vecIndex[i]);
			}
		}

		template <typename T, typename P>
		static void SetTangent(std::vector<T>& vecVertex, const std::vector<P>& vecIndex, int iIndexOffset = 0)
		{
			std::vector<Vector3> vecBitangent(vecVertex.size());

			for (int i = 0; i < vecIndex.size() / 3; ++i)
			{
				const Vector3& p0 = vecVertex[vecIndex[i * 3]].pos;
				const Vector3& p1 = vecVertex[vecIndex[i * 3 + 1]].pos;
				const Vector3& p2 = vecVertex[vecIndex[i * 3 + 2]].pos;

				Vector3 Q1 = p1 - p0;
				Vector3 Q2 = p2 - p0;

				float s1 = vecVertex[vecIndex[i * 3 + 1]].uv.x - vecVertex[vecIndex[i * 3]].uv.x;
				float s2 = vecVertex[vecIndex[i * 3 + 2]].uv.x - vecVertex[vecIndex[i * 3]].uv.x;
				float t1 = vecVertex[vecIndex[i * 3 + 1]].uv.y - vecVertex[vecIndex[i * 3]].uv.y;
				float t2 = vecVertex[vecIndex[i * 3 + 2]].uv.y - vecVertex[vecIndex[i * 3]].uv.y;

				float D = s1 * t2 - s2 * t1;

				float Tx = t2 * Q1.x - t1 * Q2.x;
				float Ty = t2 * Q1.y - t1 * Q2.y;
				float Tz = t2 * Q1.z - t1 * Q2.z;

				float Bx = -s2 * Q1.x + s1 * Q2.x;
				float By = -s2 * Q1.y + s1 * Q2.y;
				float Bz = -s2 * Q1.z + s1 * Q2.z;

				Vector3 Tangent = { Tx ,Ty,Tz };
				Vector3 B = { Bx ,By,Bz };

				for (int j = 0; j < 3; ++j)
				{
					vecVertex[vecIndex[i * 3 + j]].tangent.x += Tx;
					vecVertex[vecIndex[i * 3 + j]].tangent.y += Ty;
					vecVertex[vecIndex[i * 3 + j]].tangent.z += Tz;

					vecBitangent[vecIndex[i * 3 + j]].x += Bx;
					vecBitangent[vecIndex[i * 3 + j]].y += By;
					vecBitangent[vecIndex[i * 3 + j]].z += Bz;
				}
			}

			for (int i = 0; i < vecBitangent.size(); ++i)
			{
				float fNormalLength = vecVertex[i].normal.Length();

				if (!fNormalLength) {
					continue;
				}

				const Vector3& N = vecVertex[i].normal / fNormalLength;

				Vector3 vTangent = { vecVertex[i].tangent.x, vecVertex[i].tangent.y, vecVertex[i].tangent.z };

				float fLength = vTangent.Length();

				if (fLength)
				{
					vTangent /= fLength;
				}

				vTangent = vTangent - N * N.Dot(vTangent);

				float fTangentLength = vTangent.Length();

				if (fTangentLength)
				{
					vTangent /= fTangentLength;
				}

				vecVertex[i].tangent.x = vTangent.x;
				vecVertex[i].tangent.y = vTangent.y;
				vecVertex[i].tangent.z = vTangent.z;

				vecVertex[i].tangent.w = N.Cross(vTangent).Dot(vecBitangent[i]) < 0.f ? -1.f : 1.f;
			}
		}
		template <typename T, typename P>
		static void SetTangent(std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<P>>& vecIndex)
		{
			assert(vecVertex.size() == vecIndex.size());

			for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
			{
				SetTangent(vecVertex[i], vecIndex[i]);
			}
		}

	public:
		virtual void Reset() override;
		void CheckRangeAndMove();
		bool UpdateInViewFrustum(const std::vector<Vector4>& vecPlanes, const std::vector<Vector4>* pvecLocalPlanes);
		bool CollisionFrustumAndSphere(const std::vector<Vector4>& vecPlanes, const Vector4& vSphereInfo, const std::vector<Vector4>* pvecLocalPlanes)	const;
		bool CollisionFrustumAndBox(const OBBINFO& tBoxInfo)	const;
		bool CollisionFrustumAndElipsoid(const ELIPSOIDINFO& tElipsoid)	const;
		void UpdateInLightViewFrustum();
		bool IsInViewFrustum()  const;
		bool IsInLightViewfFrustum()    const;
		void InViewFrustum();
		void OutViewFrustum();
		void InLightViewFrustum();
		void OutLightViewFrustum();

	public:
		void Load(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);
		void Load(const char* pFileName);
		virtual void Save(FILE* pFile) override;
		virtual void Load(FILE* pFile) override;
		void Parse(const char* pResult);

	public:
		typedef struct _tagMaterialInfo
		{
			std::shared_ptr<Material> pMaterial;
			std::vector<std::shared_ptr<class Texture>> vecTexture;
		}MATERIALINFO, * PMATERIALINFO;


	private:
		void LoadOBJ(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);
		std::vector<MATERIALINFO> LoadOBJMaterial(const char* pFileName, const std::string& strPathKey = MESH_PATH);
		std::vector<MATERIALINFO> LoadOBJMaterialFromFullPath(const char* pFullPath);
		void LoadFBX(const TCHAR* pFileName, const std::string& strPathKey = MESH_PATH);
		void SaveMesh(const std::vector<std::vector<VertexStandard>>& vecVertex, const std::vector<std::vector<std::vector<unsigned int>>>& vecIndex,
			const std::vector<std::vector<std::shared_ptr<Texture>>>& vecTexture, const std::vector<std::vector<std::shared_ptr<Material>>>& vecMaterial,
			const char* pFilePath, const std::string& strPathKey = MESH_PATH);
		void LoadMesh(const char* pFilePath, const std::string& strPathKey = MESH_PATH);

	private:
		Vector4 m_tSphereInfo;
		bool m_bInViewFrustum;
		bool m_bInLightViewFrustum;
		BOUNDING_VOLUME_TYPE m_eBoundingVolumeType;
		bool m_bUseInstance;
		bool m_bUseShadow;
		class RenderInstancing* m_pInstancing;
		int	m_iInstID;
		int m_iParentJointCount;

	public:
		void SetBoundingSphereInfo(const Vector4& vInfo);
		const Vector4& GetSphereInfo()	const;
		bool UseInstance()	const;
		void NotUseInstance();
		void NotUseShadow();
		void SetInstancing(class RenderInstancing* pInstancing);
		class RenderInstancing* GetInstancing()	const;
		int GetInstID()	const;
		void SetInstID(int iID);
		void SetParentJointCount(int iCount);

	public:
		static Vector3 solveHomogeneousEquations(std::vector<std::vector<float>> A, int size, int index) {
			int i, j;
			int n = size;

			Vector3 x = { };
			x[index] = 1.f;
			Vector3 e = {};
			Vector3 z = {};
			float zmax, emax;

			do
			{
				for (i = 0; i < n; i++)
				{
					z[i] = 0;
					for (j = 0; j < n; j++)
					{
						z[i] = z[i] + A[i][j] * x[j];
					}
				}
				zmax = fabs(z[0]);
				for (i = 1; i < n; i++)
				{
					if ((fabs(z[i])) > zmax)
						zmax = fabs(z[i]);
				}
				for (i = 0; i < n; i++)
				{
					z[i] = z[i] / zmax;
				}
				for (i = 0; i < n; i++)
				{
					e[i] = 0;
					e[i] = fabs((fabs(z[i])) - (fabs(x[i])));
				}
				emax = e[0];
				for (i = 1; i < n; i++)
				{
					if (e[i] > emax)
						emax = e[i];
				}
				for (i = 0; i < n; i++)
				{
					x[i] = z[i];
				}
			} while (emax > 0.001);

			return z;
		}

		static void DeleteDuplicateIndex(const std::vector<unsigned int>& vecIndex, std::vector<unsigned int>& vecNewIndex)
		{
			std::vector<unsigned int> _vecIndex = vecIndex;

			std::qsort(&_vecIndex[0], _vecIndex.size(), 4, [](void const* src, void const* dest) {
				if (*(unsigned int*)src < *(unsigned int*)dest)
				{
					return -1;
				}
				else if (*(unsigned int*)src > *(unsigned int*)dest)
				{
					return 1;
				}

				return 0;
				});

			unsigned int iLast = _vecIndex[0];

			vecNewIndex.push_back(iLast);

			for (size_t i = 0; i < _vecIndex.size(); ++i)
			{
				if (_vecIndex[i] != iLast)
				{
					iLast = _vecIndex[i];
					vecNewIndex.push_back(iLast);
				}
			}
		}

		template <typename T>
		static std::vector<std::vector<float>> GetTrigonalMatrix(const std::vector<T>& vecVertex)
		{
			Vector3 vCenter = {};

			int iTotalIndexCount = static_cast<int>(vecVertex.size());

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				vCenter += vecVertex[i].pos;
			}

			vCenter /= static_cast<float>(iTotalIndexCount);

			std::vector<std::vector<float>> mat(3);

			for (size_t i = 0; i < mat.size(); ++i)
			{
				mat[i].resize(3);
			}

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				const Vector3& v = vecVertex[i].pos - vCenter;

				mat[0][0] += v.x * v.x;
				mat[1][1] += v.y * v.y;
				mat[2][2] += v.z * v.z;

				mat[0][1] += v.x * v.y;
				mat[0][2] += v.x * v.z;
				mat[1][2] += v.y * v.z;
			}

			mat[1][0] = mat[0][1];
			mat[2][0] = mat[0][2];
			mat[2][1] = mat[1][2];

			mat[0][0] /= static_cast<float>(iTotalIndexCount);
			mat[0][1] /= static_cast<float>(iTotalIndexCount);
			mat[0][2] /= static_cast<float>(iTotalIndexCount);
			mat[1][0] /= static_cast<float>(iTotalIndexCount);
			mat[1][1] /= static_cast<float>(iTotalIndexCount);
			mat[1][2] /= static_cast<float>(iTotalIndexCount);
			mat[2][0] /= static_cast<float>(iTotalIndexCount);
			mat[2][1] /= static_cast<float>(iTotalIndexCount);
			mat[2][2] /= static_cast<float>(iTotalIndexCount);

			return mat;
		}

		template <typename T>
		static std::vector<std::vector<float>> GetTrigonalMatrix(const std::vector<std::vector<T>>& vecVertex)
		{
			Vector3 vCenter = {};

			int iTotalIndexCount = 0;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				for (size_t j = 0; j < vecVertex[i].size(); ++j)
				{
					vCenter += vecVertex[i][j].pos;
					++iTotalIndexCount;
				}
			}

			vCenter /= static_cast<float>(iTotalIndexCount);

			std::vector<std::vector<float>> mat(3);

			for (size_t i = 0; i < 3; ++i)
			{
				mat[i].resize(3);
			}

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				for (size_t j = 0; j < vecVertex[i].size(); ++j)
				{
					const Vector3& v = vecVertex[i][j].pos - vCenter;

					mat[0][0] += v.x * v.x;
					mat[1][1] += v.y * v.y;
					mat[2][2] += v.z * v.z;

					mat[0][1] += v.x * v.y;
					mat[0][2] += v.x * v.z;
					mat[1][2] += v.y * v.z;
				}
			}

			mat[1][0] = mat[0][1];
			mat[2][0] = mat[0][2];
			mat[2][1] = mat[1][2];

			mat[0][0] /= static_cast<float>(iTotalIndexCount);
			mat[0][1] /= static_cast<float>(iTotalIndexCount);
			mat[0][2] /= static_cast<float>(iTotalIndexCount);
			mat[1][0] /= static_cast<float>(iTotalIndexCount);
			mat[1][1] /= static_cast<float>(iTotalIndexCount);
			mat[1][2] /= static_cast<float>(iTotalIndexCount);
			mat[2][0] /= static_cast<float>(iTotalIndexCount);
			mat[2][1] /= static_cast<float>(iTotalIndexCount);
			mat[2][2] /= static_cast<float>(iTotalIndexCount);

			return mat;
		}

		static std::vector<Vector3> GetPrincipalAxis(std::vector<std::vector<float>> mat)
		{
			float bb = -(mat[0][0] + mat[1][1] + mat[2][2]);
			float cc = -(-mat[1][1] * mat[2][2] + mat[2][1] * mat[1][2] - mat[0][0] * mat[2][2] - mat[0][0] * mat[1][1] + mat[0][1] * mat[1][0] + mat[0][2] * mat[2][0]);
			float dd = mat[0][0] * mat[1][2] * mat[2][1]
				- mat[0][0] * mat[1][1] * mat[2][2]
				- mat[0][1] * mat[2][0] * mat[1][2] + mat[0][1] * mat[1][0] * mat[2][2]
				- mat[0][2] * mat[1][0] * mat[2][1] + mat[0][2] * mat[2][0] * mat[1][1];

			float b = bb / 3.f;
			float c = cc / 3.f;

			float vx1 = (-b + sqrtf(b * b - c));
			float vx2 = (-b - sqrtf(b * b - c));

			Vector3 vLambda = {};

			Vector3 vStart = { vx1 + 0.1f , (vx1 + vx2) / 2.f , vx2 - 10.f };

			for (int i = 0; i < 3; ++i)
			{
				double x0 = 0.0;
				double x1 = vStart[i];
				double y0 = 0.0;

				int iCount = 0;

				do
				{
					x0 = x1;
					y0 = x0 * x0 * x0 + bb * x0 * x0 + cc * x0 + dd;
					x1 = x0 - y0 / (3.0 * x0 * x0 + b * 6.0 * x0 + c * 3.0);
					++iCount;

					if (iCount > 10000)
					{
						break;
					}
				} while (abs(x1 - x0) > epsilon);

				vLambda[i] = (float)x1;
			}

			std::vector<Vector3> RST(3);

			for (int i = 0; i < 3; ++i)
			{
				std::vector<std::vector<float>> mat1;

				mat1 = mat;

				mat1[0][0] -= vLambda[i];
				mat1[1][1] -= vLambda[i];
				mat1[2][2] -= vLambda[i];

				RST[i] = solveHomogeneousEquations(mat1, 3, i);

				RST[i].Normalize();

				continue;

				int iFindCount = 0;

				for (int j = 0; j < 3; ++j)
				{
					float fMax = FLT_MIN;
					int iMaxRow = 0;

					for (int k = j; k < 3; ++k)
					{
						if (fMax < abs(mat1[k][j]))
						{
							fMax = abs(mat1[k][j]);
							iMaxRow = k;
						}
					}

					if (abs(mat1[iMaxRow][j]) < 0.001f)
					{
						continue;
					}

					for (int k = 0; k < 3; ++k)
					{
						float fTemp = mat1[iMaxRow][k];
						mat1[iMaxRow][k] = mat1[iFindCount][k];
						mat1[iFindCount][k] = fTemp;
					}

					float fPre = mat1[iFindCount][j];

					for (int k = 0; k < 3; ++k)
					{
						mat1[iFindCount][k] /= fPre;
					}

					for (int k = 0; k < 2; ++k)
					{
						float fTarget = mat1[(iFindCount + k + 1) % 3][j];

						for (int m = 0; m < 3; ++m)
						{
							mat1[(iFindCount + k + 1) % 3][m] -= fTarget * mat1[iFindCount][m];
						}
					}

					iFindCount++;
				}

				int iCount = 0;

				for (int j = 0; j < 3; ++j)
				{
					bool bFind = false;

					for (int k = 0; k < 3; ++k)
					{
						if (abs(mat1[2 - j][k]) >= 0.001f)
						{
							bFind = true;
							break;
						}
					}

					if (bFind)
					{
						iCount++;
					}
				}

				switch (iCount)
				{
				case 2:
					RST[i] = { -mat1[0][2], -mat1[1][2], 1.f };

					RST[i].Normalize();
					break;
				case 1:
					RST[i] = { -mat1[0][1], 1.f, 0.f };

					RST[i].Normalize();
					break;
				default:
					assert(false);
					break;
				}
			}

			if (RST[0].Close(RST[1]))
			{
				RST[0] = RST[1].Cross(RST[2]);
			}
			else if (RST[1].Close(RST[2]))
			{
				RST[2] = RST[0].Cross(RST[1]);
			}
			else if (RST[0].Close(RST[2]))
			{
				RST[2] = RST[0].Cross(RST[1]);
			}

			return RST;
		}

		template <typename T>
		static std::vector<Vector3> GetMaxtirxAndPrincipalAxis(const std::vector<std::vector<T>>& vecVertex)
		{
			return GetPrincipalAxis(GetTrigonalMatrix(vecVertex));
		}

		template <typename T>
		static std::vector<Vector3> GetMaxtirxAndPrincipalAxis(const std::vector<T>& vecVertex)
		{
			return GetPrincipalAxis(GetTrigonalMatrix(vecVertex));
		}

		template <typename T>
		void GetMaxAndMinPos(const std::vector<T>& vecVertex, const Vector3& vR, Vector3& min, Vector3& max)
		{
			float fMaxDot = FLT_MIN;
			float fMinDot = FLT_MAX;

			int iMaxIndex = 0;
			int iMinIndex = 0;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				float fDot = vecVertex[i].pos.Dot(vR);

				if (fDot > fMaxDot)
				{
					fMaxDot = fDot;
					iMaxIndex = (int)i;
				}

				if (fDot < fMinDot)
				{
					fMinDot = fDot;
					iMinIndex = (int)i;
				}
			}

			max = vecVertex[iMaxIndex].pos;
			min = vecVertex[iMinIndex].pos;
		}

		template <typename T>
		static void GetMaxAndMinPos(const std::vector<std::vector<T>>& vecVertex, const Vector3& vR, Vector3& min, Vector3& max)
		{
			float fMaxDot = FLT_MIN;
			float fMinDot = FLT_MAX;

			int iMaxIndex = 0;
			int iMinIndex = 0;

			int iMaxIndex2 = 0;
			int iMinIndex2 = 0;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				for (size_t j = 0; j < vecVertex[i].size(); ++j)
				{
					float fDot = vecVertex[i][j].pos.Dot(vR);

					if (fDot > fMaxDot)
					{
						fMaxDot = fDot;
						iMaxIndex = (int)i;
						iMaxIndex2 = (int)j;
					}

					if (fDot < fMinDot)
					{
						fMinDot = fDot;
						iMinIndex = (int)i;
						iMinIndex2 = (int)j;
					}
				}
			}

			if (iMaxIndex < 0 || iMaxIndex2 < 0 || iMinIndex < 0 || iMinIndex2 < 0)
			{
				return;
			}
			max = vecVertex[iMaxIndex][iMaxIndex2].pos;
			min = vecVertex[iMinIndex][iMinIndex2].pos;
		}

		template <typename T>
		Vector4 GetBoundingSphere(const std::vector<T>& vecVertex, const Vector3& vMax, const Vector3& vMin)
		{
			float fRadius = (vMax - vMin).Length() / 2.f;
			Vector3 vSphereCenter = (vMax + vMin) / 2.f;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				const Vector3& vDist = vSphereCenter - vecVertex[i].pos;

				float fLength = vDist.Length();

				if (fRadius < fLength)
				{
					const Vector3& vEnd = vSphereCenter + vDist / fLength * fRadius;

					vSphereCenter = (vecVertex[i].pos + vEnd) / 2.f;

					fRadius = (vecVertex[i].pos - vEnd).Length() / 2.f;
				}
			}

			return Vector4(vSphereCenter, fRadius);
		}

		template <typename T>
		static Vector4 GetBoundingSphere(const std::vector<std::vector<T>>& vecVertex, const Vector3& vMax, const Vector3& vMin)
		{
			float fRadius = (vMax - vMin).Length() / 2.f;
			Vector3 vSphereCenter = (vMax + vMin) / 2.f;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				for (size_t j = 0; j < vecVertex[i].size(); ++j)
				{
					const Vector3& vDist = vSphereCenter - vecVertex[i][j].pos;

					float fLength = vDist.Length();

					if (fRadius < fLength)
					{
						const Vector3& vEnd = vSphereCenter + vDist / fLength * fRadius;

						vSphereCenter = (vecVertex[i][j].pos + vEnd) / 2.f;

						fRadius = (vecVertex[i][j].pos - vEnd).Length() / 2.f;
					}
				}
			}

			return Vector4(vSphereCenter, fRadius);
		}

		template <typename T>
		Vector4 GetBoundingSphere(const std::vector<T>& vecVertex)
		{
			m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::SPHERE;

			std::vector<Vector3> vecAxis = GetMaxtirxAndPrincipalAxis(vecVertex);

			Vector3 vMin;
			Vector3 vMax;

			GetMaxAndMinPos(vecVertex, vecAxis[0], vMin, vMax);

			SetBoundingSphereInfo(GetBoundingSphere(vecVertex, vMax, vMin));

			return m_tSphereInfo;
		}

		template <typename T>
		static Vector4 StaticGetBoundingSphere(const std::vector<std::vector<T>>& vecVertex)
		{
			std::vector<Vector3> vecAxis = GetMaxtirxAndPrincipalAxis(vecVertex);

			Vector3 vMin;
			Vector3 vMax;

			GetMaxAndMinPos(vecVertex, vecAxis[0], vMin, vMax);

			return GetBoundingSphere(vecVertex, vMax, vMin);
		}

		template <typename T>
		Vector4 GetBoundingSphere(const std::vector<std::vector<T>>& vecVertex)
		{
			m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::SPHERE;

			SetBoundingSphereInfo(StaticGetBoundingSphere(vecVertex));

			return m_tSphereInfo;
		}
		template <typename T>
		OBBINFO GetBoundingBox(const std::vector<T>& vecVertex, const std::vector<unsigned int>& vecIndex)
		{
			m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::BOX;

			std::vector<unsigned int> vecNewIndex;

			DeleteDuplicateIndex(vecIndex, vecNewIndex);

			const std::vector<Vector3>& vecRST = GetPrincipalAxis(vecVertex, vecNewIndex);

			OBBINFO info = {};

			for (int i = 0; i < 3; ++i)
			{
				float fMax = FLT_MIN;

				float fMin = FLT_MAX;

				int iMinIndex = 0;

				int iMaxIndex = 0;

				for (size_t j = 0; j < vecNewIndex.size(); ++j)
				{
					float fDot = vecVertex[vecNewIndex[j]].pos.Dot(vecRST[i]);

					if (fMax < fDot)
					{
						fMax = fDot;
						iMaxIndex = (int)j;
					}

					if (fMin > fDot)
					{
						fMin = fDot;
						iMinIndex = (int)j;
					}
				}

				info.vCenter += vecRST[i] * (fMax + fMin) / 2.f;

				info.vAxis[i] = vecRST[i] * (fMax - fMin);
			}

			return info;
		}

		template <typename T>
		ELIPSOIDINFO GetBoundingElipsoid(const std::vector<T>& vecVertex, const std::vector<unsigned int>& vecIndex)
		{
			m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::ELIPSOID;

			const std::vector<Vector3>& vRST = GetPrincipalAxis<T>(vecVertex, vecIndex);

			std::vector<unsigned int> _vecIndex;

			DeleteDuplicateIndex(vecIndex, _vecIndex);

			Vector3 vABC = {};

			for (int i = 0; i < 3; ++i)
			{
				float fMin = FLT_MAX;
				float fMax = FLT_MIN;

				for (int j = 0; j < _vecIndex.size(); ++j)
				{
					float fDot = vRST[i].Dot(vecVertex[_vecIndex[j]].pos);

					if (fMin > fDot)
					{
						fMin = fDot;
					}

					if (fMax < fDot)
					{
						fMax = fDot;
					}
				}

				vABC[i] = (fMax - fMin) / 2.f;
			}

			Matrix mat =
				Matrix
			{
				vRST[0].x,vRST[1].x,vRST[2].x,0.f,
				vRST[0].y,vRST[1].y,vRST[2].y,0.f,
				vRST[0].z,vRST[1].z,vRST[2].z,0.f,
				0.f,0.f,0.f,0.f,
			}

			*
				Matrix
			{
				1.f / vABC[0], 0.f, 0.f, 0.f,
				0.f, 1.f / vABC[1], 0.f, 0.f,
				0.f, 0.f, 1.f / vABC[2], 0.f,
				0.f, 0.f, 0.f, 0.f,
			}

			*
				Matrix
			{
				vRST[0].x, vRST[0].y, vRST[0].z, 0.f,
				vRST[1].x, vRST[1].y, vRST[1].z, 0.f,
				vRST[2].x, vRST[2].y, vRST[2].z, 0.f,
				0.f, 0.f, 0.f, 0.f,
			};

			float fMax = 0.f;
			float fMin = 0.f;

			Vector3 vCenter;
			Vector3 vStart;
			Vector3 vEnd;

			std::vector<Vector3> _vecVertex;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				const Vector3& vPos = mat.TransformCoord(vecVertex[i].pos);

				_vecVertex.push_back(vPos);

				float fDot = vPos.Dot(vRST[0]);

				if (fMax < fDot)
				{
					fMax = fDot;
				}

				if (fMin > fDot)
				{
					fMin = fDot;
				}
			}

			vCenter = vRST[0] * (fMax + fMin) / 2.f;
			vStart = vRST[0] * fMax;
			vEnd = vRST[0] * fMin;

			float fRadius = (fMax + fMin) / 2.f;

			for (size_t i = 0; i < _vecVertex.size(); ++i)
			{
				Vector3 vDist = _vecVertex[i] - vCenter;

				float fLength = vDist.Length();

				if (fLength > fRadius)
				{
					vDist /= fLength;

					const Vector3& _vEnd = vCenter - vDist * fRadius;

					vCenter = (_vecVertex[i] + _vEnd) / 2.f;

					fRadius = (_vecVertex[i] - _vEnd).Length() / 2.f;
				}
			}

			Matrix matInverse =
				Matrix
			{
				vRST[0].x,vRST[1].x,vRST[2].x,0.f,
				vRST[0].y,vRST[1].y,vRST[2].y,0.f,
				vRST[0].z,vRST[1].z,vRST[2].z,0.f,
				0.f,0.f,0.f,0.f,
			}

			*
				Matrix
			{
				vABC[0], 0.f, 0.f, 0.f,
					0.f, vABC[1], 0.f, 0.f,
					0.f, 0.f, vABC[2], 0.f,
					0.f, 0.f, 0.f, 0.f,
			}

			*
				Matrix
			{
				vRST[0].x, vRST[0].y, vRST[0].z, 0.f,
					vRST[1].x, vRST[1].y, vRST[1].z, 0.f,
					vRST[2].x, vRST[2].y, vRST[2].z, 0.f,
					0.f, 0.f, 0.f, 0.f,
			};

			ELIPSOIDINFO info = {};

			info.vCenter = matInverse.TransformCoord(vCenter);

			for (int i = 0; i < 3; ++i)
			{
				info.vRST[i] = vRST[i] * fRadius;
			}

			return info;
		}

		template <typename T>
		CYLINDERINFO GetBoundingCylinder(const std::vector<T>& vecVertex, const std::vector<unsigned int>& vecIndex)
		{
			m_eBoundingVolumeType = BOUNDING_VOLUME_TYPE::CYLINDER;

			std::vector<unsigned int> vecNewIndex;

			DeleteDuplicateIndex(vecIndex, vecNewIndex);

			const std::vector<Vector3>& vecRST = GetPrincipalAxis(vecVertex, vecNewIndex);

			float fMax = FLT_MIN;

			float fMin = FLT_MAX;

			float fMaxHeight = FLT_MIN;

			float fMinHeight = FLT_MAX;

			std::vector<Vector3> vecPos;

			for (size_t i = 0; i < vecVertex.size(); ++i)
			{
				float fDotHeight = vecRST[0].Dot(vecVertex[i].pos);

				if (fMaxHeight < fDotHeight)
				{
					fMaxHeight = fDotHeight;
				}

				if (fMinHeight > fDotHeight)
				{
					fMinHeight = fDotHeight;
				}

				const Vector3& vPos = vecVertex[i].pos - fDotHeight * vecRST[0];

				vecPos.push_back(vPos);

				float fDot = vecRST[1].Dot(vPos);

				if (fDot > fMax)
				{
					fMax = fDot;
				}

				if (fDot < fMin)
				{
					fMin = fDot;
				}
			}

			Vector3 vCenter = vecRST[1] * (fMax + fMin) / 2.f;
			float fRadius = (fMax - fMin) / 2.f;

			for (size_t i = 0; i < vecPos.size(); ++i)
			{
				const Vector3& vDist = vecPos[i] - vCenter;

				float fLength = vDist.Length();

				if (fLength > fRadius)
				{
					vDist /= fLength;

					const Vector3& vEnd = vCenter - vDist * fRadius;

					vCenter = (vecPos[i] + vEnd) / 2.f;

					fRadius = (vecPos[i] - vEnd).Length() / 2.f;
				}
			}

			CYLINDERINFO info = {};

			info.fRadius = fRadius;
			info.vStart = vCenter + vecRST[0] * fMin;
			info.vEnd = vCenter + vecRST[0] * fMax;

			return info;
		}
	};
}
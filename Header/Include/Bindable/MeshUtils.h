#pragma once

namespace Engine
{
    // Phase E7 — extracted from Drawable's static template helpers so that
    // Mesh-building code (Cloth/Terrain/Trail/RenderV2::Mesh, MeshLoader's
    // OBJ pipeline, etc.) no longer needs to include Drawable.h. The
    // implementations are unchanged from the originals — only the home
    // moved.
    namespace MeshUtils
    {
        template <typename T, typename P>
        inline void SetNormals(std::vector<T>& vecVertex, const std::vector<P>& vecIndex)
        {
            // triangle list
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

                if (!fLength) continue;

                vecVertex[i].normal /= fLength;
            }
        }

        template <typename T, typename P>
        inline void SetNormals(std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<P>>& vecIndex)
        {
            assert(vecVertex.size() == vecIndex.size());

            for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
            {
                SetNormals(vecVertex[i], vecIndex[i]);
            }
        }

        template <typename T, typename P>
        inline void SetTangent(std::vector<T>& vecVertex, const std::vector<P>& vecIndex, int /*iIndexOffset*/ = 0)
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
                (void)D;

                float Tx = t2 * Q1.x - t1 * Q2.x;
                float Ty = t2 * Q1.y - t1 * Q2.y;
                float Tz = t2 * Q1.z - t1 * Q2.z;

                float Bx = -s2 * Q1.x + s1 * Q2.x;
                float By = -s2 * Q1.y + s1 * Q2.y;
                float Bz = -s2 * Q1.z + s1 * Q2.z;

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

                if (!fNormalLength) continue;

                const Vector3& N = vecVertex[i].normal / fNormalLength;

                Vector3 vTangent = { vecVertex[i].tangent.x, vecVertex[i].tangent.y, vecVertex[i].tangent.z };

                float fLength = vTangent.Length();

                if (fLength) vTangent /= fLength;

                vTangent = vTangent - N * N.Dot(vTangent);

                float fTangentLength = vTangent.Length();

                if (fTangentLength) vTangent /= fTangentLength;

                vecVertex[i].tangent.x = vTangent.x;
                vecVertex[i].tangent.y = vTangent.y;
                vecVertex[i].tangent.z = vTangent.z;

                vecVertex[i].tangent.w = N.Cross(vTangent).Dot(vecBitangent[i]) < 0.f ? -1.f : 1.f;
            }
        }

        template <typename T, typename P>
        inline void SetTangent(std::vector<std::vector<T>>& vecVertex, const std::vector<std::vector<P>>& vecIndex)
        {
            assert(vecVertex.size() == vecIndex.size());

            for (int i = 0; i < static_cast<int>(vecVertex.size()); ++i)
            {
                SetTangent(vecVertex[i], vecIndex[i]);
            }
        }

        // Phase E7 — moved from Drawable.h's static template helpers so the
        // MeshLoader pipeline (and any future bounding-volume consumers)
        // can compute principal axes / bounding spheres without depending
        // on Drawable. Bodies are unchanged.
        inline Vector3 solveHomogeneousEquations(std::vector<std::vector<float>> A, int size, int index)
        {
            int i, j;
            int n = size;

            Vector3 x = {};
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

        template <typename T>
        inline std::vector<std::vector<float>> GetTrigonalMatrix(const std::vector<T>& vecVertex)
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
        inline std::vector<std::vector<float>> GetTrigonalMatrix(const std::vector<std::vector<T>>& vecVertex)
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

        inline std::vector<Vector3> GetPrincipalAxis(std::vector<std::vector<float>> mat)
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
        inline std::vector<Vector3> GetMaxtirxAndPrincipalAxis(const std::vector<std::vector<T>>& vecVertex)
        {
            return GetPrincipalAxis(GetTrigonalMatrix(vecVertex));
        }

        template <typename T>
        inline std::vector<Vector3> GetMaxtirxAndPrincipalAxis(const std::vector<T>& vecVertex)
        {
            return GetPrincipalAxis(GetTrigonalMatrix(vecVertex));
        }

        template <typename T>
        inline void GetMaxAndMinPos(const std::vector<T>& vecVertex, const Vector3& vR, Vector3& min, Vector3& max)
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
        inline void GetMaxAndMinPos(const std::vector<std::vector<T>>& vecVertex, const Vector3& vR, Vector3& min, Vector3& max)
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
        inline Vector4 GetBoundingSphere(const std::vector<T>& vecVertex, const Vector3& vMax, const Vector3& vMin)
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
        inline Vector4 GetBoundingSphere(const std::vector<std::vector<T>>& vecVertex, const Vector3& vMax, const Vector3& vMin)
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

        // Convenience wrappers — compute principal axis, find min/max along
        // axis 0, and return the bounding sphere. Mirrors the non-static
        // versions on Drawable but writes nowhere; returning the sphere lets
        // the caller (MeshLoadContext / future bounding-volume consumers)
        // record it however they need.
        template <typename T>
        inline Vector4 ComputeBoundingSphere(const std::vector<T>& vecVertex)
        {
            std::vector<Vector3> vecAxis = GetMaxtirxAndPrincipalAxis(vecVertex);

            Vector3 vMin;
            Vector3 vMax;

            GetMaxAndMinPos(vecVertex, vecAxis[0], vMin, vMax);

            return GetBoundingSphere(vecVertex, vMax, vMin);
        }

        template <typename T>
        inline Vector4 ComputeBoundingSphere(const std::vector<std::vector<T>>& vecVertex)
        {
            std::vector<Vector3> vecAxis = GetMaxtirxAndPrincipalAxis(vecVertex);

            Vector3 vMin;
            Vector3 vMax;

            GetMaxAndMinPos(vecVertex, vecAxis[0], vMin, vMax);

            return GetBoundingSphere(vecVertex, vMax, vMin);
        }
    }
}

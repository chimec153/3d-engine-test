#include "VoxelMesher.h"
#include "VoxelChunk.h"
#include "../Bindable/Mesh.h"

namespace Engine
{
    namespace
    {
        // 6 faces of a unit cube. Vertices are listed in CW order viewed
        // from outside (matches DX11 default: FrontCounterClockwise=FALSE,
        // CullBack culls CCW). Triangulation: (0,1,2) + (0,2,3).
        struct CubeFace
        {
            int     dx, dy, dz;     // neighbour offset along this face's normal
            Vector3 normal;
            Vector3 offsets[4];
        };

        static const CubeFace kFaces[6] = {
            // +X
            { +1,  0,  0, { 1.f,  0.f,  0.f}, {{1,1,0},{1,1,1},{1,0,1},{1,0,0}} },
            // -X
            { -1,  0,  0, {-1.f,  0.f,  0.f}, {{0,1,1},{0,1,0},{0,0,0},{0,0,1}} },
            // +Y
            {  0, +1,  0, { 0.f,  1.f,  0.f}, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}} },
            // -Y
            {  0, -1,  0, { 0.f, -1.f,  0.f}, {{1,0,1},{0,0,1},{0,0,0},{1,0,0}} },
            // +Z
            {  0,  0, +1, { 0.f,  0.f,  1.f}, {{1,1,1},{0,1,1},{0,0,1},{1,0,1}} },
            // -Z
            {  0,  0, -1, { 0.f,  0.f, -1.f}, {{0,1,0},{1,1,0},{1,0,0},{0,0,0}} },
        };

        static const DirectX::XMFLOAT2 kFaceUV[4] = {
            {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}
        };
    }

    namespace VoxelMesher
    {
        void Build(const VoxelChunk& chunk,
                   std::vector<VertexStandard>& outVertices,
                   std::vector<unsigned int>& outIndices,
                   const NeighbourLookup& neighbour)
        {
            constexpr int N = VoxelChunk::kSize;

            const auto sample = [&](int nx, int ny, int nz) -> BlockType
            {
                if (chunk.InBounds(nx, ny, nz))
                    return chunk.GetBlock(nx, ny, nz);
                return neighbour ? neighbour(nx, ny, nz) : BlockType::Air;
            };

            for (int z = 0; z < N; ++z)
            {
                for (int y = 0; y < N; ++y)
                {
                    for (int x = 0; x < N; ++x)
                    {
                        if (!IsSolid(chunk.GetBlock(x, y, z)))
                            continue;

                        const Vector3 origin(
                            static_cast<float>(x),
                            static_cast<float>(y),
                            static_cast<float>(z));

                        for (int f = 0; f < 6; ++f)
                        {
                            const CubeFace& face = kFaces[f];

                            // Skip this face if the neighbour on the
                            // other side is solid. Chunk-edge neighbours
                            // read as Air (GetBlockSafe), so border
                            // faces are still emitted — multi-chunk
                            // boundary handling lands in Phase V5.
                            if (IsSolid(sample(
                                    x + face.dx,
                                    y + face.dy,
                                    z + face.dz)))
                            {
                                continue;
                            }

                            const unsigned int base =
                                static_cast<unsigned int>(outVertices.size());

                            for (int v = 0; v < 4; ++v)
                            {
                                VertexStandard vert = {};
                                vert.pos    = origin + face.offsets[v];
                                vert.normal = face.normal;
                                vert.uv     = kFaceUV[v];
                                outVertices.push_back(vert);
                            }

                            outIndices.push_back(base + 0);
                            outIndices.push_back(base + 1);
                            outIndices.push_back(base + 2);
                            outIndices.push_back(base + 0);
                            outIndices.push_back(base + 2);
                            outIndices.push_back(base + 3);
                        }
                    }
                }
            }
        }

        std::shared_ptr<Mesh> BuildMesh(const VoxelChunk& chunk)
        {
            std::vector<VertexStandard> verts;
            std::vector<unsigned int>   inds;
            Build(chunk, verts, inds);

            // Empty chunk → return null so callers can detect "nothing
            // to draw" and skip SetMesh (or set a sentinel). Mesh's
            // template ctor would otherwise dereference an empty
            // vector.
            if (verts.empty() || inds.empty())
                return nullptr;

            return std::make_shared<Mesh>(verts, inds);
        }
    }
}

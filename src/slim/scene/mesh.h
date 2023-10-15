#pragma once

#include "../math/vec2.h"
#include "../math/mat3.h"

#include "./bvh.h"

struct EdgeVertexIndices {
    u32 from, to;
};

union TriangleVertexIndices {
    u32 ids[3];
    struct {
        u32 v1, v2, v3;
    };
};

struct Triangle {
    mat3 local_to_tangent;
    vec3 position, normal, n1, n2, n3;
    vec2 uv1, uv2, uv3;
    f32 uv_coverage, padding;
};

#define L (-1.0f)
#define R (+1.0f)
#define B (-1.0f)
#define T (+1.0f)
#define K (-1.0f)
#define F (+1.0f)

const vec3 CUBE_VERTEX_POSITIONS[CUBE_VERTEX_COUNT] = {
    {K, B, L},
    {F, B, L},
    {F, T, L},
    {K, T, L},
    {K, B, R},
    {F, B, R},
    {F, T, R},
    {K, T, R}
};

#define KBL (0)
#define FBL (1)
#define FTL (2)
#define KTL (3)
#define KBR (4)
#define FBR (5)
#define FTR (6)
#define KTR (7)

const EdgeVertexIndices CUBE_EDGES[CUBE_TRIANGLE_EDGE_COUNT] = {
    {KBL, KBR},
    {KBR, KTR},
    {KTR, KTL},
    {KTL, KBL},

    {FBR, FBL},
    {FBL, FTL},
    {FTL, FTR},
    {FTR, FBR},

    {KBL, FBL},
    {KBR, FBR},
    {KTL, FTL},
    {KTR, FTR},

    {KBL, KTR},
    {KTR, FTL},
    {FTL, KBL},

    {KBR, FTR},
    {FTR, FBL},
    {FBL, KBR}
};

const EdgeVertexIndices CUBE_QUAD_EDGES[CUBE_QUAD_EDGE_COUNT] = {
    {KBL, KBR},
    {KBR, KTR},
    {KTR, KTL},
    {KTL, KBL},

    {KBR, FBR},
    {FBR, FTR},
    {FTR, KTR},
    {KTR, KBR},

    {FBR, FBL},
    {FBL, FTL},
    {FTL, FTR},
    {FTR, FBR},

    {FBL, KBL},
    {KBL, KTL},
    {KTL, FTL},
    {FTL, FBL},

    {KTL, KTR},
    {KTR, FTR},
    {FTR, FTL},
    {FTL, KTL},

    {KBR, KBL},
    {KBL, FBL},
    {FBL, FBR},
    {FBR, KBR}
};

const TriangleVertexIndices CUBE_VERTEX_POSITION_INDICES[CUBE_TRIANGLE_COUNT] = {
     {KBL, FBL, FTL},
     {FBL, FBR, FTR},
     {FBR, KBR, KTR},
     {KBR, KBL, KTL},
     {KTL, FTL, FTR},
     {FBL, KBL, KBR},
     {KBL, FTL, KTL},
     {FBL, FTR, FTL},
     {FBR, KTR, FTR},
     {KBR, KTL, KTR},
    {KTL, FTR, KTR},
    {FBL, KBR, FBR}
};

const vec3 CUBE_VERTEX_NORMALS[CUBE_NORMAL_COUNT] = {
    {0, 0, -1},
    {1, 0, 0},
    {0, 0, 1},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0}
};

const TriangleVertexIndices CUBE_VERTEX_NORMAL_INDICES[CUBE_TRIANGLE_COUNT] = {
    {0, 0, 0},
    {1, 1, 1},
    {2, 2, 2},
    {3, 3, 3},
    {4, 4, 4},
    {5, 5, 5},
    {0, 0, 0},
    {1, 1, 1},
    {2, 2, 2},
    {3, 3, 3},
    {4, 4, 4},
    {5, 5, 5}
};

const vec2 CUBE_QUAD_VERTEX_UVS[CUBE_QUAD_UV_COUNT] = {
    {0.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f},
    {1.0f, 0.0f},
};

const TriangleVertexIndices CUBE_QUAD_VERTEX_UV_INDICES[CUBE_TRIANGLE_COUNT] = {
    {0, 1, 2},
    {0, 1, 2},
    {0, 1, 2},
    {0, 1, 2},
    {0, 1, 2},
    {0, 1, 2},
    {0, 2, 3},
    {0, 2, 3},
    {0, 2, 3},
    {0, 2, 3},
    {0, 2, 3},
    {0, 2, 3}
};

#undef L
#undef R
#undef B
#undef T
#undef K
#undef F

#undef KBL
#undef FBL
#undef FTL
#undef KTL
#undef KBR
#undef FBR
#undef FTR
#undef KTR

enum class CubeEdgesType {
    Full,
    Quad,
    BBox
};

struct Mesh {
    AABB aabb;
    BVH bvh;
    Triangle *triangles;

    vec3 *vertex_positions{nullptr};
    vec3 *vertex_normals{nullptr};
    vec2 *vertex_uvs{nullptr};

    TriangleVertexIndices *vertex_position_indices{nullptr};
    TriangleVertexIndices *vertex_normal_indices{nullptr};
    TriangleVertexIndices *vertex_uvs_indices{nullptr};

    EdgeVertexIndices *edge_vertex_indices{nullptr};

    u32 triangle_count{0};
    u32 vertex_count{0};
    u32 edge_count{0};
    u32 normals_count{0};
    u32 uvs_count{0};

    Mesh() = default;

    Mesh(u32 triangle_count,
         u32 vertex_count,
         u32 normals_count,
         u32 uvs_count,
         u32 edge_count,

         vec3 *vertex_positions,
         vec3 *vertex_normals,
         vec2 *vertex_uvs,

         TriangleVertexIndices *vertex_position_indices,
         TriangleVertexIndices *vertex_normal_indices,
         TriangleVertexIndices *vertex_uvs_indices,

         EdgeVertexIndices *edge_vertex_indices,
         AABB aabb
    ) :
            triangle_count{triangle_count},
            vertex_count{vertex_count},
            normals_count{normals_count},
            uvs_count{uvs_count},
            edge_count{edge_count},

            vertex_positions{vertex_positions},
            vertex_normals{vertex_normals},
            vertex_uvs{vertex_uvs},

            vertex_position_indices{vertex_position_indices},
            vertex_normal_indices{vertex_normal_indices},
            vertex_uvs_indices{vertex_uvs_indices},

            edge_vertex_indices{edge_vertex_indices},
            aabb{aabb}
    {}

    void loadEdges(Edge *edges) const {
        EdgeVertexIndices *ids = edge_vertex_indices;
        for (u32 edge_index = 0; edge_index < edge_count; edge_index++, ids++)
            edges[edge_index] = {
                vertex_positions[ids->from],
                vertex_positions[ids->to]
            };
    }

    explicit Mesh(CubeEdgesType edges, bool quad_uvs = true) {
        loadCube(edges, quad_uvs);
    }

    void loadCube(CubeEdgesType edges = CubeEdgesType::BBox, bool quad_uvs = true) {
        triangle_count = CUBE_TRIANGLE_COUNT;
        vertex_count = CUBE_VERTEX_COUNT;
        normals_count = CUBE_NORMAL_COUNT;

        vertex_positions = (vec3*)CUBE_VERTEX_POSITIONS;
        vertex_normals = (vec3*)CUBE_VERTEX_NORMALS;

        vertex_position_indices = (TriangleVertexIndices*)CUBE_VERTEX_POSITION_INDICES;
        vertex_normal_indices = (TriangleVertexIndices*)CUBE_VERTEX_NORMAL_INDICES;

        if (quad_uvs) {
            uvs_count = CUBE_QUAD_UV_COUNT;
            vertex_uvs = (vec2*)CUBE_QUAD_VERTEX_UVS;
            vertex_uvs_indices = (TriangleVertexIndices*)CUBE_QUAD_VERTEX_UV_INDICES;
        } else {
            uvs_count = CUBE_FLAT_UV_COUNT;
            vertex_uvs = (vec2*)CUBE_QUAD_VERTEX_UVS;
            vertex_uvs_indices = (TriangleVertexIndices*)CUBE_QUAD_VERTEX_UVS;
        }

        switch (edges) {
            case CubeEdgesType::Full:
                edge_count = CUBE_TRIANGLE_EDGE_COUNT;
                edge_vertex_indices = (EdgeVertexIndices*)CUBE_EDGES;
                break;
            case CubeEdgesType::BBox:
                edge_count = CUBE_BBOX_EDGE_COUNT;
                edge_vertex_indices = (EdgeVertexIndices*)CUBE_EDGES;
                break;
            case CubeEdgesType::Quad:
                edge_count = CUBE_QUAD_EDGE_COUNT;
                edge_vertex_indices = (EdgeVertexIndices*)CUBE_QUAD_EDGES;
                break;
        }

        aabb = {-1 , +1};
        triangles = nullptr;
    }
};
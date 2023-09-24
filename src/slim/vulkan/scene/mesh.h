#pragma once

#include "../../scene/mesh.h"
#include "../../serialization/mesh.h"

template <typename Vertex>
void loadVertices(Mesh &mesh, Vertex *vertices) {
    for (u32 triangle_index = 0; triangle_index < mesh.triangle_count; triangle_index++) {
        for (u32 vertex_num = 0; vertex_num < 3; vertex_num++, vertices++) {
            vertices->position = mesh.vertex_positions[mesh.vertex_position_indices[triangle_index].ids[vertex_num]];
            if (mesh.uvs_count) vertices->uv = mesh.vertex_uvs[mesh.vertex_uvs_indices[triangle_index].ids[vertex_num]];
            if (mesh.normals_count) vertices->normal = mesh.vertex_normals[mesh.vertex_normal_indices[triangle_index].ids[vertex_num]];
        }
    }
}

struct MeshGroup {
    VertexBuffer vertex_buffer;
    u32 mesh_count = 0;
    u32 total_triangle_count = 0;
    u32 *mesh_triangle_counts = nullptr;

    template <typename Vertex>
    void load(String *mesh_files, u32 count) {
        mesh_count = count;

        if (mesh_triangle_counts) delete mesh_triangle_counts;
        mesh_triangle_counts = new u32[mesh_count];

        total_triangle_count = 0;

        u32 max_triangle_count = 0;
        u32 max_position_count = 0;
        u32 max_normal_count = 0;
        u32 max_uv_count = 0;

        Mesh mesh;
        void *file;
        for (u32 m = 0; m < mesh_count; m++) {
            file = os::openFileForReading(mesh_files[m].char_ptr);
            readHeader(mesh, file);
            os::closeFile(file);

            total_triangle_count += mesh.triangle_count;

            if (mesh.triangle_count > max_triangle_count) max_triangle_count = mesh.triangle_count;
            if (mesh.vertex_count > max_position_count) max_position_count = mesh.vertex_count;
            if (mesh.normals_count > max_normal_count) max_normal_count = mesh.normals_count;
            if (mesh.uvs_count > max_uv_count) max_uv_count = mesh.uvs_count;
            mesh_triangle_counts[m] = mesh.triangle_count;
        }

        // Allocate temporary loading-memory with upper-bounded sizes:
        mesh.triangles = new Triangle[max_triangle_count];
        mesh.vertex_positions = new vec3[max_position_count];
        mesh.vertex_normals = new vec3[max_normal_count];
        mesh.vertex_uvs = new vec2[max_uv_count];
        mesh.vertex_position_indices = new TriangleVertexIndices[max_triangle_count];
        mesh.vertex_normal_indices = new TriangleVertexIndices[max_triangle_count];
        mesh.vertex_uvs_indices = new TriangleVertexIndices[max_triangle_count];
        mesh.edge_vertex_indices = new EdgeVertexIndices[max_triangle_count * 3];
        mesh.bvh.nodes = new BVHNode[max_triangle_count * 2];
        auto *vertices = new TriangleVertex[total_triangle_count * 3];

        for (u32 m = 0; m < mesh_count; m++) {
            file = os::openFileForReading(mesh_files[m].char_ptr);
            readHeader(mesh, file);
            readContent(mesh, file);
            os::closeFile(file);

            loadVertices(mesh, vertices);
            vertices += mesh.triangle_count * 3;
        }
        vertices -= total_triangle_count * 3;

        vertex_buffer.create(total_triangle_count * 3, sizeof(TriangleVertex));
        vertex_buffer.upload(vertices);

        // Cleanup temporary loading-memory:
        delete[] mesh.triangles;
        delete[] mesh.vertex_positions;
        delete[] mesh.vertex_normals;
        delete[] mesh.vertex_uvs;
        delete[] mesh.vertex_position_indices;
        delete[] mesh.vertex_normal_indices;
        delete[] mesh.vertex_uvs_indices;
        delete[] mesh.edge_vertex_indices;
        delete[] mesh.bvh.nodes;
        delete[] vertices;
    }
};
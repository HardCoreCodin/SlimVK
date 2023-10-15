#pragma once

#include "../../scene/mesh.h"
#include "../../serialization/mesh.h"

#include "../core/graphics.h"

namespace gpu {

    struct TriangleVertex {
        vec3 position{};
        vec3 normal;
        vec2 uv;
    };
    VertexDescriptor vertex_descriptor{
        sizeof(TriangleVertex),
        3, {
            _vec3,
            _vec3,
            _vec2
        }
    };

    template <typename Vertex = TriangleVertex>
    void loadVertices(Mesh &mesh, Vertex *vertices, bool flip_winding_order = false) {
        for (u32 triangle_index = 0; triangle_index < mesh.triangle_count; triangle_index++) {
            for (u32 vertex_num = 0; vertex_num < 3; vertex_num++, vertices++) {
                u32 v = vertex_num == 0 || !flip_winding_order ? vertex_num : (vertex_num == 2 ? 1 : 2);
                vertices->position = mesh.vertex_positions[mesh.vertex_position_indices[triangle_index].ids[v]];
                if (mesh.uvs_count) vertices->uv = mesh.vertex_uvs[mesh.vertex_uvs_indices[triangle_index].ids[v]];
                if (mesh.normals_count) vertices->normal = mesh.vertex_normals[mesh.vertex_normal_indices[triangle_index].ids[v]];
            }
        }
    }

    struct GPUMesh {
        VertexBuffer vertex_buffer;
        VertexBuffer edge_buffer;

        template <typename Vertex = TriangleVertex>
        void create(Mesh &mesh, u32 vertex_count = 0, u32 edge_vertex_count = 0) {
            if (!vertex_count) vertex_count = mesh.triangle_count * 3;
            if (!edge_vertex_count) edge_vertex_count = mesh.edge_count * 2;

            auto *vertices = new Vertex[mesh.triangle_count * 3];
            loadVertices<Vertex>(mesh, vertices);
            vertex_buffer.create(vertex_count, sizeof(Vertex));
            vertex_buffer.upload(vertices);
            delete[] vertices;

            auto *edges = new Edge[mesh.edge_count];
            mesh.loadEdges(edges);
            edge_buffer.create(edge_vertex_count, sizeof(vec3));
            edge_buffer.upload(edges);
            delete[] edges;
        }

        void destroy() {
            vertex_buffer.destroy();
            edge_buffer.destroy();
        }
    };

    struct GPUMeshGroup {
        VertexBuffer vertex_buffer;
        VertexBuffer edge_buffer;
        u32 mesh_count = 0;
        u32 total_triangle_count = 0;
        u32 *mesh_triangle_counts = nullptr;

        template <typename Vertex = TriangleVertex>
        void create(String *mesh_files, u32 count) {
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
            auto *vertices = new Vertex[total_triangle_count * 3];
            auto *edges = new Edge[total_triangle_count * 3];

            for (u32 m = 0; m < mesh_count; m++) {
                file = os::openFileForReading(mesh_files[m].char_ptr);
                readHeader(mesh, file);
                readContent(mesh, file);
                os::closeFile(file);

                loadVertices(mesh, vertices);
                mesh.loadEdges(edges);
                vertices += mesh.triangle_count * 3;
                edges += mesh.triangle_count * 3;
            }
            vertices -= total_triangle_count * 3;
            edges -= total_triangle_count * 3;

            vertex_buffer.create(total_triangle_count * 3, sizeof(Vertex));
            vertex_buffer.upload(vertices);

            edge_buffer.create(total_triangle_count * 3 * 2, sizeof(vec3));
            edge_buffer.upload(edges);

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
            delete[] edges;
        }

        void destroy() {
            vertex_buffer.destroy();
            edge_buffer.destroy();
        }
    };
}


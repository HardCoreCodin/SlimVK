#include "./shaders.h"
#include "../slim/core/transform.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/scene/selection.h"

//#include "./textures.h"

#include "../slim/app.h"

#include "./images.h"

using namespace gpu;

struct ExampleVulkanApp : SlimApp {
    Camera camera{{}, {0, 0, -3.5}}, *cameras{&camera};
    Navigation navigation;
    Frustum frustum;
    // Scene:
    Light rim_light{ {1.0f, 0.25f, 0.25f}, 0.9f * 40.0f, {6, 5, 2}};
    Light glass_light1{ {0.25f, 1.0f, 0.25f}, 20.0f, {-3.3f, 0.6f, -3.0f}};
    Light glass_light2{ {0.25f, 0.25f, 1.0f}, 20.0f, {-0.6f, 1.75f, -3.15f}};
    Light *lights{&rim_light};

    enum MaterialID { Floor, DogMaterial, Rough, Phong, Blinn, Mirror, Glass, MaterialCount };

    u8 flags{MATERIAL_HAS_NORMAL_MAP | MATERIAL_HAS_ALBEDO_MAP};
    Material floor_material{0.8f, 0.2f, flags,
                            2, {Floor_Albedo, Floor_Normal}};
    Material dog_material{1.0f, 0.2f, flags,
                          2, {Dog_Albedo, Dog_Normal}};
    Material rough_material{0.8f, 0.7f};
    Material phong_material{1.0f,0.5f,0,0, {},
                            BRDF_Phong, 1.0f, 1.0, 0.0f, IOR_AIR,
                            {1.0f, 1.0f, 0.4f}};
    Material blinn_material{{1.0f, 0.3f, 1.0f},0.5f,0,0, {},
                            BRDF_Blinn, 1.0f, 1.0, 0.0f, IOR_AIR,
                            {1.0f, 0.4f, 1.0f}};
    Material mirror_material{
        0.0f,0.02f,
        MATERIAL_IS_REFLECTIVE,
        0, {},
        BRDF_CookTorrance, 1.0f, 1.0,  0.0f,
        IOR_AIR, F0_Aluminium
    };
    Material glass_material {
        0.05f,0.25f,
        MATERIAL_IS_REFRACTIVE,
        0,{},
        BRDF_CookTorrance, 1.0f, 1.0f, 0.0f,
        IOR_GLASS, F0_Glass_Low
    };
    Material *materials{&floor_material};


    enum MesheID { Monkey, Dragon, Dog, MeshCount };

    Mesh meshes[MeshCount];
    char strings[MeshCount][100]{};
    String mesh_files[MeshCount] = {
        String::getFilePath("monkey.mesh",strings[Monkey],__FILE__),
        String::getFilePath("dragon.mesh",strings[Dragon],__FILE__),
        String::getFilePath("dog.mesh"   ,strings[Dog   ],__FILE__)
    };

    OrientationUsingQuaternion rot{0, -45 * DEG_TO_RAD, 0};
    Geometry monkey{{rot,{6, 4.5, 2}, 0.4f},
                    GeometryType_Mesh, Glass,      Monkey};
    Geometry dragon{{{},{-2, 2, -3}},
                    GeometryType_Mesh, Glass,      Dragon};
    Geometry dog   {{rot,{4, 2.1f, 3}, 0.8f},
                    GeometryType_Mesh, DogMaterial,   Dog};
    Geometry *geometries{&monkey};

    Scene scene{{3,1,3,
                 MaterialCount,0, MeshCount},
                geometries, cameras, lights, materials, nullptr, nullptr, meshes, mesh_files};


    VertexBuffer mesh_vertex_buffers[MeshCount];
//    IndexBuffer mesh_index_buffers[MeshCount];

    DescriptorPool descriptor_pool;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorSets descriptor_sets;
    PushConstantsLayout push_constants_layout;
    PipelineLayout graphics_pipeline_layout;
    GraphicsPipeline graphics_pipeline;

    struct PushConstants {
        alignas(16) mat4 model;
    };
    PushConstants model_push_constants;

    struct CameraUniform {
        alignas(16) mat4 view;
        alignas(16) mat4 proj;
    };
    CameraUniform camera_uniform_data;
    UniformBuffer camera_uniform_buffer[VULKAN_MAX_FRAMES_IN_FLIGHT];

    GPUImage floor_albedo;

    void OnInit() override {
        floor_albedo.createTextureFromPixels(floor_albedo_image.content,
                                             floor_albedo_image.width,
                                             floor_albedo_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[0]);

        push_constants_layout.addForVertexAndFragment(sizeof(PushConstants));

        for (u32 m = 0; m < MeshCount; m++) {
            Mesh &mesh = meshes[m];
            const u32 vertex_count = mesh.triangle_count * 3;
            mesh_vertex_buffers[m].create(vertex_count, sizeof(TriangleVertex));
//            mesh_index_buffers[m].create(vertex_count, sizeof(u32));

            TriangleVertex *mesh_vertices = new TriangleVertex[vertex_count];
//            u32 *mesh_indices = new u32[vertex_count];
            u32 index = 0;
            for (u32 f = 0; f < mesh.triangle_count; f++) {
                for (u32 v = 0; v < 3; v++, index++) {
//                    mesh_indices[index] = index;
                    TriangleVertex &vertex = mesh_vertices[index];
                    vertex.position = mesh.vertex_positions[mesh.vertex_position_indices[f].ids[v]];
                    if (mesh.uvs_count) vertex.uv = mesh.vertex_uvs[mesh.vertex_uvs_indices[f].ids[v]];
                    if (mesh.normals_count) vertex.normal = mesh.vertex_normals[mesh.vertex_normal_indices[f].ids[v]];
                }
            }
//
//            for (size_t v = 0; v < mesh.vertex_count; v++)
//                mesh_vertices[v].position = mesh.vertex_positions[v];
//
//            if (mesh.uvs_count) {
//                if (mesh.uvs_count != mesh.vertex_count)
//                    for (size_t f = 0; f < mesh.triangle_count; f++)
//                        for (size_t v = 0; v < 3; v++)
//                            mesh_vertices[mesh.vertex_position_indices[f].ids[v]].uv = mesh.vertex_uvs[mesh.vertex_uvs_indices[f].ids[v]];
//                else
//                    for (size_t v = 0; v < mesh.vertex_count; v++)
//                        mesh_vertices[v].uv = mesh.vertex_uvs[v];
//            }
//            if (mesh.normals_count) {
//                if (mesh.normals_count != mesh.vertex_count)
//                    for (size_t f = 0; f < mesh.triangle_count; f++)
//                        for (size_t v = 0; v < 3; v++)
//                            mesh_vertices[mesh.vertex_position_indices[f].ids[v]].normal = mesh.vertex_normals[mesh.vertex_normal_indices[f].ids[v]];
//                else
//                    for (size_t v = 0; v < mesh.vertex_count; v++)
//                        mesh_vertices[v].normal = mesh.vertex_normals[v];
//            }
//            for (size_t f = 0, i = 0; f < mesh.triangle_count; f++)
//                for (size_t v = 0; v < 3; v++)
//                    mesh_indices[i++] = mesh.vertex_position_indices[f].ids[v];

            mesh_vertex_buffers[m].upload(mesh_vertices);
//            mesh_index_buffers[m].upload(mesh_indices);
            delete[] mesh_vertices;
//            delete[] mesh_indices;
        }

        descriptor_set_layout.addForVertexUniformBuffer(0);
        descriptor_set_layout.addForFragmentTexture(1);
        descriptor_set_layout.create();

        graphics_pipeline_layout.create(&descriptor_set_layout, &push_constants_layout);

        descriptor_pool.addForUniformBuffers(1);
        descriptor_pool.addForCombinedImageSamplers(1);
        descriptor_pool.create(VULKAN_MAX_FRAMES_IN_FLIGHT);

        descriptor_sets.count = VULKAN_MAX_FRAMES_IN_FLIGHT;
        descriptor_pool.allocate(descriptor_set_layout, descriptor_sets);

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffer[i].create(sizeof(CameraUniform));
            camera_uniform_buffer[i].writeDescriptor(descriptor_sets.handles[i], 0);
            floor_albedo.writeSamplerDescriptor(descriptor_sets.handles[i], 1);
        }

        graphics_pipeline.createFromSourceStrings(
            present::render_pass,
            graphics_pipeline_layout,
            vertex_descriptor,
            vertex_shader_source_string,
            fragment_shader_source_string);
    }

    void OnUpdate(f32 delta_time) override {
        if (!controls::is_pressed::alt) navigation.update(camera, delta_time);
        frustum.updateProjection(camera.focal_length,(
                (f32)present::swapchain_rect.extent.height /
                (f32)present::swapchain_rect.extent.width));

        camera_uniform_data.view = Mat4(camera.orientation, camera.position).inverted();
        camera_uniform_data.proj = mat4{
            frustum.projection.scale.x, 0, 0, 0,
             0, -frustum.projection.scale.y, 0, 0,
             0, 0, frustum.projection.scale.z, 1,
             0, 0, frustum.projection.shear, 0
        };
        camera_uniform_buffer[present::current_frame].upload(&camera_uniform_data);
    }

    void OnRenderMainPass(GraphicsCommandBuffer &command_buffer) override {
        graphics_pipeline.bind(command_buffer);
        graphics_pipeline_layout.bind(descriptor_sets.handles[present::current_frame], command_buffer);

        for (size_t m = 0; m < MeshCount; m++) {
            model_push_constants.model = Mat4(
                geometries[m].transform.orientation,
                geometries[m].transform.scale,
                geometries[m].transform.position);
            graphics_pipeline_layout.pushConstants(command_buffer, push_constants_layout.ranges[0], &model_push_constants);

            mesh_vertex_buffers[m].bind(command_buffer);
            mesh_vertex_buffers[m].draw(command_buffer);
//            mesh_index_buffers[m].bind(command_buffer);
//            mesh_index_buffers[m].draw(command_buffer);
        }
    }

    void OnMouseButtonDown(mouse::Button &mouse_button) override {
        mouse::pos_raw_diff_x = mouse::pos_raw_diff_y = 0;
    }
    void OnMouseButtonDoubleClicked(mouse::Button &mouse_button) override {
        if (&mouse_button == &mouse::left_button) {
            mouse::is_captured = !mouse::is_captured;
            os::setCursorVisibility(!mouse::is_captured);
            os::setWindowCapture(    mouse::is_captured);
            OnMouseButtonDown(mouse_button);
        }
    }
    void OnKeyChanged(u8 key, bool is_pressed) override {
        Move &move = navigation.move;
        Turn &turn = navigation.turn;
        if (key == 'Q') turn.left     = is_pressed;
        if (key == 'E') turn.right    = is_pressed;
        if (key == 'R') move.up       = is_pressed;
        if (key == 'F') move.down     = is_pressed;
        if (key == 'W') move.forward  = is_pressed;
        if (key == 'S') move.backward = is_pressed;
        if (key == 'A') move.left     = is_pressed;
        if (key == 'D') move.right    = is_pressed;
    }

    void OnShutdown() override {
        floor_albedo.destroy();
        graphics_pipeline.destroy();
        graphics_pipeline_layout.destroy();

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
            camera_uniform_buffer[i].destroy();

        for (size_t m = 0; m < MeshCount; m++) {
            mesh_vertex_buffers[m].destroy();
//            mesh_index_buffers[m].destroy();
        }

        descriptor_set_layout.destroy();
        descriptor_pool.destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}
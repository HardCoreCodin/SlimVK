#include "./shaders.h"
#include "../slim/core/transform.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/scene/selection.h"
#include "../slim/vulkan/scene/mesh.h"
//#include "./textures.h"

#include "../slim/app.h"

#include "./images.h"

using namespace gpu;

struct ExampleVulkanApp : SlimApp {
    Camera camera{{}, {0, 0, -3.5}}, *cameras{&camera};
    Canvas canvas;
    Viewport viewport{canvas, &camera};
    CameraRayProjection projection;

    // Scene:
    Light rim_light{ {1.0f, 0.25f, 0.25f}, 0.9f * 8.0f, {6, 5, 2}};
    Light glass_light1{ {0.25f, 1.0f, 0.25f}, 4.0f, {-3.3f, 0.6f, -3.0f}};
    Light glass_light2{ {0.25f, 0.25f, 1.0f}, 4.0f, {-0.6f, 1.75f, -3.15f}};
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
    char mesh_file_string_buffers[MeshCount][100]{};
    String mesh_files[MeshCount] = {
        String::getFilePath("monkey.mesh",mesh_file_string_buffers[Monkey],__FILE__),
        String::getFilePath("dragon.mesh",mesh_file_string_buffers[Dragon],__FILE__),
        String::getFilePath("dog.mesh"   ,mesh_file_string_buffers[Dog   ],__FILE__)
    };
    MeshGroup mesh_group;

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

    SceneTracer scene_tracer{scene.counts.geometries, scene.mesh_stack_size};
    Selection selection{scene, scene_tracer, projection};


    DescriptorPool descriptor_pool;
    DescriptorSetLayout descriptor_set_layout;
    DescriptorSets descriptor_sets;
    PushConstantsLayout push_constants_layout;
    PipelineLayout graphics_pipeline_layout;
    GraphicsPipeline graphics_pipeline;

    struct Model {
        alignas(16) mat4 object_to_world;
        alignas(16) mat4 object_to_world_inverse_transposed;
    };
    Model model;

    struct CameraUniform {
        alignas(16) mat4 view;
        alignas(16) mat4 proj;
    };
    struct LightsUniform {
        alignas(16) vec3 camera_position;
        alignas(16) Light key_light;
        alignas(16) Light fill_light;
        alignas(16) Light rim_light;
    };
    LightsUniform lights_uniform_data;
    CameraUniform camera_uniform_data;
    UniformBuffer camera_uniform_buffer[VULKAN_MAX_FRAMES_IN_FLIGHT];
    UniformBuffer lights_uniform_buffer[VULKAN_MAX_FRAMES_IN_FLIGHT];

    GPUImage floor_albedo;

    enum struct VertexShaderBinding {
        View,
        Lights,
        Texture
    };

    void OnInit() override {
        floor_albedo.createTextureFromPixels(floor_albedo_image.content,
                                             floor_albedo_image.width,
                                             floor_albedo_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[0]);

        push_constants_layout.addForVertexAndFragment(sizeof(Model));

        mesh_group.load<TriangleVertex>(mesh_files, MeshCount);

        descriptor_set_layout.addForVertexUniformBuffer((u32)VertexShaderBinding::View);
        descriptor_set_layout.addForFragmentUniformBuffer((u32)VertexShaderBinding::Lights);
        descriptor_set_layout.addForFragmentTexture((u32)VertexShaderBinding::Texture);
        descriptor_set_layout.create();

        graphics_pipeline_layout.create(&descriptor_set_layout, &push_constants_layout);

        descriptor_pool.addForUniformBuffers(2);
        descriptor_pool.addForCombinedImageSamplers(1);
        descriptor_pool.create(VULKAN_MAX_FRAMES_IN_FLIGHT);

        descriptor_sets.count = VULKAN_MAX_FRAMES_IN_FLIGHT;
        descriptor_pool.allocate(descriptor_set_layout, descriptor_sets);

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            auto descriptor_set = descriptor_sets.handles[i];
            camera_uniform_buffer[i].create(sizeof(CameraUniform));
            camera_uniform_buffer[i].writeDescriptor(descriptor_set, (u32)VertexShaderBinding::View);

            lights_uniform_buffer[i].create(sizeof(LightsUniform));
            lights_uniform_buffer[i].writeDescriptor(descriptor_set, (u32)VertexShaderBinding::Lights);

            floor_albedo.writeSamplerDescriptor(descriptor_set, (u32)VertexShaderBinding::Texture);
        }

        graphics_pipeline.createFromSourceStrings(
            present::render_pass,
            graphics_pipeline_layout,
            vertex_descriptor,
            vertex_shader_source_string,
            fragment_shader_source_string);
    }
    void OnWindowResize(u16 width, u16 height) override {
        viewport.updateDimensions(width, height);
    }
    void OnUpdate(f32 delta_time) override {
        if (!mouse::is_captured) selection.manipulate(viewport);
        if (!controls::is_pressed::alt) viewport.updateNavigation(delta_time);

        projection.reset(camera, viewport.dimensions, false);

        lights_uniform_data.key_light = glass_light1;
        lights_uniform_data.fill_light = glass_light1;
        lights_uniform_data.rim_light = rim_light;
        lights_uniform_data.camera_position = camera.position;

        camera_uniform_data.view = Mat4(camera.orientation, camera.position).inverted();
        camera_uniform_data.proj = mat4{
            viewport.frustum.projection.scale.x, 0, 0, 0,
             0, -viewport.frustum.projection.scale.y, 0, 0,
             0, 0, viewport.frustum.projection.scale.z, 1,
             0, 0, viewport.frustum.projection.shear, 0
        };
        camera_uniform_buffer[present::current_frame].upload(&camera_uniform_data);
        lights_uniform_buffer[present::current_frame].upload(&lights_uniform_data);
    }

    void OnRenderMainPass(GraphicsCommandBuffer &command_buffer) override {
        graphics_pipeline.bind(command_buffer);
        graphics_pipeline_layout.bind(descriptor_sets.handles[present::current_frame], command_buffer);
        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 m = 0, first_index = 0; m < MeshCount; m++) {
            Transform &xf = geometries[m].transform;
            model.object_to_world = Mat4(xf.orientation, xf.scale, xf.position);
            model.object_to_world_inverse_transposed = model.object_to_world.inverted().transposed();
            graphics_pipeline_layout.pushConstants(command_buffer, push_constants_layout.ranges[0], &model);

            u32 vertex_count = mesh_group.mesh_triangle_counts[m] * 3;
            mesh_group.vertex_buffer.draw(command_buffer, vertex_count, (i32)first_index);
            first_index += vertex_count;
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
        Move &move = viewport.navigation.move;
        Turn &turn = viewport.navigation.turn;
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

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffer[i].destroy();
            lights_uniform_buffer[i].destroy();
        }
        mesh_group.vertex_buffer.destroy();
        descriptor_set_layout.destroy();
        descriptor_pool.destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}
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
    Light rim_light{ {1.0f, 0.5f, 0.5f}, 0.9f * 80.0f, {6, 5, 2}};
    Light glass_light1{ {0.5f, 1.0f, 0.5f}, 40.0f, {-3.3f, 0.6f, -3.0f}};
    Light glass_light2{ {0.5f, 0.5f, 1.0f}, 40.0f, {-0.6f, 1.75f, -3.15f}};
    Light *lights{&rim_light};

    enum MaterialID { FloorMaterial, DogMaterial, Rough, Phong, Blinn, Mirror, Glass, MaterialCount };

    u8 flags{MATERIAL_HAS_NORMAL_MAP | MATERIAL_HAS_ALBEDO_MAP};
    Material floor_material{0.7f, 0.9f, flags,
                            2, {Floor_Albedo, Floor_Normal}};
    Material dog_material{1.0f, 0.6f, flags,
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

    enum MesheID { Floor, Dog, Dragon, MeshCount };

    Mesh meshes[MeshCount];
    char mesh_file_string_buffers[MeshCount][100]{};
    String mesh_files[MeshCount] = {
        String::getFilePath("cube.mesh",mesh_file_string_buffers[Floor],__FILE__),
        String::getFilePath("dog.mesh"   ,mesh_file_string_buffers[Dog   ],__FILE__),
        String::getFilePath("dragon.mesh",mesh_file_string_buffers[Dragon],__FILE__)
    };
    MeshGroup mesh_group;

    OrientationUsingQuaternion rot{0, -45 * DEG_TO_RAD, 0};
    Geometry floor{{{},{}, {20.0f, 1.0f, 20.0f}},
                    GeometryType_Mesh, Glass,      Floor};
    Geometry dog   {{rot,{4, 2.1f, 3}, 0.8f},
                    GeometryType_Mesh, DogMaterial,   Dog};
    Geometry dragon{{{},{-2, 2, -3}},
                    GeometryType_Mesh, Glass,      Dragon};
    Geometry *geometries{&floor};

    Scene scene{{3,1,3,
                 MaterialCount,0, MeshCount},
                geometries, cameras, lights, materials, nullptr, nullptr, meshes, mesh_files};

    SceneTracer scene_tracer{scene.counts.geometries, scene.mesh_stack_size};
    Selection selection{scene, scene_tracer, projection};


    DescriptorPool lighting_descriptor_pool;
    DescriptorPool textures_descriptor_pool;
    DescriptorSetLayout lighting_descriptor_set_layout;
    DescriptorSetLayout textures_descriptor_set_layout;
    DescriptorSets lighting_descriptor_sets;
    DescriptorSets textures_descriptor_sets;
    PushConstantsLayout push_constants_layout;
    PipelineLayout graphics_pipeline_layout;
    GraphicsPipeline graphics_pipeline;

    struct MaterialParams {
        vec3 albedo;
        float roughness;
        vec3 F0;
        float metalness;
        float normal_strength;
        u32 use_textures;
    };

    struct Model {
        alignas(16) mat4 object_to_world;
        alignas(16) MaterialParams material_params;
    };
    Model model;

    MaterialParams floor_material_params = {
        floor_material.albedo,
        floor_material.roughness,
        floor_material.reflectivity,
        floor_material.metalness,
        1.0f,
        3
    };
    MaterialParams dog_material_params = {
        dog_material.albedo,
        dog_material.roughness,
        dog_material.reflectivity,
        dog_material.metalness,
        8.0f,
        3
    };
    MaterialParams rough_material_params = {
        rough_material.albedo,
        rough_material.roughness,
        rough_material.reflectivity,
        rough_material.metalness,
        1.0f,
        0
    };
    MaterialParams *material_params = &floor_material_params;

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
    GPUImage floor_normal;
    GPUImage dog_albedo;
    GPUImage dog_normal;

    char vertex_shader_source_file[100];
    char fragment_shader_source_file[100];

    enum struct VertexShaderBinding {
        View,
        Lights
    };

    enum struct FragmentShaderBinding {
        AlbedoTexture,
        NormalTexture
    };

    void OnInit() override {
        String::getFilePath("vertex_shader.glsl",vertex_shader_source_file,__FILE__);
        String::getFilePath("fragment_shader.glsl",fragment_shader_source_file,__FILE__);

        floor_albedo.createTextureFromPixels(floor_albedo_image.content,
                                             floor_albedo_image.width,
                                             floor_albedo_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[Floor_Albedo]);

        floor_normal.createTextureFromPixels(floor_normal_image.content,
                                             floor_normal_image.width,
                                             floor_normal_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[Floor_Normal]);

        dog_albedo.createTextureFromPixels(dog_albedo_image.content,
                                           dog_albedo_image.width,
                                           dog_albedo_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[Dog_Albedo]);

        dog_normal.createTextureFromPixels(dog_normal_image.content,
                                           dog_normal_image.width,
                                           dog_normal_image.height,
                                             transient_graphics_command_buffer,
                                             image_file_names[Dog_Normal]);

        push_constants_layout.addForVertexAndFragment(sizeof(Model));

        mesh_group.load<TriangleVertex>(mesh_files, MeshCount);

        lighting_descriptor_set_layout.addForVertexUniformBuffer((u32)VertexShaderBinding::View);
        lighting_descriptor_set_layout.addForFragmentUniformBuffer((u32)VertexShaderBinding::Lights);
        lighting_descriptor_set_layout.create();

        textures_descriptor_set_layout.addForFragmentTexture((u32)FragmentShaderBinding::AlbedoTexture);
        textures_descriptor_set_layout.addForFragmentTexture((u32)FragmentShaderBinding::NormalTexture);
        textures_descriptor_set_layout.create();

        graphics_pipeline_layout.create(&lighting_descriptor_set_layout, &push_constants_layout, 2);

        lighting_descriptor_sets.count = VULKAN_MAX_FRAMES_IN_FLIGHT;
        lighting_descriptor_pool.addForUniformBuffers(2 * VULKAN_MAX_FRAMES_IN_FLIGHT);
        lighting_descriptor_pool.create(lighting_descriptor_sets.count);
        lighting_descriptor_pool.allocate(lighting_descriptor_set_layout, lighting_descriptor_sets);

        textures_descriptor_sets.count = 2;
        textures_descriptor_pool.addForCombinedImageSamplers(textures_descriptor_sets.count * 2);
        textures_descriptor_pool.create(textures_descriptor_sets.count);
        textures_descriptor_pool.allocate(textures_descriptor_set_layout, textures_descriptor_sets);
        floor_albedo.writeSamplerDescriptor(textures_descriptor_sets.handles[0], (u32)FragmentShaderBinding::AlbedoTexture);
        floor_normal.writeSamplerDescriptor(textures_descriptor_sets.handles[0], (u32)FragmentShaderBinding::NormalTexture);
        dog_albedo.writeSamplerDescriptor(textures_descriptor_sets.handles[1], (u32)FragmentShaderBinding::AlbedoTexture);
        dog_normal.writeSamplerDescriptor(textures_descriptor_sets.handles[1], (u32)FragmentShaderBinding::NormalTexture);

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            auto descriptor_set = lighting_descriptor_sets.handles[i];
            camera_uniform_buffer[i].create(sizeof(CameraUniform));
            camera_uniform_buffer[i].writeDescriptor(descriptor_set, (u32)VertexShaderBinding::View);

            lights_uniform_buffer[i].create(sizeof(LightsUniform));
            lights_uniform_buffer[i].writeDescriptor(descriptor_set, (u32)VertexShaderBinding::Lights);
        }
        graphics_pipeline.createFromSourceFiles(
            present::render_pass,
            graphics_pipeline_layout,
            vertex_descriptor,
            vertex_shader_source_file,
            fragment_shader_source_file);
    }
    void OnWindowResize(u16 width, u16 height) override {
        viewport.updateDimensions(width, height);
    }
    void OnUpdate(f32 delta_time) override {
        scene.updateAABBs();
        scene.updateBVH();
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
        graphics_pipeline_layout.bind(lighting_descriptor_sets.handles[present::current_frame], command_buffer);
        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            Transform &xf = geometries[g].transform;
            model.object_to_world = Mat4(xf.orientation, xf.scale, xf.position);
            model.material_params = material_params[g];
            graphics_pipeline_layout.pushConstants(command_buffer, push_constants_layout.ranges[0], &model);
            if (g != Dragon) graphics_pipeline_layout.bind(textures_descriptor_sets.handles[g], command_buffer, 1);

            for (u32 m = 0, first_index = 0; m < MeshCount; m++) {
                u32 vertex_count = mesh_group.mesh_triangle_counts[m] * 3;
                if (m == geometries[g].id) {
                    mesh_group.vertex_buffer.draw(command_buffer, vertex_count, (i32)first_index);
                    break;
                } else first_index += vertex_count;
            }
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
        lighting_descriptor_set_layout.destroy();
        textures_descriptor_set_layout.destroy();
        lighting_descriptor_pool.destroy();
        textures_descriptor_pool.destroy();

        floor_albedo.destroy();
        floor_normal.destroy();
        dog_albedo.destroy();
        dog_normal.destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}
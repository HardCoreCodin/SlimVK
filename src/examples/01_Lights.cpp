//#include "./shaders.h"
#include "../slim/core/transform.h"
#include "../slim/math/utils.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/scene/selection.h"
#include "../slim/vulkan/scene/mesh.h"
//#include "../slim/vulkan/raster/pipeline.h"
//#include "./textures.h"

#include "../slim/app.h"

#include "./images.h"

using namespace gpu;

namespace line_rendering {
    const char* vertex_shader_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 out_position;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec3 color;
} push_constant;

void main() {
    gl_Position = push_constant.mvp * vec4(in_position, 1.0);
    gl_Position.w += 0.00001f;
})VERTEX_SHADER";

    const char* fragment_shader_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec3 color;
} push_constant;

void main() {
    out_color = vec4(push_constant.color, 1.0f);
})FRAGMENT_SHADER";

    struct PushConstant {
        alignas(16) mat4 mvp;
        alignas(16) vec3 color;
    };
    PushConstant push_constant;
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertexAndFragment(sizeof(PushConstant))}};

    VertexDescriptor vertex_descriptor{sizeof(vec3),1, { _vec3 }};
    VertexShader vertex_shader;
    FragmentShader fragment_shader;

    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};

    bool create(RenderPass *render_pass = &present::render_pass) {
        if (!vertex_shader.handle &&
            !vertex_shader.createFromSourceString(vertex_shader_string, &vertex_descriptor, "line_vertex_shader"))
                return false;

        if (!fragment_shader.handle &&
            !fragment_shader.createFromSourceString(fragment_shader_string, "line_fragment_shader"))
                return false;

        if (!pipeline_layout.handle &&
            !pipeline_layout.create(nullptr, 0, &push_constant_spec))
                return false;

        if (!pipeline.handle &&
            !pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, fragment_shader, true))
                return false;

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
    }
}

namespace default_material {
    const char* vertex_shader_file_name = "vertex_shader.glsl";
    const char* fragment_shader_file_name = "fragment_shader.glsl";

    struct MaterialParams {
        vec3 albedo;
        float roughness;
        vec3 F0;
        float metalness;
        float normal_strength;
        u32 use_textures;
    };

    struct PushConstant {
        alignas(16) mat4 object_to_world;
        alignas(16) MaterialParams material_params;
    };
    PushConstant push_constant;
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertexAndFragment(sizeof(PushConstant))}};

    VertexShader vertex_shader;
    FragmentShader fragment_shader;

    enum struct VertexShaderBinding {
        View,
        Lights
    };

    enum struct FragmentShaderBinding {
        AlbedoTexture,
        NormalTexture
    };

    DescriptorSetLayoutSpec descriptor_set_layout_specs[]{
        DescriptorSetLayoutSpec({
                                    DescriptorSetLayoutBindingForVertexUniformBuffer(),
                                    DescriptorSetLayoutBindingForFragmentUniformBuffer()}),
        DescriptorSetLayoutSpec({
             DescriptorSetLayoutBindingForFragmentImageAndSampler(),
             DescriptorSetLayoutBindingForFragmentImageAndSampler()
         })
    };
    constexpr u8 descriptor_set_layout_count{sizeof(descriptor_set_layout_specs) / sizeof(DescriptorSetLayoutSpec)};
    DescriptorSetLayout descriptor_set_layouts[descriptor_set_layout_count]{};
    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};

    DescriptorPool textures_descriptor_pool;
    DescriptorSets textures_descriptor_sets;

    DescriptorPool lighting_descriptor_pool;
    DescriptorSets lighting_descriptor_sets;

    void createResources(u8 variant_count) {
        lighting_descriptor_sets.count = VULKAN_MAX_FRAMES_IN_FLIGHT;
        lighting_descriptor_pool.addForUniformBuffers(2 * VULKAN_MAX_FRAMES_IN_FLIGHT);
        lighting_descriptor_pool.create(lighting_descriptor_sets.count);
        lighting_descriptor_pool.allocate(descriptor_set_layouts[0], lighting_descriptor_sets);

        textures_descriptor_sets.count = variant_count;
        textures_descriptor_pool.addForCombinedImageSamplers(textures_descriptor_sets.count * 2);
        textures_descriptor_pool.create(textures_descriptor_sets.count);
        textures_descriptor_pool.allocate(descriptor_set_layouts[1], textures_descriptor_sets);
    }

    void writeTextureDescriptors(u8 descriptor_set_index, const GPUImage &albedo, const GPUImage &normal) {
        albedo.writeDescriptor(textures_descriptor_sets.handles[descriptor_set_index], (u32)FragmentShaderBinding::AlbedoTexture);
        normal.writeDescriptor(textures_descriptor_sets.handles[descriptor_set_index], (u32)FragmentShaderBinding::NormalTexture);
    }

    void writeUniformDescriptors(u8 descriptor_set_index, const UniformBuffer &camera_uniform_buffer, const UniformBuffer &lights_uniform_buffer) {
        camera_uniform_buffer.writeDescriptor(lighting_descriptor_sets.handles[descriptor_set_index], (u32)VertexShaderBinding::View);
        lights_uniform_buffer.writeDescriptor(lighting_descriptor_sets.handles[descriptor_set_index], (u32)VertexShaderBinding::Lights);
    }

    bool createPipeline(RenderPass *render_pass = &present::render_pass) {
        char string_buffer[100];

        if (!vertex_shader.handle &&
            !vertex_shader.createFromSourceFile(String::getFilePath(vertex_shader_file_name, string_buffer, __FILE__).char_ptr, &vertex_descriptor))
            return false;

        if (!fragment_shader.handle &&
            !fragment_shader.createFromSourceFile(String::getFilePath(fragment_shader_file_name, string_buffer, __FILE__).char_ptr))
            return false;

        for (u32 i = 0; i < descriptor_set_layout_count; i++)
            if (!descriptor_set_layouts[i].handle &&
                !descriptor_set_layouts[i].create(descriptor_set_layout_specs[i]))
                return false;

        if (!pipeline_layout.handle &&
            !pipeline_layout.create(descriptor_set_layouts, descriptor_set_layout_count, &push_constant_spec))
            return false;

        if (!pipeline.handle &&
            !pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, fragment_shader))
            return false;

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        for (auto & descriptor_set_layout : descriptor_set_layouts)
            descriptor_set_layout.destroy();

        lighting_descriptor_pool.destroy();
        textures_descriptor_pool.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
    }
}



namespace cube_mesh_data {
    Triangle triangles[CUBE_TRIANGLE_COUNT];
    BVHNode bvh_nodes[CUBE_TRIANGLE_COUNT * 2];

    void load(Mesh &mesh, BVHBuilder &bvh_builder) {
//        mesh.loadCube(CubeEdgesType::BBox, false);
        mesh.triangles = triangles;
        mesh.bvh.nodes = bvh_nodes;
        bvh_builder.buildMesh(mesh);
    }
}

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

    enum MesheID { Dog, Dragon, Floor, MeshCount };

    Mesh dog_mesh;
    Mesh dragon_mesh;
    Mesh floor_mesh{CubeEdgesType::BBox, true};
    Mesh *meshes = &dog_mesh;
    char mesh_file_string_buffers[MeshCount - 1][100]{};
    String mesh_files[MeshCount - 1] = {
        String::getFilePath("dog.mesh"   ,mesh_file_string_buffers[Dog   ],__FILE__),
        String::getFilePath("dragon.mesh",mesh_file_string_buffers[Dragon],__FILE__)
    };
    GPUMeshGroup mesh_group;

    OrientationUsingQuaternion rot{0, -45 * DEG_TO_RAD, 0};
    Geometry dog   {{rot,{4, 2.1f, 3}, 0.8f},
                    GeometryType_Mesh, DogMaterial,   Dog};
    Geometry dragon{{{},{-2, 2, -3}},
                    GeometryType_Mesh, Glass,      Dragon};
    Geometry floor{{{},{0, -3, 0}, {20.0f, 1.0f, 20.0f}},
                   GeometryType_Mesh, Glass,      Floor};
    Geometry *geometries{&dog};

    Scene scene{{3,1,3,
                 MaterialCount,0, MeshCount - 1},
                geometries, cameras, lights, materials, nullptr, nullptr, meshes, mesh_files};

    SceneTracer scene_tracer{scene.counts.geometries, scene.mesh_stack_size};
    Selection selection{scene, scene_tracer, projection};


    default_material::MaterialParams dog_material_params = {
        dog_material.albedo,
        dog_material.roughness,
        dog_material.reflectivity,
        dog_material.metalness,
        8.0f,
        3
    };
    default_material::MaterialParams rough_material_params = {
        rough_material.albedo,
        rough_material.roughness,
        rough_material.reflectivity,
        rough_material.metalness,
        1.0f,
        0
    };
    default_material::MaterialParams floor_material_params = {
        floor_material.albedo,
        floor_material.roughness,
        floor_material.reflectivity,
        floor_material.metalness,
        1.0f,
        3
    };
    default_material::MaterialParams *material_params = &dog_material_params;

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
    GPUMesh floor_gpu_mesh;

    void OnInit() override {
        cube_mesh_data::load(floor_mesh, *scene.bvh_builder);
        floor_gpu_mesh.create(floor_mesh);
        mesh_group.create(mesh_files, MeshCount - 1);

        scene.counts.meshes = MeshCount;
        scene.updateAABBs();
        scene.updateBVH(2);

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

        default_material::createPipeline();
        default_material::createResources(2);
        default_material::writeTextureDescriptors(0, dog_albedo, dog_normal);
        default_material::writeTextureDescriptors(1, floor_albedo, floor_normal);

        for (u8 i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffer[i].create(sizeof(CameraUniform));
            lights_uniform_buffer[i].create(sizeof(LightsUniform));
            default_material::writeUniformDescriptors(i, camera_uniform_buffer[i], lights_uniform_buffer[i]);
        }

        line_rendering::create();
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
        default_material::pipeline.bind(command_buffer);
        default_material::pipeline_layout.bind(default_material::lighting_descriptor_sets.handles[present::current_frame], command_buffer);
        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            Transform &xf = geometries[g].transform;
            default_material::push_constant.object_to_world = Mat4(xf.orientation, xf.scale, xf.position);
            default_material::push_constant.material_params = material_params[g];
            default_material::pipeline_layout.pushConstants(command_buffer, default_material::push_constant_spec.ranges[0], &default_material::push_constant);
            if (g == Floor) {
                default_material::pipeline_layout.bind(default_material::textures_descriptor_sets.handles[1], command_buffer, 1);
                floor_gpu_mesh.vertex_buffer.bind(command_buffer);
                floor_gpu_mesh.vertex_buffer.draw(command_buffer);
            } else {
                if (g == Dog) default_material::pipeline_layout.bind(default_material::textures_descriptor_sets.handles[0], command_buffer, 1);
                for (u32 m = 0, first_index = 0; m < (MeshCount - 1); m++) {
                    u32 vertex_count = mesh_group.mesh_triangle_counts[m] * 3;
                    if (m == geometries[g].id) {
                        mesh_group.vertex_buffer.draw(command_buffer, vertex_count, (i32)first_index);
                        break;
                    } else first_index += vertex_count;
                }
            }
        }

        mat4 vp = camera_uniform_data.view * camera_uniform_data.proj;
        line_rendering::pipeline.bind(command_buffer);
//        floor_mesh_edge_buffer.bind(command_buffer);
//        Transform &xf = geometries[Floor].transform;
//        line_model.mvp = Mat4(xf.orientation, xf.scale, xf.position) * vp;
//        line_model.line_color = 1.0f;
//        line_graphics_pipeline_layout.pushConstants(command_buffer, line_push_constants_layout.ranges[0], &line_model);
//        floor_mesh_edge_buffer.bind(command_buffer);
//        floor_mesh_edge_buffer.draw(command_buffer);

        mesh_group.edge_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            Transform &xf = geometries[g].transform;
            line_rendering::push_constant.mvp = Mat4(xf.orientation, xf.scale, xf.position) * vp;
            line_rendering::push_constant.color = 1.0f;
            line_rendering::pipeline_layout.pushConstants(command_buffer, line_rendering::push_constant_spec.ranges[0], &line_rendering::push_constant);
            if (g == Floor) {
                floor_gpu_mesh.edge_buffer.bind(command_buffer);
                floor_gpu_mesh.edge_buffer.draw(command_buffer);
            } else {
                for (u32 m = 0, first_index = 0; m < (MeshCount - 1); m++) {
                    u32 vertex_count = mesh_group.mesh_triangle_counts[m] * 3 * 2;
                    if (m == geometries[g].id) {
                        mesh_group.edge_buffer.draw(command_buffer, vertex_count, (i32)first_index);
                        break;
                    } else first_index += vertex_count;
                }
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
        default_material::destroy();
        line_rendering::destroy();

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffer[i].destroy();
            lights_uniform_buffer[i].destroy();
        }
        mesh_group.vertex_buffer.destroy();
        mesh_group.edge_buffer.destroy();
        floor_gpu_mesh.destroy();

        floor_albedo.destroy();
        floor_normal.destroy();
        dog_albedo.destroy();
        dog_normal.destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}
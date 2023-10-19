#include "../slim/core/transform.h"
#include "../slim/math/mat4_constructurs.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/scene/selection.h"
#include "../slim/vulkan/scene/mesh.h"
#include "../slim/vulkan/raster/pipeline.h"
#include "../slim/vulkan/raster/line_render.h"
#include "../slim/vulkan/raster/default_material.h"

#include "../slim/app.h"

#include "./images.h"

using namespace gpu;

char vertex_shader_file[100];
char fragment_shader_file[100];
String shader_files[]{
    String::getFilePath("vertex_shader.glsl", vertex_shader_file, __FILE__),
    String::getFilePath("fragment_shader.glsl", fragment_shader_file, __FILE__)
};

struct ExampleVulkanApp : SlimApp {
    Camera camera{{}, {0, 0, -3.5}}, *cameras{&camera};
    Canvas canvas;
    Viewport viewport{canvas, &camera};
    CameraRayProjection camera_ray_projection;

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
    Selection selection{scene, scene_tracer, camera_ray_projection};


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

    GPUImage floor_albedo;
    GPUImage floor_normal;
    GPUImage dog_albedo;
    GPUImage dog_normal;
    GPUMesh floor_gpu_mesh;

    void OnInit() override {
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

        raster_render_pipeline::create();
        default_material::create(vertex_shader_file, fragment_shader_file, &floor_albedo, 4);
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

        camera_ray_projection.reset(camera, viewport.dimensions, false);

        raster_render_pipeline::update(scene, viewport);
    }

    void OnRenderMainPass(GraphicsCommandBuffer &command_buffer) override {
        default_material::bind(command_buffer);
        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            default_material::setModel(command_buffer, Mat4(geometries[g].transform), material_params[g]);
            if (g == Floor) {
                default_material::bindTextures(command_buffer, 0);
                floor_gpu_mesh.vertex_buffer.bind(command_buffer);
                floor_gpu_mesh.vertex_buffer.draw(command_buffer);
            } else {
                if (g == Dog) default_material::bindTextures(command_buffer, 1);
                for (u32 m = 0, first_index = 0; m < (MeshCount - 1); m++) {
                    u32 vertex_count = mesh_group.mesh_triangle_counts[m] * 3;
                    if (m == geometries[g].id) {
                        mesh_group.vertex_buffer.draw(command_buffer, vertex_count, (i32)first_index);
                        break;
                    } else first_index += vertex_count;
                }
            }
        }

        line_rendering::pipeline.bind(command_buffer);
        mesh_group.edge_buffer.bind(command_buffer);
        mat4 view_projection = raster_render_pipeline::camera_uniform_data.view * raster_render_pipeline::camera_uniform_data.proj;
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            line_rendering::setModel(command_buffer, Mat4(geometries[g].transform) * view_projection);

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
        raster_render_pipeline::destroy();
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
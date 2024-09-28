#include "../slim/core/transform.h"
#include "../slim/math/mat4_constructurs.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/scene/selection.h"
#include "../slim/draw/selection.h"
#include "../slim/vulkan/raster/pipeline.h"
#include "../slim/vulkan/raster/skybox.h"
#include "../slim/vulkan/raster/debug_image.h"
#include "../slim/vulkan/raster/default_material.h"
#include "../slim/vulkan/scene/mesh.h"

#include "../slim/app.h"

#include "./images.h"

#define HAS_ALBEDO_MAP 1
#define HAS_NORMAL_MAP 2
#define DRAW_DEPTH 4
#define DRAW_POSITION 8
#define DRAW_NORMAL 16
#define DRAW_UVS 32
#define DRAW_ALBEDO 64
#define DRAW_IBL 128
#define DRAW_SHADOW_MAP 256
#define DRAW_SHADOW_MAP_PROJECTED 512

using namespace gpu;

struct ExampleVulkanApp : SlimApp {
    Camera camera{{-25 * DEG_TO_RAD, 0, 0}, {0, 7, -11}}, *cameras{&camera};
    CameraRayProjection camera_ray_projection;
    Canvas canvas;
    Viewport viewport{canvas, &camera};
    
    DirectionalLight directional_light{
        {-60*DEG_TO_RAD, 30*DEG_TO_RAD, 0}, {1.0f, 0.83f, 0.7f}, 1.7f, {0, 25, -20}
    };
    PointLight point_lights[2]{
        {{1.0f, 2.0f, 0.0f}, Blue, 200.0f},
        {{-4.0f, 3.0f, 0.0f}, Green, 200.0f}
    };
    SpotLight spot_lights[2]{
        {{DEG_TO_RAD*-90, DEG_TO_RAD*0, 0}, {3, 4, 5}, Magenta, 300.0f},
        {{DEG_TO_RAD*-90, DEG_TO_RAD*0, 0}, {-3, 4, -5}, Cyan, 300.0f}
    };

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
    Mesh triangle_mesh;
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
    Geometry dragon{{{},{-12, 2, -3}},
                    GeometryType_Mesh, Glass,      Dragon};
    Geometry floor{{{},{0, -3, 0}, {20.0f, 1.0f, 20.0f}},
                   GeometryType_Mesh, Glass,      Floor};
    Geometry *geometries{&dog};

    Scene scene{{3, 1, 1, 2, 2, MaterialCount, 0, MeshCount - 1},
        geometries, &camera, &directional_light, point_lights, spot_lights, materials, 
        nullptr, nullptr, meshes, mesh_files};
    SceneTracer scene_tracer{scene.counts.geometries, scene.mesh_stack_size};
    Selection selection{scene, scene_tracer, camera_ray_projection};

    u32 debug_flags = DRAW_SHADOW_MAP_PROJECTED;
    u32 debug_maps = 0;

    default_material::MaterialParams dog_material_params = {
        dog_material.albedo,
        dog_material.roughness,
        dog_material.reflectivity,
        dog_material.metalness,
        2.0f,
        HAS_ALBEDO_MAP | HAS_NORMAL_MAP
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
        HAS_ALBEDO_MAP | HAS_NORMAL_MAP
    };
    default_material::MaterialParams *material_params = &dog_material_params;
    
    GPUMesh floor_gpu_mesh;
    GPUMesh triangle_gpu_mesh;

    GPUImage *textures = nullptr;
    GPUImage *skybox_maps = nullptr;
    GPUImage *ibl_maps = nullptr;

    ShadowMap directional_shadhow_maps[VULKAN_MAX_FRAMES_IN_FLIGHT];

    u32 skybox_index = 0;
    f32 IBL_intensity = 1.0f;

    void initTextures(CubeMapSet *cube_map_sets, u32 cube_map_sets_count, RawImage *texture_images, u32 texture_count) {
	    if (texture_images && texture_count) {
		    textures = new GPUImage[texture_count];
		    for (u32 i = 0; i < texture_count; i++)
                textures[i].createTexture(texture_images[i]);
        }
			
	    if (cube_map_sets && cube_map_sets_count) {
		    skybox_maps = new GPUImage[cube_map_sets_count];
		    ibl_maps = new GPUImage[cube_map_sets_count * 2];
		    for (u32 i = 0; i < cube_map_sets_count; i++) {
                skybox_maps[i].createCubeMap(cube_map_sets[i].skybox);
                ibl_maps[i * 2 + 0].createCubeMap(cube_map_sets[i].radiance);
                ibl_maps[i * 2 + 1].createCubeMap(cube_map_sets[i].irradiance);
		    }
	    }  
    }

    void OnInit() override {
        triangle_mesh.loadTriangle();
        triangle_gpu_mesh.create(triangle_mesh);
        floor_gpu_mesh.create(floor_mesh);
        mesh_group.create(mesh_files, MeshCount - 1);

        scene.counts.meshes = MeshCount;
        scene.updateAABBs();
        scene.updateBVH(2);

        initTextures(cube_map_sets.array, CUBE_MAP_SETS_COUNT, images, ImageCount);
        for (int i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
            directional_shadhow_maps[i].create(2048, 2048, 1.0f, 1.0f);

        raster_render_pipeline::create();
        default_material::create(textures, 4, ibl_maps, CUBE_MAP_SETS_COUNT, directional_shadhow_maps);
        line_rendering::create();
        skybox_renderer::create(skybox_maps, CUBE_MAP_SETS_COUNT);
        debug_image::create(directional_shadhow_maps, VULKAN_MAX_FRAMES_IN_FLIGHT);
    }

    void OnWindowResize(u16 width, u16 height) override {
        viewport.updateDimensions(width, height);
    }

    void OnUpdate(f32 delta_time) override {
        scene.updateAABBs();
        scene.updateBVH();
        camera_ray_projection.reset(camera, viewport.dimensions, false);

        if (!mouse::is_captured) selection.manipulate(viewport);
        if (!controls::is_pressed::alt) viewport.updateNavigation(delta_time);

        raster_render_pipeline::update(scene, viewport, IBL_intensity);
    }
    
    virtual void OnRender() {
        GraphicsCommandBuffer &command_buffer{*gpu::graphics_command_buffer};

        directional_shadhow_maps[present::current_frame].beginWrite(command_buffer);
        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            shadow_pass::setModel(command_buffer, Mat4(geometries[g].transform) * scene.directional_lights[0].shadowMapMatrix());
            if (g == Floor) {
                floor_gpu_mesh.vertex_buffer.bind(command_buffer);
                floor_gpu_mesh.vertex_buffer.draw(command_buffer);
            } 
            else {
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
        directional_shadhow_maps[present::current_frame].endWrite(command_buffer);

        gpu::beginRenderPass();
      
        if (debug_flags == 0) {
            mat3 camera_ray_matrix = camera.orientation;
            camera_ray_matrix.Z *= camera.focal_length;
		    camera_ray_matrix.X /= viewport.dimensions.height_over_width;
            skybox_renderer::draw(command_buffer, Mat4(camera_ray_matrix), skybox_index);
        }

        default_material::bindDirectionalShadowMap(command_buffer);
        default_material::bindIBL(command_buffer, (u8)skybox_index);
        default_material::bind(command_buffer, debug_flags);
        if (debug_flags & DRAW_SHADOW_MAP) {
            raster_render_pipeline::update({}, {}, {}, 
            scene.point_lights,
            (u8)scene.counts.point_lights,
            scene.directional_lights[0]);
            default_material::setModel(command_buffer, {}, {}, debug_maps | debug_flags);
            default_material::bindTextures(command_buffer, 0);
            vkCmdDraw(command_buffer.handle, 3, 1, 0, 0);
            gpu::endRenderPass();
            return;
        }

        mesh_group.vertex_buffer.bind(command_buffer);
        for (u32 g = 0; g < scene.counts.geometries; g++) {
            default_material::setModel(command_buffer, geometries[g].transform,
                material_params[g],
                (material_params[g].flags & debug_maps) | debug_flags);
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


        mat4 view_projection_matrix = raster_render_pipeline::camera_uniform_data.view * raster_render_pipeline::camera_uniform_data.proj;
                
        for (u32 i = 0; i < scene.counts.point_lights; i++) 
            line_rendering::drawBox(command_buffer, scene.point_lights[i].transformationMatrix() * view_projection_matrix, BrightCyan, 0.5f);
		
        for (u32 i = 0; i < scene.counts.spot_lights; i++)  
            line_rendering::drawBox(command_buffer, scene.spot_lights[i].transformationMatrix() * view_projection_matrix, BrightBlue, 0.5f);
        line_rendering::drawBox(command_buffer, scene.directional_lights[0].transformationMatrix() * view_projection_matrix, BrightYellow, 0.5f);
		line_rendering::drawBox(command_buffer, scene.directional_lights[0].shadowBoundsMatrix() * view_projection_matrix, BrightYellow, 0.5f);
		
        if (controls::is_pressed::alt) drawSelection(selection, command_buffer, view_projection_matrix, scene.meshes);
      
        gpu::endRenderPass();
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
        if (!is_pressed) {
            if (key == '1') {debug_flags = 0;             debug_maps = 0; }
            if (key == '2') {debug_flags = DRAW_DEPTH;    debug_maps = 0; }
            if (key == '3') {debug_flags = DRAW_POSITION; debug_maps = 0; }
            if (key == '4') {debug_flags = DRAW_UVS;      debug_maps = 0; }
            if (key == '5') {debug_flags = DRAW_NORMAL;   debug_maps = 0; }
            if (key == '7') {debug_flags = DRAW_ALBEDO;   debug_maps = 0; }
            if (key == '6') {debug_flags = DRAW_NORMAL;   debug_maps = HAS_NORMAL_MAP; }
            if (key == '8') {debug_flags = DRAW_ALBEDO;   debug_maps = HAS_ALBEDO_MAP; }
            if (key == '9') {debug_flags = DRAW_SHADOW_MAP;debug_maps = 0; }
            if (key == '0') {debug_flags = DRAW_SHADOW_MAP_PROJECTED;debug_maps = 0; }
            if (key == 'B') skybox_index = (skybox_index + 1) % 2;
            if (key == 'Z') {dog_material_params.normal_strength -= 0.2f; if (dog_material_params.normal_strength < 0.0f) dog_material_params.normal_strength = 0.0f; }
            if (key == 'X') {dog_material_params.normal_strength += 0.2f; if (dog_material_params.normal_strength > 4.0f) dog_material_params.normal_strength = 4.0f; }
            if (key == 'C') {floor_material_params.normal_strength -= 0.2f; if (floor_material_params.normal_strength < 0.0f) floor_material_params.normal_strength = 0.0f; }
            if (key == 'V') {floor_material_params.normal_strength += 0.2f; if (floor_material_params.normal_strength > 4.0f) floor_material_params.normal_strength = 4.0f; }
            if (key == 'I') IBL_intensity += controls::is_pressed::ctrl ? -0.1f : 0.1f;
            if (key == 'L') directional_light.intensity += controls::is_pressed::ctrl ? -0.1f : 0.1f;
            if (IBL_intensity < 0.0f)
                IBL_intensity = 0.0f;
            if (directional_light.intensity < 0.0f)
                directional_light.intensity = 0.0f;

        }
        if (controls::is_pressed::shift)
        {
            constexpr float speed = 0.1f;
            if (move.right)    directional_light.position.x += speed;
            if (move.left)     directional_light.position.x -= speed;
            if (move.up)       directional_light.position.y += speed;
            if (move.down)     directional_light.position.y -= speed;
            if (move.forward)  directional_light.position.z += speed;
            if (move.backward) directional_light.position.z -= speed;
        }
    }

    void OnShutdown() override {
        debug_image::destroy();
        skybox_renderer::destroy();
        default_material::destroy();
        line_rendering::destroy();
        raster_render_pipeline::destroy();
        mesh_group.vertex_buffer.destroy();
        mesh_group.edge_buffer.destroy();
        floor_gpu_mesh.destroy();
        triangle_gpu_mesh.destroy();

        for (u32 i = 0; i < ImageCount; i++) textures[i].destroy();
        for (u32 i = 0; i < CUBE_MAP_SETS_COUNT; i++) {
            skybox_maps[i].destroy();
            ibl_maps[i * 2 + 0].destroy();
            ibl_maps[i * 2 + 1].destroy();
        }
        
        for (u32 i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) directional_shadhow_maps[i].destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}
#pragma once

#include "../math/mat4_constructurs.h"
#include "../scene/selection.h"

#include "../vulkan/core/graphics.h"
#include "../vulkan/scene/mesh.h"
#include "../vulkan/raster/line_render.h"

void drawSelection(Selection &selection, const gpu::GraphicsCommandBuffer &command_buffer, const mat4 &view_projection_matrix, const Mesh *meshes) {
    if (!(controls::is_pressed::alt && !mouse::is_captured && (selection.geometry || selection.light)))
        return;

    if (selection.geometry) {
        selection.xform = selection.geometry->transform;
        if (selection.geometry->type == GeometryType_Mesh)
            selection.xform.scale *= meshes[selection.geometry->id].aabb.max;
    } else {
        selection.xform.position = selection.light->position_or_direction;
        selection.xform.scale = selection.light->intensity * 0.5f * LIGHT_INTENSITY_RADIUS_FACTOR;
        selection.xform.orientation.reset();
    }

    mat4 mvp_matrix = Mat4(selection.xform) * view_projection_matrix;
    line_rendering::drawBox(command_buffer, mvp_matrix, Yellow, 0.5f);
    if (selection.box_side) {
        ColorID color = White;
        switch (selection.box_side) {
            case BoxSide_Left:  case BoxSide_Right: color = Red;   break;
            case BoxSide_Top:   case BoxSide_Bottom: color = Green; break;
            case BoxSide_Front: case BoxSide_Back: color = Blue;  break;
            case BoxSide_None: break;
        }

        line_rendering::drawBox(command_buffer, mvp_matrix, color, 0.5f, 0.00002f, selection.box_side);
    }
}

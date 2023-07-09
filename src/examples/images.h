#pragma once

#include "../slim/serialization/image.h"
//#include "../slim/core/string.h"

enum ImageID {
    Floor_Albedo,
//    Floor_Normal,
//
//    Dog_Albedo,
//    Dog_Normal,
//
//    Cathedral_SkyboxColor,
//    Cathedral_SkyboxRadiance,
//    Cathedral_SkyboxIrradiance,
//
//    Bolonga_SkyboxColor,
//    Bolonga_SkyboxRadiance,
//    Bolonga_SkyboxIrradiance,

    ImageCount
};

ByteColorImage floor_albedo_image;
ByteColorImage *images = &floor_albedo_image;
const char *image_file_names[ImageCount]{
    "floor_albedo.image",
//    String::getFilePath("floor_normal.image",string_buffers[Floor_Normal],__FILE__),
//
//    String::getFilePath("dog_albedo.image",string_buffers[Dog_Albedo],__FILE__),
//    String::getFilePath("dog_normal.image",string_buffers[Dog_Normal],__FILE__),
//
//    String::getFilePath("cathedral_color.image",string_buffers[Cathedral_SkyboxColor],__FILE__),
//    String::getFilePath("cathedral_radiance.image",string_buffers[Cathedral_SkyboxRadiance],__FILE__),
//    String::getFilePath("cathedral_irradiance.image",string_buffers[Cathedral_SkyboxIrradiance],__FILE__),
//
//    String::getFilePath("bolonga_color.image",string_buffers[Bolonga_SkyboxColor],__FILE__),
//    String::getFilePath("bolonga_radiance.image",string_buffers[Bolonga_SkyboxRadiance],__FILE__),
//    String::getFilePath("bolonga_irradiance.image",string_buffers[Bolonga_SkyboxIrradiance],__FILE__)
};

ImagePack<ByteColor> image_pack{ImageCount, images, image_file_names, __FILE__};
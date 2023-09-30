#pragma once

#include "../slim/serialization/image.h"

enum ImageID {
    Floor_Albedo,
    Floor_Normal,

    Dog_Albedo,
    Dog_Normal,
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
ByteColorImage floor_normal_image;
ByteColorImage dog_albedo_image;
ByteColorImage dog_normal_image;
ByteColorImage *images = &floor_albedo_image;
const char *image_file_names[ImageCount] {
    "floor_albedo.image",
    "floor_normal.image",
    "dog_albedo.image",
    "dog_normal.image"
};

ImagePack<ByteColor> image_pack{ImageCount, images, image_file_names, __FILE__};
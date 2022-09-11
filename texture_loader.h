#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <string_view>
#include "geometry.h"

class TextureLoader
{
public:
    void loadTexture(std::string_view filename);
    uvec3 textureSize(std::string_view filename);
};

#endif // TEXTURE_LOADER_H

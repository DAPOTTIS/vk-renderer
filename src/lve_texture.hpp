#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "lve_buffer.hpp"
#include "lve_device.hpp"

namespace lve {
    class LveTexture{
        LveDevice& device;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;

        LveTexture(LveDevice& device);
        void createTextureImage();
        void createImage(int width, int height);
    };
}
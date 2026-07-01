#pragma once

#include <stb/stb_image.h>
#include "lve_device.hpp"

namespace lve {
    class LveTexture{
        LveDevice& device;
        VkImage textureImage;
        VkImageView imgView;
        VmaAllocation allocation;
        VkSampler sampler;

        public:
        LveTexture(LveDevice& device);
        ~LveTexture();
        void createTextureImage(std::string imgPath);
        void createImage(int width, int height);
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        VkSampler getSampler() const { return sampler; }
        VkImageView getImageView() const { return imgView; }
        VkImage getImage() const { return textureImage; }
    };
}

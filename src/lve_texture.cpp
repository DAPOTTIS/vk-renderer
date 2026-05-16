#include "lve_texture.hpp"

#include <stdexcept>

#include "lve_buffer.hpp"
#include <vulkan/vulkan_core.h>

namespace lve{
    LveTexture::LveTexture(LveDevice& device) : device{device} {}
    
    void LveTexture::createTextureImage(){
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        const VkDeviceSize imageSize = texWidth * texHeight * 4;
        if(!pixels) {
            pixels = stbi_load("textures/default.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        }
        
        LveBuffer stagingBuffer(
            device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        VkResult data = stagingBuffer.map(imageSize, 0);
        stagingBuffer.writeToBuffer((void *)data, imageSize, 0);
        stagingBuffer.unmap();
        stbi_image_free(pixels);

        createImage(texWidth, texHeight);
    }
    void LveTexture::createImage(int width, int height){
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1; //no mips for now
        imageInfo.arrayLayers = 1; //not an array (I assume that means not a texture atlas?)
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        // texels laid out in order optimal for access from the shader,
        // cant be changed later, so if i need direct access this should be linear instead of optimal
        // but thats unneeded since we arent using staging images
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        // preinitialized would preserve texels during first transition. only useful if we used linear tiling
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        //only used in 1 queue
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // multisampling flags

        if (vkCreateImage(device.device(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.device(), textureImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device.device(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate memory!");
        }

        vkBindImageMemory(device.device(), textureImage, textureImageMemory, 0);
    }
}
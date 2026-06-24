#define STB_IMAGE_IMPLEMENTATION
#include "lve_texture.hpp"

#include <stdexcept>

#include "lve_buffer.hpp"
#include "lve_device.hpp"
#include <vulkan/vulkan_core.h>

namespace lve{
    LveTexture::LveTexture(LveDevice& device) : device{device} {
        createTextureImage();
    }

    LveTexture::~LveTexture() {
        vkDestroySampler(device.device(), sampler, nullptr);
        vkDestroyImageView(device.device(), imgView, nullptr);
        vmaDestroyImage(device._allocator, textureImage, allocation);
    }

    void LveTexture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // we're not transferring queue family ownership
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = textureImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            // before the copy: nothing needs to finish, copy needs to wait
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // after the copy: copy needs to finish, shader read needs to wait
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::runtime_error("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        device.endSingleTimeCommands(commandBuffer);
    }

    void LveTexture::createTextureImage(){
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if(!pixels) {
            pixels = stbi_load("textures/default.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        }
        if(!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        const VkDeviceSize imageSize = texWidth * texHeight * 4;

        // 1. Create staging buffer and copy pixel data into it
        LveBuffer stagingBuffer(
            device,
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        stagingBuffer.map();
        stagingBuffer.writeToBuffer(pixels);
        stagingBuffer.unmap();
        stbi_image_free(pixels);

        // 2. Create the VkImage, image view, and sampler
        createImage(texWidth, texHeight);

        // 3. Transition image from UNDEFINED to TRANSFER_DST so we can copy into it
        transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // 4. Copy staging buffer contents into the image
        device.copyBufferToImage(
            stagingBuffer.getBuffer(),
            textureImage,
            static_cast<uint32_t>(texWidth),
            static_cast<uint32_t>(texHeight),
            1);

        // 5. Transition image to SHADER_READ_ONLY so the fragment shader can sample it
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // stagingBuffer is automatically cleaned up when it goes out of scope
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

        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        //we finally create the image
        VkResult err = vmaCreateImage(device._allocator, &imageInfo, &vmaAllocInfo, &textureImage, &allocation, nullptr);
        if (err) {
            throw std::runtime_error("failed to create image!");
        }

        //we create the image view so we can pass it to the shader n stuff
        VkImageViewCreateInfo view{};
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = textureImage;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = VK_FORMAT_R8G8B8A8_SRGB;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;
        view.subresourceRange.levelCount = 1;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device.physicalDevice(), &properties);

        err = vkCreateImageView(device.device(), &view, nullptr, &imgView);
        if (err) {
            throw std::runtime_error("failed to create image view!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // mag and min are for oversampling/undersampling respectively, can either be nearest
        // or linear
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        //addressing mode, repeat just repeats the texture once we're outside of the sampling bound
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //anisotropic filtering, self explanatory, we got it from the device's limits tho
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        //black border color for when sampling outside of the range
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // decides whether values are in a 0-1 range or a raw 0-texwidth/height range
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        //compare operation is mainly for PCF shadows and stuff that needs value comparisons
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        //we dont have mips rn
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        err = vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler);
        if(err){
            throw std::runtime_error("failed to create sampler");
        }
    }
}


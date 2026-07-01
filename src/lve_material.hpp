#pragma once

#include <glm/glm.hpp>
#include <memory>
#include "lve_texture.hpp"
#include "lve_device.hpp"

namespace lve{
    class LveMaterial{
        public:
        void setTextureDescriptorSet(VkDescriptorSet descriptorSet) { textureDescriptorSet = descriptorSet; }
        const VkDescriptorSet& getTextureDescriptorSet() { return textureDescriptorSet; }
        void setTexture(std::shared_ptr<LveTexture> texture) { this->texture = texture; }
        const std::shared_ptr<LveTexture>& getTexture() { return texture; }

        LveMaterial(LveDevice& device, std::string texPath){
            texture = std::make_shared<LveTexture>(device);
            texture->createTextureImage(texPath);
        }

        LveMaterial(LveDevice& device){
            texture = std::make_shared<LveTexture>(device);
            texture->createTextureImage("");
        }

        VkDescriptorImageInfo createImageInfo(){
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.sampler = texture->getSampler();
            imageInfo.imageView = texture->getImageView();
            return imageInfo;
        }
        private:
        glm::vec3 albedo{0,0,0};
        std::shared_ptr<LveTexture> texture{};
        VkDescriptorSet textureDescriptorSet;
    };
};

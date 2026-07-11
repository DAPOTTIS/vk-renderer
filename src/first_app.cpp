#include "first_app.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

#include "keyboard_movement_controller.hpp"
#include "lve_buffer.hpp"
#include "lve_camera.hpp"
#include "lve_descriptors.hpp"
#include "lve_material.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <cassert>
#include <chrono>

//imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace lve {


FirstApp::FirstApp() {
  globalPool =
      LveDescriptorPool::Builder(lveDevice)
          .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT + 100)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
          .build();
  textureSetLayout =
      LveDescriptorSetLayout::Builder(lveDevice)
          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();
  loadGameObjects();
  initImGui();
}

FirstApp::~FirstApp() {
  vkDeviceWaitIdle(lveDevice.device());
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void FirstApp::initImGui() {
  // Create ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  // Initialize GLFW backend
  ImGui_ImplGlfw_InitForVulkan(lveWindow.getGLFWwindow(), true);

  // Initialize Vulkan backend
  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.ApiVersion = VK_API_VERSION_1_2;
  init_info.Instance = lveDevice.getInstance();
  init_info.PhysicalDevice = lveDevice.physicalDevice();
  init_info.Device = lveDevice.device();
  init_info.QueueFamily = lveDevice.findPhysicalQueueFamilies().graphicsFamily;
  init_info.Queue = lveDevice.graphicsQueue();
  init_info.DescriptorPoolSize = 10;  // ImGui creates its own pool
  init_info.MinImageCount = 2;
  init_info.ImageCount = LveSwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.PipelineInfoMain.RenderPass = lveRenderer.getSwapChainRenderPass();
  init_info.PipelineInfoMain.Subpass = 0;
  init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info);
}

void FirstApp::run() {
  std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] = std::make_unique<LveBuffer>(
        lveDevice,
        sizeof(GlobalUbo),
        1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }



  auto globalSetLayout =
      LveDescriptorSetLayout::Builder(lveDevice)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < globalDescriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    LveDescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  SimpleRenderSystem simpleRenderSystem{
      lveDevice,
      lveRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout(),
      textureSetLayout->getDescriptorSetLayout()
  };
  PointLightSystem pointLightSystem{
      lveDevice,
      lveRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout()};
  LveCamera camera{};

  auto viewerObject = LveGameObject::createGameObject();
  viewerObject.transform.translation.z = -2.5f;
  KeyboardMovementController cameraController{};

  auto currentTime = std::chrono::high_resolution_clock::now();

  while (!lveWindow.shouldClose()) {
    glfwPollEvents();

    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
    currentTime = newTime;

    cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
    camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

    float aspect = lveRenderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

    if (auto commandBuffer = lveRenderer.beginFrame()) {
      int frameIndex = lveRenderer.getFrameIndex();
      FrameInfo frameInfo{
          frameIndex,
          frameTime,
          commandBuffer,
          camera,
          globalDescriptorSets[frameIndex],
          gameObjects};

      // update
      GlobalUbo ubo{};
      ubo.projection = camera.getProjection();
      ubo.view = camera.getView();
      ubo.inverseView = camera.getInverseView();
      pointLightSystem.update(frameInfo, ubo);
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();


      // render
      lveRenderer.beginSwapChainRenderPass(commandBuffer);

      simpleRenderSystem.renderGameObjects(frameInfo);
      pointLightSystem.render(frameInfo);

      // ImGui rendering - draw on top of everything
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Demo window for testing - remove later
      ImGui::ShowDemoWindow();

      ImGui::Render();
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

      lveRenderer.endSwapChainRenderPass(commandBuffer);
      lveRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::loadGameObjects() {
    auto defaultMaterial = std::make_shared<LveMaterial>(lveDevice, "textures/default.png");
    VkDescriptorImageInfo defaultImageInfo = defaultMaterial->createImageInfo();
    VkDescriptorSet defaultTextureDescriptorSet;
    LveDescriptorWriter(*textureSetLayout, *globalPool)
        .writeImage(0, &defaultImageInfo)
        .build(defaultTextureDescriptorSet);
    defaultMaterial->setTextureDescriptorSet(defaultTextureDescriptorSet);
    std::shared_ptr<LveModel> lveModel =
        LveModel::createModelFromFile(lveDevice, "models/flat_vase.obj");
    auto flatVase = LveGameObject::createGameObject();
    flatVase.model = lveModel;
    flatVase.material = defaultMaterial;
    flatVase.transform.translation = {-.5f, .5f, 0.f};
    flatVase.transform.scale = glm::vec3{3.f};
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));


    lveModel = LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    auto smoothVase = LveGameObject::createGameObject();
    smoothVase.model = lveModel;
    smoothVase.material = defaultMaterial;

    smoothVase.transform.translation = {.5f, .5f, 0.f};
    smoothVase.transform.scale = glm::vec3{3.f};
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    auto material = std::make_shared<LveMaterial>(lveDevice, "textures/texture.jpg");
    VkDescriptorImageInfo imageInfo = material->createImageInfo();
    VkDescriptorSet textureDescriptorSet;
    LveDescriptorWriter(*textureSetLayout, *globalPool)
        .writeImage(0, &imageInfo)
        .build(textureDescriptorSet);
    material->setTextureDescriptorSet(textureDescriptorSet);


    lveModel = LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    auto floor = LveGameObject::createGameObject();
    floor.material = material;
    floor.model = lveModel;
    floor.transform.translation = {0.f, .5f, 0.f};
    floor.transform.scale = {3.f, 1.f, 3.f};
    gameObjects.emplace(floor.getId(), std::move(floor));

    std::vector<glm::vec3> lightColors{
        {1.f, .1f, .1f},
        {.1f, .1f, 1.f},
        {.1f, 1.f, .1f},
        {1.f, 1.f, .1f},
        {.1f, 1.f, 1.f},
        {1.f, 1.f, 1.f}
        };

    for(int i = 0; i < lightColors.size(); i++){
        auto pointLight = LveGameObject::makePointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(
                            glm::mat4(1.f),
                            (i *  glm::two_pi<float>())/lightColors.size(),
                            {0.f, -1.f, 0.f });
        pointLight.transform.translation = glm::vec3(rotateLight* glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}
}  // namespace lve

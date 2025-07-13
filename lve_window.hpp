#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace lve {
    class LveWindow {
    public:
        LveWindow(int w, int h, std::string name);

        ~LveWindow();

        LveWindow(const LveWindow &) = delete;

        LveWindow &operator=(const LveWindow &) = delete;
        LveWindow() = default;
    
        bool shouldClose() { return glfwWindowShouldClose(window); }
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
        bool wasWindowResized() {return frameBufferResized;}
        void resetWindowResized() { frameBufferResized = 0; }
        
        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        
        GLFWwindow *getGLFWwindow() const { return window; }
    private:
        static void frameBufferResizeCallback(GLFWwindow *window, int width, int height);
        void initWindow();

        int width;
        int height;
        bool frameBufferResized = false;

        std::string windowName;
        GLFWwindow *window;
    };
} // namespace lve

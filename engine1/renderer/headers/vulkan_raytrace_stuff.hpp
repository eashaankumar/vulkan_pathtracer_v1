#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // configures glm to match vulkan's depth range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL2/SDL_vulkan.h>

#include <vector>
#include <memory>

#ifndef VULKAN_RAYTRACE_STUFF
#define VULKAN_RAYTRACE_STUFF

struct VulkanBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::DeviceAddress address;
};

struct Vertex {
        float pos[3];
};

struct VulkanAccelerationStructure
{
    vk::AccelerationStructureKHR accelerationStructure;
    VulkanBuffer structureBuffer;
    VulkanBuffer scratchBuffer;
    VulkanBuffer instancesBuffer;
};

struct UniformData
{
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
};

struct VulkanImage
{
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView imageView;
};


class Shader
{
    public:
    
    enum ShaderType
    {
        RayGen, Miss, ClosestHit
    };

    vk::ShaderModule shaderModule;
    ShaderType shaderType;
    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo;
    vk::RayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo;
    std::string path;
    std::shared_ptr<vk::Device> device;

    Shader(const Shader& shader);
    Shader(const std::string& path, ShaderType shaderType, vk::Device& device, uint32_t shaderIndex);
    void UnloadShaderModule();

};

class RayTracingPipeline
{
    private:
        std::vector<Shader> shaders;
    public:
        std::unique_ptr<vk::Device> device;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;

        RayTracingPipeline(vk::Device& device);

        void AddShader(const std::string& path, Shader::ShaderType shaderType);
        void CreatePipeline(vk::PhysicalDevice& physicalDevice, vk::DescriptorSetLayout& rtDescriptorSetLayout, vk::DispatchLoaderDynamic& dynamicDispatchLoader, uint32_t maxRayRecursionDepth);
        void DestroyPipeline();
};


class VulkanRaytraceStuff
{
public:
    void init(const char* appname, int width, int height);
    void run();
    ~VulkanRaytraceStuff();
    
    vk::Extent2D extent;
    SDL_Window* window;
    VulkanBuffer cameraUniformBuffer;
    std::vector<const char *> requiredInstanceExtensions;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    uint32_t queueId;
    vk::Device device;
    vk::DispatchLoaderDynamic dynamicDispatchLoader;
    vk::Queue computePresentQueue;
    vk::CommandPool commandPool;
    vk::SwapchainKHR swapChain;

    std::unique_ptr<RayTracingPipeline> pipeline;

    VkSurfaceKHR surface;

    vk::Semaphore semaphore, semaphore2;
    vk::Fence fence;
    std::vector<vk::CommandBuffer> commandBuffers;

    VulkanAccelerationStructure topAccelerationStructure, bottomAccelerationStructure;

    VulkanImage renderTargetImage;
};

struct Mesh
{
    public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkTransformMatrixKHR transformMatrix;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    VulkanBuffer transformBuffer;

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress;
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress;
    vk::AccelerationStructureGeometryKHR geometryBLAS;
    
    uint32_t NumTriangles()
    {
        return static_cast<uint32_t>(indices.size() / 3);
    }

    uint32_t IndexCount()
    {
        return static_cast<uint32_t>(indices.size());
    }

    void Prepare(vk::PhysicalDevice& physicalDevice, vk::Device& device);
};

class RayTracingAccelerationStructure
{
    public:
    RayTracingAccelerationStructure();

    void AddMesh(Mesh& mesh, glm::mat4 trs);
    void Build();
    void Clear();
    void Destroy();
};

#endif
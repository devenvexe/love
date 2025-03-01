/**
 * Copyright (c) 2006-2023 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#pragma once

// löve
#include "common/config.h"
#include "graphics/Graphics.h"
#include "StreamBuffer.h"
#include "ShaderStage.h"
#include "Shader.h"
#include "Texture.h"

// libraries
#include "VulkanWrapper.h"
#include "libraries/xxHash/xxhash.h"

// c++
#include <iostream>
#include <memory>
#include <functional>
#include <set>


namespace love
{
namespace graphics
{
namespace vulkan
{

struct RenderPassAttachment
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	bool discard = true;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	bool operator==(const RenderPassAttachment &attachment) const
	{
		return format == attachment.format && 
			discard == attachment.discard && 
			msaaSamples == attachment.msaaSamples;
	}
};

struct RenderPassConfiguration
{
	std::vector<RenderPassAttachment> colorAttachments;

	struct StaticRenderPassConfiguration
	{
		RenderPassAttachment depthAttachment;
		bool resolve = false;
	} staticData;

	bool operator==(const RenderPassConfiguration &conf) const
	{
		return colorAttachments == conf.colorAttachments && 
			(memcmp(&staticData, &conf.staticData, sizeof(StaticRenderPassConfiguration)) == 0);
	}
};

struct RenderPassConfigurationHasher
{
	size_t operator()(const RenderPassConfiguration &configuration) const
	{
		size_t hashes[] = { 
			XXH32(configuration.colorAttachments.data(), configuration.colorAttachments.size() * sizeof(VkFormat), 0),
			XXH32(&configuration.staticData, sizeof(configuration.staticData), 0),
		};
		return XXH32(hashes, sizeof(hashes), 0);
	}
};

struct FramebufferConfiguration
{
	std::vector<VkImageView> colorViews;

	struct StaticFramebufferConfiguration
	{
		VkImageView depthView = VK_NULL_HANDLE;
		VkImageView resolveView = VK_NULL_HANDLE;

		uint32_t width = 0;
		uint32_t height = 0;

		VkRenderPass renderPass = VK_NULL_HANDLE;
	} staticData;

	bool operator==(const FramebufferConfiguration &conf) const
	{
		return colorViews == conf.colorViews &&
			(memcmp(&staticData, &conf.staticData, sizeof(StaticFramebufferConfiguration)) == 0);
	}
};

struct FramebufferConfigurationHasher
{
	size_t operator()(const FramebufferConfiguration &configuration) const
	{
		size_t hashes[] = {
			XXH32(configuration.colorViews.data(), configuration.colorViews.size() * sizeof(VkImageView), 0),
			XXH32(&configuration.staticData, sizeof(configuration.staticData), 0),
		};

		return XXH32(hashes, sizeof(hashes), 0);
	}
};

struct OptionalInstanceExtensions
{
	// VK_KHR_get_physical_device_properties2
	bool physicalDeviceProperties2 = false;
};

struct OptionalDeviceFeatures
{
	// VK_EXT_extended_dynamic_state
	bool extendedDynamicState = false;

	// VK_KHR_get_memory_requirements2
	bool memoryRequirements2 = false;

	// VK_KHR_dedicated_allocation
	bool dedicatedAllocation = false;

	// VK_KHR_buffer_device_address
	bool bufferDeviceAddress = false;

	// VK_EXT_memory_budget
	bool memoryBudget = false;

	// VK_KHR_shader_float_controls
	bool shaderFloatControls = false;

	// VK_KHR_spirv_1_4
	bool spirv14 = false;
};

struct GraphicsPipelineConfiguration
{
	VkRenderPass renderPass;
	VertexAttributes vertexAttributes;
	Shader *shader = nullptr;
	bool wireFrame;
	BlendState blendState;
	ColorChannelMask colorChannelMask;
	VkSampleCountFlagBits msaaSamples;
	uint32_t numColorAttachments;
	PrimitiveType primitiveType;

	struct DynamicState
	{
		CullMode cullmode = CULL_NONE;
		Winding winding = WINDING_MAX_ENUM;
		StencilAction stencilAction = STENCIL_MAX_ENUM;
		CompareMode stencilCompare = COMPARE_MAX_ENUM;
		DepthState depthState{};
	} dynamicState;

	GraphicsPipelineConfiguration()
	{
		memset(this, 0, sizeof(GraphicsPipelineConfiguration));
	}

	bool operator==(const GraphicsPipelineConfiguration &other) const
	{
		return memcmp(this, &other, sizeof(GraphicsPipelineConfiguration)) == 0;
	}
};

struct GraphicsPipelineConfigurationHasher
{
	size_t operator() (const GraphicsPipelineConfiguration &configuration) const
	{
		return XXH32(&configuration, sizeof(GraphicsPipelineConfiguration), 0);
	}
};

struct QueueFamilyIndices
{
	Optional<uint32_t> graphicsFamily;
	Optional<uint32_t> presentFamily;

	bool isComplete() const
	{
		return graphicsFamily.hasValue && presentFamily.hasValue;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct RenderpassState
{
	bool active = false;
	VkRenderPassBeginInfo beginInfo{};
	bool useConfigurations = false;
	RenderPassConfiguration renderPassConfiguration{};
	FramebufferConfiguration framebufferConfiguration{};
	VkPipeline pipeline = VK_NULL_HANDLE;
	std::vector<VkImage> transitionImages;
	uint32_t numColorAttachments = 0;
	float width = 0.0f;
	float height = 0.0f;
	VkSampleCountFlagBits msaa = VK_SAMPLE_COUNT_1_BIT;
};

struct ScreenshotReadbackBuffer
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;

	VkImage image;
	VmaAllocation imageAllocation;
};

class Graphics final : public love::graphics::Graphics
{
public:
	Graphics();
	~Graphics();

	const char *getName() const override;
	const VkDevice getDevice() const;
	const VmaAllocator getVmaAllocator() const;

	// implementation for virtual functions
	love::graphics::Texture *newTexture(const love::graphics::Texture::Settings &settings, const love::graphics::Texture::Slices *data) override;
	love::graphics::Buffer *newBuffer(const love::graphics::Buffer::Settings &settings, const std::vector<love::graphics::Buffer::DataDeclaration>& format, const void *data, size_t size, size_t arraylength) override;
	void clear(OptionalColorD color, OptionalInt stencil, OptionalDouble depth) override;
	void clear(const std::vector<OptionalColorD> &colors, OptionalInt stencil, OptionalDouble depth) override;
	Matrix4 computeDeviceProjection(const Matrix4 &projection, bool rendertotexture) const override;
	void discard(const std::vector<bool>& colorbuffers, bool depthstencil) override;
	void present(void *screenshotCallbackdata) override;
	void setViewportSize(int width, int height, int pixelwidth, int pixelheight) override;
	bool setMode(void *context, int width, int height, int pixelwidth, int pixelheight, bool windowhasstencil, int msaa) override;
	void unSetMode() override;
	void setActive(bool active) override;
	int getRequestedBackbufferMSAA() const override;
	int getBackbufferMSAA() const  override;
	void setColor(Colorf c) override;
	void setScissor(const Rect &rect) override;
	void setScissor() override;
	void setStencilMode(StencilAction action, CompareMode compare, int value, love::uint32 readmask, love::uint32 writemask) override;
	void setDepthMode(CompareMode compare, bool write) override;
	void setFrontFaceWinding(Winding winding) override;
	void setColorMask(ColorChannelMask mask) override;
	void setBlendState(const BlendState &blend) override;
	void setPointSize(float size) override;
	void setWireframe(bool enable) override;
	PixelFormat getSizedFormat(PixelFormat format, bool rendertarget, bool readable) const override;
	bool isPixelFormatSupported(PixelFormat format, uint32 usage, bool sRGB) override;
	Renderer getRenderer() const override;
	bool usesGLSLES() const override;
	RendererInfo getRendererInfo() const override;
	void draw(const DrawCommand &cmd) override;
	void draw(const DrawIndexedCommand &cmd) override;
	void drawQuads(int start, int count, const VertexAttributes &attributes, const BufferBindings &buffers, graphics::Texture *texture) override;

	graphics::GraphicsReadback *newReadbackInternal(ReadbackMethod method, love::graphics::Buffer *buffer, size_t offset, size_t size, data::ByteData *dest, size_t destoffset) override;
	graphics::GraphicsReadback *newReadbackInternal(ReadbackMethod method, love::graphics::Texture *texture, int slice, int mipmap, const Rect &rect, image::ImageData *dest, int destx, int desty) override;

	// internal functions.

	VkCommandBuffer getCommandBufferForDataTransfer();
	void queueCleanUp(std::function<void()> cleanUp);
	void addReadbackCallback(std::function<void()> callback);
	void submitGpuCommands(bool present, void *screenshotCallbackData = nullptr);
	const VkDeviceSize getMinUniformBufferOffsetAlignment() const;
	graphics::Texture *getDefaultTexture() const;
	VkSampler getCachedSampler(const SamplerState &sampler);
	void setComputeShader(Shader *computeShader);
	std::set<Shader*> &getUsedShadersInFrame();
	graphics::Shader::BuiltinUniformData getCurrentBuiltinUniformData();
	const OptionalDeviceFeatures &getEnabledOptionalDeviceExtensions() const;
	VkSampleCountFlagBits getMsaaCount(int requestedMsaa) const;
	void setVsync(int vsync);
	int getVsync() const;

protected:
	graphics::ShaderStage *newShaderStageInternal(ShaderStageType stage, const std::string &cachekey, const std::string &source, bool gles) override;
	graphics::Shader *newShaderInternal(StrongRef<love::graphics::ShaderStage> stages[SHADERSTAGE_MAX_ENUM]) override;
	graphics::StreamBuffer *newStreamBuffer(BufferUsage type, size_t size) override;
	bool dispatch(int x, int y, int z) override;
	void initCapabilities() override;
	void getAPIStats(int &shaderswitches) const override;
	void setRenderTargetsInternal(const RenderTargets &rts, int pixelw, int pixelh, bool hasSRGBtexture) override;

private:
	void createVulkanInstance();
	bool checkValidationSupport();
	void pickPhysicalDevice();
	int rateDeviceSuitability(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	void createLogicalDevice();
	void initVMA();
	void createSurface();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
	VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR &capabilities);
	void createSwapChain();
	void createImageViews();
	void createScreenshotCallbackBuffers();
	void createDefaultRenderPass();
	void createDefaultFramebuffers();
	VkFramebuffer createFramebuffer(FramebufferConfiguration &configuration);
	VkFramebuffer getFramebuffer(FramebufferConfiguration &configuration);
	void createDefaultShaders();
	VkRenderPass createRenderPass(RenderPassConfiguration &configuration);
	VkPipeline createGraphicsPipeline(GraphicsPipelineConfiguration &configuration);
	void createColorResources();
	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	void createDepthResources();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void createDefaultTexture();
	void cleanup();
	void cleanupSwapChain();
	void recreateSwapChain();
	void initDynamicState();
	void beginFrame();
	void startRecordingGraphicsCommands();
	void endRecordingGraphicsCommands();
	void ensureGraphicsPipelineConfiguration(GraphicsPipelineConfiguration &configuration);
	bool usesConstantVertexColor(const VertexAttributes &attribs);
	void createVulkanVertexFormat(
		VertexAttributes vertexAttributes, 
		std::vector<VkVertexInputBindingDescription> &bindingDescriptions, 
		std::vector<VkVertexInputAttributeDescription> &attributeDescriptions);
	void prepareDraw(
		const VertexAttributes &attributes,
		const BufferBindings &buffers, graphics::Texture *texture,
		PrimitiveType, CullMode);
	void setRenderPass(const RenderTargets &rts, int pixelw, int pixelh, bool hasSRGBtexture);
	void setDefaultRenderPass();
	void startRenderPass();
	void endRenderPass();
	VkSampler createSampler(const SamplerState &sampler);
	void cleanupUnusedObjects();
	void requestSwapchainRecreation();

	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	uint32_t deviceApiVersion = VK_API_VERSION_1_0;
	bool windowHasStencil = false;
	int requestedMsaa = 0;
	VkDevice device = VK_NULL_HANDLE; 
	OptionalInstanceExtensions optionalInstanceExtensions;
	OptionalDeviceFeatures optionalDeviceFeatures;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSurfaceTransformFlagBitsKHR preTransform = {};
	Matrix4 displayRotation;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = VkExtent2D();
	std::vector<VkImageView> swapChainImageViews;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage colorImage = VK_NULL_HANDLE;
	VkImageView colorImageView = VK_NULL_HANDLE;
	VmaAllocation colorImageAllocation = VK_NULL_HANDLE;
	VkImage depthImage = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
	VmaAllocation depthImageAllocation = VK_NULL_HANDLE;
	VkRenderPass defaultRenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> defaultFramebuffers;
	std::unordered_map<RenderPassConfiguration, VkRenderPass, RenderPassConfigurationHasher> renderPasses;
	std::unordered_map<FramebufferConfiguration, VkFramebuffer, FramebufferConfigurationHasher> framebuffers;
	std::unordered_map<GraphicsPipelineConfiguration, VkPipeline, GraphicsPipelineConfigurationHasher> graphicsPipelines;
	std::unordered_map<VkRenderPass, bool> renderPassUsages;
	std::unordered_map<VkFramebuffer, bool> framebufferUsages;
	std::unordered_map<VkPipeline, bool> pipelineUsages;
	std::unordered_map<uint64, VkSampler> samplers;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> commandBuffers;
	Shader *computeShader = nullptr;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	int vsync = 1;
	VkDeviceSize minUniformBufferOffsetAlignment = 0;
	bool imageRequested = false;
	uint32_t frameCounter = 0;
	size_t currentFrame = 0;
	uint32_t imageIndex = 0;
	bool swapChainRecreationRequested = false;
	bool transitionColorDepthLayouts = false;
	VmaAllocator vmaAllocator = VK_NULL_HANDLE;
	StrongRef<love::graphics::Texture> defaultTexture;
	StrongRef<love::graphics::Buffer> defaultConstantColor;
	// functions that need to be called to cleanup objects that were needed for rendering a frame.
	// We need a vector for each frame in flight.
	std::vector<std::vector<std::function<void()>>> cleanUpFunctions;
	std::vector<std::vector<std::function<void()>>> readbackCallbacks;
	std::vector<ScreenshotReadbackBuffer> screenshotReadbackBuffers;
	std::set<Shader*> usedShadersInFrame;
	RenderpassState renderPassState;
};

} // vulkan
} // graphics
} // love

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "builder/VulkanTexture2dBuilder.h"
#include "engine/EngineCore.h"
#include "light/Light.h"
#include "engine/utils/math/Math.h"
#include "texture/VulkanTexture2d.h"
#include "mesh/Mesh.h"
#include "mesh/MeshInstance.h"
#include "services/renderer/Renderer.h"
#include "storage/VulkanStorage.h"
#include "VulkanInitializer.h"


namespace parus::imgui
{
	class ImGuiLibrary;
}

namespace parus::vulkan
{

	class VulkanRenderer final : public Renderer
	{
	public:
		~VulkanRenderer() override = default;

		void init() override;
		void registerEvents() override;
		void clean() override;
		void drawFrame() override;
		void deviceWaitIdle() override;

		friend class parus::imgui::ImGuiLibrary;
		friend VulkanTexture2d VulkanTexture2dBuilder::buildFromFile(const std::string& filePath);
		friend VulkanTexture2d VulkanTexture2dBuilder::buildFromSolidColor(const math::Vector3& color);

	private:
		VulkanStorage storage;
		VulkanInitializer initializer;

		static constexpr float Z_NEAR = 0.1f;
		static constexpr float Z_FAR = 1500.0f;

		bool isDrawDebugEnabled = false;

		std::vector<MeshInstance> meshInstances;
		VulkanDirectionalLight directionalLight;

		void cleanupFrameResources();

		std::queue<std::pair<std::string, std::shared_ptr<Mesh>>> modelQueue;
		std::mutex importModelMutex;

		VulkanTexture2d cubemap;

		// Load model
		void importMesh(const std::string& meshPath, const MeshType meshType = MeshType::STATIC_MESH);
		void processLoadedMeshes();
		[[nodiscard]] bool flushMeshQueue();
		void rebuildSceneBuffers();
		void rebuildDescriptorSets();

		void loadSceneAssets();

		// SwapChain
		[[nodiscard]] std::optional<uint32_t> acquireNextImage();

		void createCubemapTexture();

		// Command buffer
		void resetCommandBuffer(const int bufferId) const;
		void setFullscreenViewportScissor(VkCommandBuffer cmd) const;
		void drawMainScenePass(VkCommandBuffer commandBufferToRecord) const;
		void drawSkyboxPass(VkCommandBuffer commandBufferToRecord) const;
		void recordCommandBuffer(VkCommandBuffer commandBufferToRecord, uint32_t imageIndex) const;

		[[nodiscard]] VkCommandBuffer getCommandBuffer(const int bufferId) const;

		// Texture image
		void generateMipmaps(
			const VulkanTexture2d& texture,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight);

		void transitionImageLayout(
			const VkImage image,
			VkFormat format,
			const VkImageLayout oldLayout,
			const VkImageLayout newLayout,
			uint32_t mipLevels);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		// Buffer manager
		void createSkyVertexBuffer(const std::vector<math::Vertex>& vertices);
		void createVertexBuffer(const std::vector<math::Vertex>& vertices);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void createSkyIndexBuffer(const std::vector<uint32_t>& indices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);
		void updateUniformBuffer(uint32_t currentImage);

		// Descriptor Sets
		void createMeshDescriptorSets(const std::shared_ptr<Mesh>& mesh);
		void createGlobalDescriptorSets();
		void createInstanceDescriptorSets();
		void createMaterialDescriptorSets(const std::shared_ptr<Mesh>& mesh) const;
		void createLightsDescriptorSets();

		void onResize();
		
		int currentFrame = 0;
		bool framebufferResized = false;

		struct FrameData {
			VkCommandPool commandPool;
			VkCommandBuffer commandBuffer;
			VkFence fence;
			std::vector<std::pair<VkBuffer, VkDeviceMemory>> buffersToDelete;
		};

		std::array<FrameData, VulkanStorage::MAX_FRAMES_IN_FLIGHT> frames;

		bool isRunning = false;

	};

}


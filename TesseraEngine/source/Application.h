#pragma once

#include <list>
#include <memory>

#include "glfw/GlfwInitializer.h"
#include "vulkan/CommandBufferManager.h"
#include "vulkan/DebugManager.h"
#include "vulkan/FramebufferManager.h"
#include "vulkan/GraphicsPipelineManager.h"
#include "vulkan/ImageViewManager.h"
#include "vulkan/InstanceManager.h"
#include "vulkan/QueueManager.h"
#include "vulkan/SurfaceManager.h"
#include "vulkan/SwapChainManager.h"
#include "vulkan/SyncObjectsManager.h"
#include "vulkan/BufferManager.h"
#include "vulkan/DescriptorSetLayoutManager.h"


namespace tessera
{
	class Application final : public Initializable
	{
	public:
		void init() override;
		void loop() const;
		void clean() override;

	private:
		template <class T>
		void registerServiceManager(const T* servicePointer, const std::shared_ptr<Initializable>& service);

		static inline std::list<std::shared_ptr<Initializable>> initializerList = {
			std::make_shared<glfw::GlfwInitializer>(),
			std::make_shared<vulkan::InstanceManager>(),
			std::make_shared<vulkan::DebugManager>(),
			std::make_shared<vulkan::SurfaceManager>(),
			std::make_shared<vulkan::DeviceManager>(),
			std::make_shared<vulkan::QueueManager>(),
			std::make_shared<vulkan::SwapChainManager>(),
			std::make_shared<vulkan::ImageViewManager>(),
			std::make_shared<vulkan::DescriptorSetLayoutManager>(),
			std::make_shared<vulkan::GraphicsPipelineManager>(),
			std::make_shared<vulkan::FramebufferManager>(),
			std::make_shared<vulkan::CommandBufferManager>(),
			std::make_shared<vulkan::BufferManager>(),
			std::make_shared<vulkan::SyncObjectsManager>(),
		};
	};

}


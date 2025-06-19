#pragma once

#include "utils/interfaces/Service.h"

namespace tessera
{
	/**
	 * @brief Interface for Vulkan service.
	 *
	 * This class defines the interface for a services that can be initialized or destructed.
	 * Used primarily in Vulkan modules.
	 */
	class Initializable : public Service
	{
	public:
		Initializable() = default;
		
		virtual ~Initializable() = default;

		/**
		 * @brief Initialize the Vulkan service.
		 *
		 * Derived classes must implement this method to perform
		 * initialization tasks for the Vulkan service.
		 */
		virtual void init() {}

		/**
		 * @brief Clean up resources used by the Vulkan service.
		 *
		 * Derived classes must implement this method to perform
		 * cleanup tasks for the Vulkan service, releasing any
		 * resources that were allocated during initialization.
		 */
		virtual void clean() = 0;

	};

}


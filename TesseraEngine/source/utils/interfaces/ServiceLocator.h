#pragma once
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "Service.h"

namespace tessera
{
	/**
	 * @brief Service Locator class for managing dependencies.
	 */
	class ServiceLocator
	{
	public:
		/**
		 * @brief Method to register a service.
		 *
		 * @param servicePointer The shared pointer to the service instance.
		 * @param service
		 */
		template<typename T>
		static void registerService(const T* servicePointer, const std::shared_ptr<Service>& service)
		{
			services[&typeid(*servicePointer)] = service;
		}

		/**
		 * @brief Method to get a service.
		 *
		 * @tparam T The type of service to retrieve.
		 * @return std::shared_ptr<T> The shared pointer to the service instance.
		 */
		template<typename T>
		static std::shared_ptr<T> getService()
		{
			const auto it = services.find(&typeid(T));

			if (it == services.end())
			{
				throw std::runtime_error(std::string("ServiceLocator: failed to get instance of service ") + typeid(T).name());
			}

			return std::static_pointer_cast<T>(it->second);
		}

	private:
		static std::unordered_map<const std::type_info*, std::shared_ptr<Service>> services; /**< Static member variable to store services. */
	};

}


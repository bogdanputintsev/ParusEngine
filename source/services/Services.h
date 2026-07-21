#pragma once

#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "Service.h"
#include "engine/Asserts.h"

namespace parus
{
	/**
	 * Service locator for dependency injection and service discovery.
	 * Provides type-safe registration and retrieval of core engine services.
	 */
	class Services final
	{
	public:
		/** Register a service instance by its type for later retrieval. */
		template<typename TKey>
		static void registerService(const std::shared_ptr<Service>& service)
		{
			services[&typeid(TKey)] = service;
		}

		/** Retrieve a registered service by type; throws if not found. */
		template<typename T>
		static std::shared_ptr<T> get()
		{
			const auto& serviceIterator = services.find(&typeid(T));
			ASSERT(serviceIterator != services.end(), std::string("Failed to get instance of service ") + typeid(T).name());

			return std::static_pointer_cast<T>(serviceIterator->second);
		}

	private:
		/** Type-indexed map holding all registered service instances. */
		static std::unordered_map<const std::type_info*, std::shared_ptr<Service>> services;
	};

}

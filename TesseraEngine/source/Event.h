#pragma once
#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>

#include "utils/TesseraLog.h"

namespace tessera
{
	enum class EventType : uint8_t
	{
		EVENT_APPLICATION_QUIT = 0x01,

		EVENT_KEY_PRESSED = 0x02,

		EVENT_KEY_RELEASED = 0x03,

		EVENT_MOUSE_BUTTON_PRESSED = 0x04,

		EVENT_MOUSE_BUTTON_RELEASED = 0x05,

		EVENT_MOUSE_MOVED = 0x06,

		EVENT_MOUSE_WHEEL = 0x07,

		EVENT_WINDOW_RESIZED = 0x08,

	};

	class EventSystem final
	{
	private:
		class EventHandlerBase
		{
		public:
			virtual ~EventHandlerBase() = default;
			virtual void trigger(const std::vector<std::any>& args) = 0;
			[[nodiscard]] virtual std::vector<std::type_index> getParamTypes() const = 0;
		};

		template <typename ...Args>
		class EventHandler final : public EventHandlerBase
		{
		public:
			void addCallback(std::function<void(Args...)> callback)
			{
				callbacks.emplace_back(std::move(callback));
			}
			
			void trigger(const std::vector<std::any>& args) override
			{
				ASSERT(args.size() == sizeof...(Args), "Argument count mismatch.");

				auto extractedArguments = extractArgs(args, std::index_sequence_for<Args...>{});

				for (const auto& callback : callbacks)
				{
					std::apply(callback, extractedArguments);
				}
			}
			
			[[nodiscard]] std::vector<std::type_index> getParamTypes() const override
			{
				return {typeid(Args)...};
			}
			
		private:
			template <size_t... Is>
			std::tuple<Args...> extractArgs(const std::vector<std::any>& args, std::index_sequence<Is...>)
			{
				return { std::any_cast<std::tuple_element_t<Is, std::tuple<Args...>>>(args[Is])... };
			}
			
			std::vector<std::function<void(Args...)>> callbacks;
		};
		
		std::unordered_map<EventType, std::unique_ptr<EventHandlerBase>> handlers;
	public:
		template <typename ...Args>
		void registerEvent(const EventType eventType, std::function<void(Args...)> callback)
		{
			if (const auto& eventHandler = handlers.find(eventType); eventHandler != handlers.end())
			{
				auto* existingEventHandler = dynamic_cast<EventHandler<Args...>*>(eventHandler->second.get());
				ASSERT(existingEventHandler, "Type mismatch for event.");
				existingEventHandler->addCallback(std::move(callback));
			}
			else
			{
				auto newEventHandler = std::make_unique<EventHandler<Args...>>();
				newEventHandler->addCallback(std::move(callback));
				handlers[eventType] = std::move(newEventHandler);
			}
		}

		template <typename ...Args>
		void fireEvent(const EventType eventType, Args... args)
		{
			const auto eventHandler = handlers.find(eventType);
			if (eventHandler == handlers.end())
			{
				return;
			}

			const std::vector<std::type_index> expected = eventHandler->second->getParamTypes();
			const std::vector<std::type_index> actual = {typeid(args)...};

			ASSERT(expected == actual, "Type mismatch for firing event.");

			std::vector<std::any> anyArgs;
			(anyArgs.emplace_back(args), ...);
			eventHandler->second->trigger(anyArgs);
		}
	};
	
	template<typename T>
	struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

	template<typename C, typename R, typename... Args>
	struct FunctionTraits<R(C::*)(Args...) const> 
	{
		using ArgsTuple = std::tuple<Args...>;
	};

	template<typename F>
	void registerHelper(EventSystem& es, EventType type, F&& f) 
	{
		auto func = std::function(std::forward<F>(f));
		es.registerEvent(type, func);
	}
	
}


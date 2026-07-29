#pragma once
#include <functional>
#include <string>
#include <vector>

namespace parus
{

	/** One named, type-erased property: read renders it to text, write parses tokens into it. */
	struct PropertyAccessor final
	{
		std::string name;
		std::string typeHint;
		std::function<std::string(const void*)> read;
		std::function<bool(void*, const std::vector<std::string>&, std::string&)> write;
	};

	/** The set of properties exposed by one target type (an entity, a component, or the camera). */
	struct TargetSchema final
	{
		std::string name;
		std::vector<PropertyAccessor> properties;

		/** Returns nullptr if no property with that name exists. */
		[[nodiscard]] const PropertyAccessor* findProperty(const std::string& propertyName) const;
	};

	/** Holds every registered TargetSchema, keyed by schema name. */
	class PropertyRegistry final
	{
	public:
		void registerSchema(TargetSchema schema);
		/** Returns nullptr if no schema with that name exists. */
		[[nodiscard]] const TargetSchema* findSchema(const std::string& schemaName) const;

	private:
		std::vector<TargetSchema> schemas;
	};

}

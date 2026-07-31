#include "PropertyRegistry.h"

namespace parus
{
	const PropertyAccessor* TargetSchema::findProperty(const std::string& propertyName) const
	{
		for (const PropertyAccessor& accessor : properties)
		{
			if (accessor.name == propertyName)
			{
				return &accessor;
			}
		}

		return nullptr;
	}

	void PropertyRegistry::registerSchema(TargetSchema schema)
	{
		schemas.push_back(std::move(schema));
	}

	const TargetSchema* PropertyRegistry::findSchema(const std::string& schemaName) const
	{
		for (const TargetSchema& schema : schemas)
		{
			if (schema.name == schemaName)
			{
				return &schema;
			}
		}

		return nullptr;
	}
}

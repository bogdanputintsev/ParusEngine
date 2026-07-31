#include <gtest/gtest.h>

#include "services/console/reflection/PropertyRegistry.h"

namespace parus
{
	namespace
	{
		struct Dummy { float value = 0.0f; };
	}

	TEST(PropertyRegistry, ReadAndWriteThroughAccessor)
	{
		TargetSchema schema;
		schema.name = "Dummy";
		schema.properties.push_back(PropertyAccessor{
			"value", "float",
			[](const void* object) { return std::to_string(static_cast<const Dummy*>(object)->value); },
			[](void* object, const std::vector<std::string>& args, std::string& error)
			{
				if (args.empty())
				{
					error = "need a value";
					return false;
				}
				static_cast<Dummy*>(object)->value = std::stof(args[0]);
				return true;
			}
		});

		PropertyRegistry registry;
		registry.registerSchema(schema);

		const TargetSchema* found = registry.findSchema("Dummy");
		ASSERT_NE(found, nullptr);

		const PropertyAccessor* accessor = found->findProperty("value");
		ASSERT_NE(accessor, nullptr);

		Dummy dummy;
		std::string error;
		EXPECT_TRUE(accessor->write(&dummy, { "42" }, error));
		EXPECT_FLOAT_EQ(dummy.value, 42.0f);
		EXPECT_EQ(accessor->read(&dummy), std::to_string(42.0f));
	}

	TEST(PropertyRegistry, FindReturnsNullForUnknown)
	{
		PropertyRegistry registry;
		EXPECT_EQ(registry.findSchema("Nope"), nullptr);
	}
}

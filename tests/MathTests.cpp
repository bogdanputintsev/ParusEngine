#include <gtest/gtest.h>

#include "engine/utils/math/Math.h"

namespace parus
{
    TEST(HarnessSanity, TrueIsTrue)
    {
        EXPECT_TRUE(true);
    }
}

namespace parus::math
{
    TEST(Vector3Length, ComputesEuclideanLength)
    {
        const Vector3 vector{ 3.0f, 4.0f, 0.0f };

        EXPECT_FLOAT_EQ(vector.length(), 5.0f);
    }

    TEST(Vector3Length, ZeroVectorHasZeroLength)
    {
        const Vector3 zero{ 0.0f, 0.0f, 0.0f };

        EXPECT_FLOAT_EQ(zero.length(), 0.0f);
    }

    TEST(Vector3Normalize, ProducesUnitLengthVector)
    {
        const Vector3 normalized = Vector3{ 3.0f, 4.0f, 0.0f }.normalize();

        EXPECT_FLOAT_EQ(normalized.length(), 1.0f);
    }

    TEST(Vector3Normalize, ZeroVectorStaysZero)
    {
        const Vector3 normalized = Vector3{ 0.0f, 0.0f, 0.0f }.normalize();

        EXPECT_EQ(normalized, (Vector3{ 0.0f, 0.0f, 0.0f }));
    }
}

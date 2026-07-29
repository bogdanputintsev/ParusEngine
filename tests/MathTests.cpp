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

    TEST(Matrix4x4Transform, ScaleScalesAPoint)
    {
        const math::Matrix4x4 matrix = math::Matrix4x4::scale(2.0f, 3.0f, 4.0f);
        const math::Vector3 result = matrix.transformPoint({ 1.0f, 1.0f, 1.0f });

        EXPECT_NEAR(result.x, 2.0f, 1e-4f);
        EXPECT_NEAR(result.y, 3.0f, 1e-4f);
        EXPECT_NEAR(result.z, 4.0f, 1e-4f);
    }

    TEST(Matrix4x4Transform, IdentityRotationLeavesPointUnchanged)
    {
        const math::Matrix4x4 matrix = math::Matrix4x4::rotation(0.0f, 0.0f, 0.0f);
        const math::Vector3 result = matrix.transformPoint({ 1.0f, 2.0f, 3.0f });

        EXPECT_NEAR(result.x, 1.0f, 1e-4f);
        EXPECT_NEAR(result.y, 2.0f, 1e-4f);
        EXPECT_NEAR(result.z, 3.0f, 1e-4f);
    }

    TEST(Matrix4x4Transform, YawNinetyDegreesSendsPlusXToMinusZ)
    {
        const math::Matrix4x4 matrix = math::Matrix4x4::rotation(0.0f, 90.0f, 0.0f);
        const math::Vector3 result = matrix.transformPoint({ 1.0f, 0.0f, 0.0f });

        EXPECT_NEAR(result.x,  0.0f, 1e-4f);
        EXPECT_NEAR(result.y,  0.0f, 1e-4f);
        EXPECT_NEAR(result.z, -1.0f, 1e-4f);
    }

    TEST(Matrix4x4Transform, TransformPointAppliesTranslation)
    {
        const math::Matrix4x4 matrix = math::Matrix4x4::translation(5.0f, 6.0f, 7.0f);
        const math::Vector3 result = matrix.transformPoint({ 1.0f, 1.0f, 1.0f });

        EXPECT_NEAR(result.x, 6.0f, 1e-4f);
        EXPECT_NEAR(result.y, 7.0f, 1e-4f);
        EXPECT_NEAR(result.z, 8.0f, 1e-4f);
    }
}

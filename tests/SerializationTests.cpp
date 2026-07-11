#include <gtest/gtest.h>

#include <sstream>

#include "services/serialization/BinaryStream.h"

namespace parus::serialization
{
    TEST(BinaryStreamRoundTrip, UInt32)
    {
        std::stringstream stream;
        writeUInt32(stream, 123456u);

        EXPECT_EQ(readUInt32(stream), 123456u);
    }

    TEST(BinaryStreamRoundTrip, UInt64)
    {
        std::stringstream stream;
        writeUInt64(stream, 9876543210ull);

        EXPECT_EQ(readUInt64(stream), 9876543210ull);
    }

    TEST(BinaryStreamRoundTrip, Float)
    {
        std::stringstream stream;
        writeFloat(stream, 3.14159f);

        EXPECT_FLOAT_EQ(readFloat(stream), 3.14159f);
    }

    TEST(BinaryStreamRoundTrip, String)
    {
        std::stringstream stream;
        writeString(stream, "hello world");

        EXPECT_EQ(readString(stream), "hello world");
    }

    TEST(BinaryStreamRoundTrip, EmptyString)
    {
        std::stringstream stream;
        writeString(stream, "");

        EXPECT_EQ(readString(stream), "");
    }

    TEST(BinaryStreamRoundTrip, Vector3)
    {
        const math::Vector3 original{ 1.5f, -2.25f, 100.0f };

        std::stringstream stream;
        writeVector3(stream, original);

        EXPECT_EQ(readVector3(stream), original);
    }

    TEST(BinaryStreamRoundTrip, Matrix4x4)
    {
        const math::Matrix4x4 original = math::Matrix4x4::translation(3.0f, 4.0f, 5.0f);

        std::stringstream stream;
        writeMatrix4x4(stream, original);

        EXPECT_EQ(readMatrix4x4(stream), original);
    }

    // Several values written back-to-back must read out in the same order,
    // proving the stream cursor advances correctly across mixed types.
    TEST(BinaryStreamRoundTrip, SequentialMixedValues)
    {
        std::stringstream stream;
        writeUInt32(stream, 42u);
        writeString(stream, "parus");
        writeFloat(stream, 2.0f);

        EXPECT_EQ(readUInt32(stream), 42u);
        EXPECT_EQ(readString(stream), "parus");
        EXPECT_FLOAT_EQ(readFloat(stream), 2.0f);
    }
}
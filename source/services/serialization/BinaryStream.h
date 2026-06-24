#pragma once
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

#include "engine/utils/math/Math.h"

namespace parus::serialization
{

    inline void writeUInt8(std::ostream& stream, const uint8_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    inline void writeUInt32(std::ostream& stream, const uint32_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    inline void writeUInt64(std::ostream& stream, const uint64_t value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    inline void writeFloat(std::ostream& stream, const float value)
    {
        stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    inline void writeBytes(std::ostream& stream, const void* data, const size_t size)
    {
        stream.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    inline void writeString(std::ostream& stream, const std::string& value)
    {
        writeUInt32(stream, static_cast<uint32_t>(value.size()));
        stream.write(value.data(), static_cast<std::streamsize>(value.size()));
    }

    inline void writeVector3(std::ostream& stream, const math::Vector3& vector)
    {
        writeFloat(stream, vector.x);
        writeFloat(stream, vector.y);
        writeFloat(stream, vector.z);
    }

    inline void writeMatrix4x4(std::ostream& stream, const math::Matrix4x4& matrix)
    {
        const math::TrivialMatrix4x4 trivialMatrix = matrix.trivial();
        writeBytes(stream, &trivialMatrix, sizeof(trivialMatrix));
    }

    inline void readBytes(std::istream& stream, void* data, const size_t size)
    {
        stream.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
    }

    inline uint8_t readUInt8(std::istream& stream)
    {
        uint8_t value = 0;
        readBytes(stream, &value, sizeof(value));

        return value;
    }

    inline uint32_t readUInt32(std::istream& stream)
    {
        uint32_t value = 0;
        readBytes(stream, &value, sizeof(value));

        return value;
    }

    inline uint64_t readUInt64(std::istream& stream)
    {
        uint64_t value = 0;
        readBytes(stream, &value, sizeof(value));

        return value;
    }

    inline float readFloat(std::istream& stream)
    {
        float value = 0.0f;
        readBytes(stream, &value, sizeof(value));

        return value;
    }

    inline std::string readString(std::istream& stream)
    {
        const uint32_t length = readUInt32(stream);
        std::string value(length, '\0');
        stream.read(value.data(), static_cast<std::streamsize>(length));

        return value;
    }

    inline math::Vector3 readVector3(std::istream& stream)
    {
        const float x = readFloat(stream);
        const float y = readFloat(stream);
        const float z = readFloat(stream);

        return math::Vector3(x, y, z);
    }

    inline math::Matrix4x4 readMatrix4x4(std::istream& stream)
    {
        math::TrivialMatrix4x4 trivial{};
        readBytes(stream, &trivial, sizeof(trivial));

        std::array<std::array<float, 4>, 4> values{};
        for (size_t row = 0; row < 4; ++row)
        {
            for (size_t col = 0; col < 4; ++col)
            {
                values[row][col] = trivial.values[row][col];
            }
        }

        return math::Matrix4x4(values);
    }

}

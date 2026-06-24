#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>

namespace parus::serialization
{

    inline constexpr uint32_t FORMAT_VERSION = 1;
    inline constexpr std::array<char, 4> MAGIC_PWORLD = { 'P', 'W', 'L', 'D' };
    inline constexpr std::array<char, 4> MAGIC_PMESH  = { 'P', 'M', 'S', 'H' };
    inline constexpr std::array<char, 4> MAGIC_PTEX   = { 'P', 'T', 'E', 'X' };

#pragma pack(push, 1)
    /** 56-byte common header shared by all three format types. */
    struct FormatHeader
    {
        std::array<char, 4>     magic;
        uint32_t                version         = FORMAT_VERSION;
        uint32_t                flags           = 0;
        std::array<uint8_t, 32> contentHash     = {};
        uint64_t                payloadSize     = 0;
        uint32_t                pipelineProfile = 0;
    };
#pragma pack(pop)

    static_assert(sizeof(FormatHeader) == 56, "FormatHeader must be exactly 56 bytes");

    void writeHeader(std::ostream& stream, const FormatHeader& header);

}

#include "FormatHeader.h"

#include "BinaryStream.h"

namespace parus::serialization
{

    void writeHeader(std::ostream& stream, const FormatHeader& header)
    {
        writeBytes(stream, header.magic.data(), header.magic.size());
        writeUInt32(stream, header.version);
        writeUInt32(stream, header.flags);
        writeBytes(stream, header.contentHash.data(), header.contentHash.size());
        writeUInt64(stream, header.payloadSize);
        writeUInt32(stream, header.pipelineProfile);
    }

}

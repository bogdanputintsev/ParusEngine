#include "ShaderLoader.h"

#include <fstream>
#include <stdexcept>

namespace tessera
{
	
	std::vector<char> ShaderLoader::readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) 
		{
			throw std::runtime_error("failed to open file!");
		}

		const auto fileSize = static_cast<std::streamsize>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

}

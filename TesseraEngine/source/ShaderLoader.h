#pragma once
#include <string>
#include <vector>

namespace tessera
{
	
	class ShaderLoader
	{
	public:
		static std::vector<char> readFile(const std::string& filename);
	};

}


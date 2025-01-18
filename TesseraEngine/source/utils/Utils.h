#pragma once
#include <chrono>
#include <random>
#include <string>
#include <fstream>


namespace tessera
{

	inline std::string generateHash()
	{
		// Get current time in nanoseconds
		const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
		const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

		// Generate a random number
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<unsigned long long> dis(0, std::numeric_limits<unsigned long long>::max());
		const auto randomNumber = dis(gen);

		// Combine datetime and random number to generate hash
		std::ostringstream oss;
		oss << std::hex << std::setfill('0') << std::setw(16) << nanoseconds << std::setw(16) << randomNumber;

		return oss.str();
	}

	inline std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		ASSERT(file.is_open(), "failed to open file " + filename);

		const auto fileSize = static_cast<std::streamsize>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	inline bool approximatelyEqual(const float a, const float b)
	{
		return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * std::numeric_limits<float>::epsilon());
	}

}

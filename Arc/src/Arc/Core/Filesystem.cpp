#include "arcpch.h"
#include "Filesystem.h"

#include <fstream>

namespace ArcEngine
{
	Buffer Filesystem::ReadFileBinary(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

		if (!stream)
			return {};

		std::streampos end = stream.tellg();
		stream.seekg(0, std::ios::beg);
		uint64_t size = end - stream.tellg();

		if (size == 0)
			return {};

		Buffer buffer(size);
		stream.read(buffer.As<char>(), (std::streamsize)size);
		stream.close();
		return buffer;
	}

	void Filesystem::ReadFileText(const std::filesystem::path& filepath, eastl::string& outString)
	{
		ARC_PROFILE_SCOPE();

		std::ifstream in(filepath.c_str(), std::ios::in | std::ios::binary); // ifstream closes itself due to RAII
		if (in)
		{
			in.seekg(0, std::ios::end);
			const size_t size = in.tellg();
			if (size != -1)
			{
				outString.resize(size);
				in.seekg(0, std::ios::beg);
				in.read(&outString[0], size);
			}
			else
			{
				ARC_CORE_ERROR("Could not read from file '{0}'", filepath);
			}
		}
		else
		{
			ARC_CORE_ERROR("Could not open file '{0}'", filepath);
		}
	}

	void Filesystem::WriteFileText(const std::filesystem::path& filepath, const eastl::string& buffer)
	{
		std::ofstream stream(filepath, std::ios::out);
		stream << buffer.c_str();
		stream.close();
	}
}
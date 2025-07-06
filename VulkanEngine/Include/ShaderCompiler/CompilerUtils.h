#pragma once
#include "../Core/Config.h"

VK_BEGIN

inline bool ReadFile(const std::string& filepath, std::string& buffer)
{
	std::fstream file(filepath);

	if (!file) return false;

	std::stringstream stream;
	stream << file.rdbuf();

	buffer += stream.str();

	file.close();

	return true;
}

inline bool WriteFile(const std::string& filepath, const std::string& buffer)
{
	std::fstream file(filepath);

	if (!file) return false;

	file.clear();
	file.write(buffer.c_str(), buffer.size());

	file.close();

	return true;
}

VK_END

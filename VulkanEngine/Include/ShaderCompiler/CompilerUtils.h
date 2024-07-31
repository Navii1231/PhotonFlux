#pragma once
#include "../Core/Config.h"

#include <fstream>
#include <sstream>

VK_BEGIN

inline bool ReadFile(const std::string& filepath, std::string& buffer)
{
	std::fstream file(filepath);

	if (!file) return false;

	std::stringstream stream;
	stream << file.rdbuf();

	buffer += stream.str();

	return true;
}

VK_END

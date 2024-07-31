#pragma once
#include <fstream>

#include "../Application/Application.h"

#define PH_FLUX_NAMESPACE     PhFlux

#define PH_BEGIN  namespace PH_FLUX_NAMESPACE {
#define PH_END    }

inline bool WriteFile(const std::string& filepath, const std::string& contents)
{
	std::fstream stream(filepath);

	if (!stream)
		return false;

	stream.clear();

	stream.write(contents.c_str(), contents.size());

	stream.close();

	return true;
}

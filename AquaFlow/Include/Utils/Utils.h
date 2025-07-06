#pragma once
#include "../Core/AqCore.h"

AQUA_BEGIN

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

AQUA_END

// Can be used to print glm vectors in global namespace
template <typename T, int N>
std::ostream& operator <<(std::ostream& stream, const glm::vec<N, T>& vec)
{
	for (int i = 0; i < N; i++)
	{
		stream << vec[i] << " ";
	}

	return stream;
}


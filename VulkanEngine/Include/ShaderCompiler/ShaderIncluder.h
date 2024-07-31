#pragma once
#include "CompilerUtils.h"
#include "ShaderConfig.h"

#include <filesystem>

VK_BEGIN

struct IncludeResult
{
	std::string HeaderName;
	std::string Contents;

	IncludeResult() = default;
	IncludeResult(const std::string& headerName, const std::string& contents)
		: HeaderName(headerName), Contents(contents) {}
};

class ShaderIncluder
{
public:
	ShaderIncluder(const std::filesystem::path& shaderDirectory,
		const std::vector<std::filesystem::path>& Paths)
		: mShaderDirectory(shaderDirectory), mSystemPaths(Paths) {}

	IncludeResult* IncludeSystem(const std::string& headerName,
		const std::string& includerName, size_t recursionDepth);

	IncludeResult* IncludeLocal(const std::string& headerName,
		const std::string& includerName, size_t inclusionDepth);

	void ReleaseInclude(IncludeResult* result);

private:
	std::filesystem::path mShaderDirectory;
	std::vector<std::filesystem::path> mSystemPaths;

	std::vector<std::filesystem::path> mRecursionStack;

private:
	std::filesystem::path ResolveLocalPath(const std::filesystem::path& Header, const std::filesystem::path& IncludePath);
};

VK_END

#pragma once
#include "../Core/AqCore.h"

#include <string>

AQUA_BEGIN

class CompileErrorChecker
{
public:
	CompileErrorChecker(const std::string& filepath)
		: mFilepath(filepath) {}

	std::string GetError(const vkEngine::CompileError& error) const
	{
		std::string ErrorInfo{};

		if (error.Type != vkEngine::ErrorType::eNone)
		{
			ErrorInfo = "Couldn't Compile Code\n";
			ErrorInfo += error.Info;

			vkEngine::WriteFile(mFilepath, error.SrcCode);
		}

		return ErrorInfo;
	}

	std::vector<std::string> GetErrors(const std::vector<vkEngine::CompileError>& errors) const
	{
		std::vector<std::string> Errors;

		for (const auto& error : errors)
			Errors.push_back(GetError(error));

		return Errors;
	}

	void AssertOnError(const std::vector<std::string>& errorValues) const
	{
		for (const auto& errorValue : errorValues)
		{
			if (errorValue.empty())
				continue;

			std::cout << errorValue << std::endl;
			_STL_ASSERT(errorValue.empty(), "Couldn't compile shader code!");
		}
	}

private:
	std::string mFilepath;
};

AQUA_END

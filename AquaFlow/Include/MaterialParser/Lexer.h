#pragma once
#include "../Core/AqCore.h"

#include <vector>
#include <string>
#include <string_view>

AQUA_BEGIN

struct Token
{
	std::string_view Lexeme;
	int LineNo = -1;
	int CharOff = -1;

	size_t PosInStr = static_cast<size_t>(-1);
};

class Lexer
{
public:
	Lexer() = default;

	Lexer(const Lexer&) = delete;
	Lexer& operator =(const Lexer&) = delete;

	Token Advance();
	Token Peek(size_t off = 1);

	void SetMarker();
	void RetrieveMarker();

	void Reset();

	// NOTE: The string must not be destroyed while the class ::AQUA_NAMESPACE::Lexer is using it
	void SetString(const std::string_view& source);
	void SetWhiteSpacesAndDelimiters(const std::string& spaces, const std::string& delimiters);

	const Token& GetCurrent() const { return mCurrent; }
	bool HasConsumed() const { return mPosInStr >= mSource.size(); }

	std::vector<Token> Lex();
	std::vector<Token> LexString(const std::string_view& view);

private:
	std::string_view mSource;
	Token mCurrent;
	Token mMarkerToken;

	size_t mPosInStr = 0;

	size_t mLineNumber = 0;
	size_t mCharOffset = 0;

	std::string mDelimiters;
	std::string mWhiteSpaces;

private:

	void SkipWhiteSpaces();
	Token ReadUntilDelimiterHits();
	bool IsInString(char val, const std::string& str);

	char Increment();
	void UpdateState(char Current);
};

AQUA_END


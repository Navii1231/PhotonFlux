#include "Lexer.h"

void AQUA_NAMESPACE::Lexer::SetString(const std::string_view& source)
{
	mSource = source;
}

AQUA_NAMESPACE::Token AQUA_NAMESPACE::Lexer::Advance()
{
	if (HasConsumed())
		return {};

	mCurrent = Peek(1);
	return mCurrent;
}

AQUA_NAMESPACE::Token AQUA_NAMESPACE::Lexer::Peek(size_t off)
{
	Token curr = mCurrent;

	while (off != 0)
	{
		SkipWhiteSpaces();
		curr = ReadUntilDelimiterHits();
		off--;
	}

	return curr;
}

void AQUA_NAMESPACE::Lexer::SetMarker()
{
	mMarkerToken = mCurrent;
}

void AQUA_NAMESPACE::Lexer::RetrieveMarker()
{
	mCurrent = mMarkerToken;
	mCharOffset = mCurrent.CharOff;
	mLineNumber = mCurrent.LineNo;
	mPosInStr = mCurrent.PosInStr;
}

void AQUA_NAMESPACE::Lexer::Reset()
{
	mPosInStr = 0;
	mLineNumber = 0;
	mCharOffset = 0;
}

void AQUA_NAMESPACE::Lexer::SetWhiteSpacesAndDelimiters(const std::string& spaces, const std::string& delimiters)
{
	mWhiteSpaces = spaces;
	mDelimiters = delimiters;

	mDelimiters.append(mWhiteSpaces);
}

std::vector<AQUA_NAMESPACE::Token> AQUA_NAMESPACE::Lexer::Lex()
{
	std::vector<Token> tokens;

	Reset();

	while (!HasConsumed())
	{
		tokens.push_back(Advance());
	}

	return tokens;
}

std::vector<AQUA_NAMESPACE::Token> AQUA_NAMESPACE::Lexer::LexString(const std::string_view& view)
{
	// Create a new token to store new string
	Lexer lexer{};
	lexer.SetWhiteSpacesAndDelimiters(mWhiteSpaces, mDelimiters);
	lexer.SetString(view);

	return lexer.Lex();
}

void AQUA_NAMESPACE::Lexer::SkipWhiteSpaces()
{
	_STL_ASSERT(!mSource.empty(), "The Lexer string was empty!");

	const char* Src = mSource.begin()._Unwrapped();
	char Current = Src[mPosInStr];

	while (IsInString(Current, mWhiteSpaces) && Current != 0)
	{
		Current = Src[++mPosInStr];
		UpdateState(Current);
	}
}

AQUA_NAMESPACE::Token AQUA_NAMESPACE::Lexer::ReadUntilDelimiterHits()
{
	_STL_ASSERT(!mSource.empty(), "The Lexer string was empty!");

	Token next;
	next.LineNo = static_cast<int>(mLineNumber);
	next.CharOff = static_cast<int>(mCharOffset);
	next.PosInStr = mPosInStr;

	char Current = mSource[mPosInStr++];

	// Checking if we are directly hitting a delimiter
	if (IsInString(Current, mDelimiters) && Current != '\0')
	{
		next.Lexeme = mSource.substr(next.PosInStr, 1);
		return next;
	}

	Current = mSource[mPosInStr];

	// Scam till we don't run into a delimiter, but don't include it
	while (!IsInString(Current, mDelimiters) && Current != '\0')
		Current = Increment();

	next.Lexeme = mSource.substr(next.PosInStr, mPosInStr - next.PosInStr);

	return next;
}

bool AQUA_NAMESPACE::Lexer::IsInString(char val, const std::string& str)
{
	auto found = std::find(str.begin(), str.end(), val);
	return found != str.end();
}

char AQUA_NAMESPACE::Lexer::Increment()
{
	char Current = mSource[++mPosInStr];

	if (Current == '\n')
	{
		mLineNumber++;
		mCharOffset = 0;
		return Current;
	}

	mCharOffset++;

	return Current;
}

void AQUA_NAMESPACE::Lexer::UpdateState(char Current)
{
	if (Current == '\n')
	{
		mLineNumber++;
		mCharOffset = 0;
		return;
	}

	mCharOffset++;
}

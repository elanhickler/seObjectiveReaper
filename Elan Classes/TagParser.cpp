#include "TagParser.h"

void StringIterator2::remember()
{
	if (!p1.isEmpty()) memory.push_back({ p1, p2 });
}

void StringIterator2::rememberAndSkipOver()
{
	remember();
	skip();
	start();
}

String StringIterator2::getMemory()
{
	StringArray str;
	vector<String> strVec;

	for (const auto& mem : memory)
	{
		str.add({ mem.a, mem.b });
		strVec.push_back({ mem.a, mem.b });
	}

	memory.clear();

	return str.joinIntoString("");
}

void StringIterator2::restart()
{
	pos = 0;
	len = in_str.length();
	p1 = in_str.getCharPointer();
	p2 = p1;
}

void StringIterator2::skipNewLines()
{
	while (!atEnd() && CHAr::isNewLine(*this)) move();
}

void StringIterator2::skipToNewLine()
{
	while (!atEnd() && !CHAr::isNewLine(*this)) move();
}

void StringIterator2::skipToChar(const juce_wchar& c)
{
	while (!atEnd() && *this != c) move();
}

void StringIterator2::skipToChar(const String & s)
{
	while (!atEnd() && !CHAr::isAnyOf(*this, s)) move();
}

void StringIterator2::skipPastChar(const juce_wchar & c)
{
	skipToChar(c);
	while (!atEnd() && *this == c) move();
}

void StringIterator2::skipToSpace()
{
	while (!atEnd() && !CHAr::isWhiteSpace(*this)) move();
}

void StringIterator2::skipToSpaceOrChar(const juce_wchar & c)
{
	while (!atEnd() && *this != c && !CHAr::isWhiteSpace(*this)) move();
}

void StringIterator2::skipSpaces()
{
	while (!atEnd() && CHAr::isWhiteSpace(*this)) move();
}

void StringIterator2::skipChar(const juce_wchar & c)
{
	while (!atEnd() && *p2 == c) move();
}

void StringIterator2::skipChar(const String & s)
{
	while (!atEnd() && CHAr::isAnyOf(*this, s)) move();
}

String StringIterator2::consumeToNewLine()
{
	start();
	skipToNewLine();
	return get();
}

String StringIterator2::consumeToSpace()
{
	start();
	skipToSpace();
	return get();
}

String StringIterator2::consumeToNonSpace()
{
	start();
	skipSpaces();
	return get();
}

String StringIterator2::consumeInt()
{
	start();
	enum MODE { begin, end };
	MODE mode = begin;
	while (!this->atEnd())
	{
		switch (mode)
		{
		case begin:
			if (!isAnyOf("-+") || !CHAr::isDigit(*this)) return get();
			move();
			mode = end;
			continue;
		case end:
			if (!CHAr::isDigit(*this)) return get();
			move();
			continue;
		}
	}
	return get();
}

String StringIterator2::consumeToChar(const juce_wchar & c)
{
	start();
	while (!atEnd() && *this != c) move();
	return get();
}

String StringIterator2::consumeToChar(const String & s)
{
	start();
	while (!atEnd() && !isAnyOf(s)) move();
	return get();
}

String StringIterator2::consumePastChar(const String & s)
{
	start();
	while (!atEnd() && !isAnyOf(s)) move();
	while (!atEnd() && isAnyOf(s)) move();
	return get();
}

String StringIterator2::consumeToSpaceOrChar(const juce_wchar & c)
{
	start();
	skipToSpaceOrChar(c);
	return get();
}

String StringIterator2::consumeToEnd(int stop_early)
{
	start();
	while (!atEnd(stop_early)) move();
	return get();
}

String StringIterator2::consumeToNewLineTRIM()
{
	skipSpaces();
	auto p = p1 = p2;
	while (!atEnd() && !CHAr::isNewLine(*this))
	{
		skipToSpace();
		p = p2;
		skipChar(' ');
		if (CHAr::isNewLine(*this)) break;
	}
	return String(p1, p);
}

String StringIterator2::consumeToCharTRIM(const juce_wchar & c)
{
	skipSpaces();
	auto p = p1 = p2;
	while (!atEnd() && *this != c)
	{
		skipToSpaceOrChar(c);
		p = p2;
		skipSpaces();
		if (*this == c) break;
	}
	return String(p1, p);
}

String StringIterator2::consumeToNewlineOrCharTRIM(const juce_wchar & c)
{
	skipSpaces();
	auto p = p1 = p2;
	while (!atEnd() && *this != c)
	{
		skipToSpaceOrChar(c);
		p = p2;
		skipChar(' ');
		if (*this == c || CHAr::isNewLine(*this)) break;
	}
	return String(p1, p);
}

String StringIterator2::consumeToChar_IgnoreEscapes(const String & s, const juce_wchar & escape)
{
	start();

	while (!atEnd() && !CHAr::isAnyOf(*this, s))
	{
		if (*this == escape)
		{
			rememberAndSkipOver();
			skip();
			continue;
		}

		move();
	}

	remember();

	return getMemory();
}

TagParser::TagParser(const String & in_str)
{
	iter = in_str;

	auto p = iter.p1;

	bool inside_bracket = false;

	while (!iter.atEnd())
	{
		switch (mode)
		{
		case Begin:
			prefix = iter.consumeToChar_IgnoreEscapes("<[", '\\');
			switch ((char)iter)
			{
			case '<':
				iter.move();
				mode = InsideExpression; continue;

			case '[':
				if (prefix.isNotEmpty())
				{
					tags.push_back({ "", prefix, "", false, false });
					prefix = "";
				}
				iter.move();
				mode = InsideBracket;
				continue;
			}
			continue;

		case InsideExpression:
			property_name = iter.consumeToChar_IgnoreEscapes("#>", '\\');

			if (iter == '#')
				use_value = true;

			iter.skipPastChar('>');

			if (inside_bracket)
			{
				suffix = iter.consumeToChar_IgnoreEscapes("]", '\\');
				iter.skip();
				inside_bracket = false;
			}

			mode = SaveResult;
			continue;

		case InsideBracket:
			inside_bracket = is_optional = true;

			prefix = iter.consumeToChar_IgnoreEscapes("<", '\\');
			iter.skip();
			mode = InsideExpression;
			continue;


		//case InsideBracket:
		//	inside_bracket = is_optional = true;
		//	prefix = iter.consumeToChar_IgnoreEscapes("<]", '\\');
		//	iter.skip();
		//	mode = prefix.getLastCharacter() == '<' ? InsideExpression : SaveResult;
		//	continue;

		case SaveResult:
			tags.push_back({ property_name, prefix, suffix, use_value, is_optional });
			property_name = "";
			use_value = false;
			mode = Begin;
			continue;
		}
	}
	if (mode == SaveResult || prefix.isNotEmpty()) tags.push_back({ property_name, prefix, suffix, use_value, is_optional });
}

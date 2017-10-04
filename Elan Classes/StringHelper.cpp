#include "StringHelper.h"

using std::min;
using std::max;

String toS(const juce_wchar& c)
{
	return String::charToString(c);
}
String toS(int i)
{
	return String(i);
}

bool CHAr::is(const juce_wchar& c1, const juce_wchar& c2)
{
	return CharacterFunctions::compare(c1, c2) == 0;
}
bool CHAr::is_i(const juce_wchar& c1, const juce_wchar& c2)
{
	return CharacterFunctions::compareIgnoreCase(c1, c2) == 0;
}
bool CHAr::isDigit(const juce_wchar& c)
{
	return CharacterFunctions::isDigit(c);
}
bool CHAr::isWhiteSpace(const juce_wchar& c)
{
    return CharacterFunctions::isWhitespace(c);
}
bool CHAr::isNewLine(const juce_wchar& c)
{
    return CHAr::isAnyOf(c, "\r\n");
}
bool CHAr::isAnyOf(const juce_wchar& c, const String& list)
{
	return list.containsChar(c);
}
bool CHAr::isAnyOf_i(const juce_wchar& c, const String& list)
{
	for (auto p = list.getCharPointer(); !p.isEmpty(); ++p) 
		if (CHAr::is_i(c, *p)) return true;
	return false;
}
bool CHAr::isUpper(const juce_wchar& c)
{
	return CharacterFunctions::isUpperCase(c);
}
bool CHAr::isLower(const juce_wchar& c)
{
	return CharacterFunctions::isLowerCase(c);
}
juce_wchar CHAr::toUpper(const juce_wchar& c)
{
	return CharacterFunctions::toUpperCase(c);
}
juce_wchar CHAr::toLower(const juce_wchar& c)
{
	return CharacterFunctions::toLowerCase(c);
}

bool STR::isInt(const String& s)
{
	if (s.isEmpty()) return false;
	auto p = s.getCharPointer();
	if (*p == '-' || *p == '+') ++p;
	while (!p.isEmpty() && CHAr::isDigit(*p)) ++p;
	return p.isEmpty();
}

String STR::toUpper(const String & s)
{
	return s.toUpperCase();
}

String STR::toLower(const String & s)
{
	return s.toLowerCase();
}

vector<String> STR::split(const String & s, const juce_wchar & delim)
{
    StringIterator iter(s);
    vector<String> tokens;

    while (!iter.atEnd())
    {
        tokens.push_back(iter.consumeToChar(delim));
        iter.skip();
    }

    return tokens;
}

vector<String> STR::split(const String & s, const String & delim)
{
    StringIterator iter(s);
    vector<String> tokens;

    while (!iter.atEnd())
    {
        tokens.push_back(iter.consumeToChar(delim));
        iter.skip();
    }

    return tokens;
}

vector<String> STR::splitPast(const String & s, const String & delim)
{
  StringIterator iter(s);
  vector<String> tokens;

  while (!iter.atEnd())
    tokens.push_back(iter.consumePastChar(delim));

  return tokens;
}

// split by any delimiter
vector<String> STR::split(const String & s, char delim, size_t min_size, String defval)
{
    vector<String> elems = split(s, delim);
    elems.resize(max(elems.size(), min_size), defval);
    return elems;
}


void StringIterator::remember()
{
  if (!p1.isEmpty()) memory.push_back({ p1, p2 });
}

void StringIterator::rememberAndSkipOver()
{
    remember();
    skip();
    start();
}

String StringIterator::getMemory()
{
    String str;

    for (const auto& mem : memory)
        str += String(mem.a, mem.b);

    memory.clear();

    return std::move(str);
}

void StringIterator::restart()
{
    pos = 0;
    len = in_str.length();
    p1 = in_str.getCharPointer();
    p2 = p1;
}

void StringIterator::skipNewLines()
{
    while (!atEnd() && CHAr::isNewLine(*this)) move();
}

void StringIterator::skipToNewLine()
{
    while (!atEnd() && !CHAr::isNewLine(*this)) move();
}

void StringIterator::skipToChar(const juce_wchar&  c)
{
    while (!atEnd() && *this != c) move();
}

void StringIterator::skipToChar(const String&  s)
{
    while (!atEnd() && !CHAr::isAnyOf(*this, s)) move();
}

void StringIterator::skipPastChar(const juce_wchar& c)
{
    skipToChar(c);
    while (!atEnd() && *this == c) move();
}

void StringIterator::skipToSpace()
{
    while (!atEnd() && !CHAr::isWhiteSpace(*this)) move();
}

void StringIterator::skipToSpaceOrChar(const juce_wchar&  c)
{
    while (!atEnd() && *this != c && !CHAr::isWhiteSpace(*this)) move();
}

void StringIterator::skipSpaces()
{
    while (!atEnd() && CHAr::isWhiteSpace(*this)) move();
}

void StringIterator::skipChar(const juce_wchar & c)
{
    while (!atEnd() && *this == c) move();
}

String StringIterator::consumeToNewLine()
{    
    start();
    skipToNewLine();
    return get();
}

String StringIterator::consumeToSpace()
{
    start();
    skipToSpace();
    return get();
}

String StringIterator::consumeToNonSpace()
{
    start();
    skipSpaces();
    return get();
}

String StringIterator::consumeInt()
{
    start();
    enum MODE { begin, end };
    MODE mode = begin;
    while (!this->atEnd())
    {
        switch (mode)
        {
        case begin:
            if (!anyOf("-+") || !CHAr::isDigit(*this)) return get();
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

String StringIterator::consumeToChar(const juce_wchar& c)
{    
    start();
    while (!atEnd() && *this != c) move();
    return get();
}

String StringIterator::consumeToChar(const String& s)
{
    start();
    while (!atEnd() && !anyOf(s)) move();
    return get();
}

String StringIterator::consumePastChar(const String& s)
{
  start();
  while (!atEnd() && !anyOf(s)) move();
  while (!atEnd() && anyOf(s)) move();
  return get();
}

String StringIterator::consumeToSpaceOrChar(const juce_wchar & c)
{
    start();
    skipToSpaceOrChar(c);
    return get();
}

String StringIterator::consumeToEnd(int stop_early)
{
    start();
    while (!atEnd(stop_early)) move();
    return get();
}

String StringIterator::consumeToNewLineTRIM()
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

String StringIterator::consumeToCharTRIM(const juce_wchar& c)
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

String StringIterator::consumeToNewlineOrCharTRIM(const juce_wchar& c)
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

String StringIterator::consumeToChar_IgnoreEscapes(const String& s, const juce_wchar& escape)
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
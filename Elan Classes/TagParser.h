#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

using std::function;
using std::unordered_map;
using std::vector;

class StringIterator2
{
public:
	StringIterator2() {}
	StringIterator2(String str) : in_str(str) { restart(); }

	// operator
	juce_wchar operator++() { return inc(); }
	juce_wchar operator[](int i) const { return in_str[i]; }
	void operator=(String s) { in_str = s; restart(); }

	// conversion
	operator const int() const { return pos; }
	operator const char() const { return (char)* p2; }
	operator const juce_wchar() const { return *p2; }
	operator const String() const { return in_str; }

	// comparison
	bool operator!=(char c) const { return *p2 != (unsigned)c; }
	bool operator==(char c) const { return *p2 == (unsigned)c; }
	bool operator<(char c) const { return *p2 < (unsigned)c; }
	bool operator>(char c) const { return *p2 > (unsigned)c; }

	bool operator!=(int i) const { return pos != i; }
	bool operator==(int i) const { return pos == i; }
	bool operator<(int i) const { return pos < i; }
	bool operator>(int i) const { return pos > i; }

	bool operator!=(juce_wchar c) const { return *p2 != c; }
	bool operator==(juce_wchar c) const { return *p2 == c; }
	bool operator<(juce_wchar c) const { return *p2 < (juce_wchar)c; }
	bool operator>(juce_wchar c) const { return *p2 > (juce_wchar)c; }

	CharPointer_UTF8 p2 = in_str.getCharPointer();
	CharPointer_UTF8 p1 = in_str.getCharPointer();

	// Add a substring to memory
	void remember();

	// Add a substring to memory, skip next char, and have p1 = p2 which essentially removes previous characters from string. This is used to efficiently remove a single character (the next character) from the substring, especially useful for when trying to find and remove escape characters
	void rememberAndSkipOver();

	// Return all remembered substrings as a single string
	String getMemory();

	// have p1 = p2, used before doing a series of p2 increments 
	void start() { p1 = p2; }

	// increment p2 iterator if not at end
	void skip() { if (!atEnd()) { ++pos; ++p2; } }

	// increment p2 iterator and return character before increment
	char inc() { ++pos; return (char)* p2++; }

	// increment p2 iterator without returning character
	void move() { ++pos; ++p2; }

	void back() { --p1; }

	// get length of string
	int length() { return len; }

	// check if iterator is reading the last character
	bool atLast() const { return pos == len - 1; }

	// check if iterator is at end minus stop_early
	bool atEnd(int stop_early = 0) const { return pos >= (len - stop_early); }

	bool is(const juce_wchar & c) const { return *p2 == c; }
	bool isNot(const juce_wchar & c) const { return *p2 != c; }

	// check if current character is equal to any of characters in string
	bool isAnyOf(String s) const { return s.containsChar(*p2); }

	bool isDigit() const { return CharacterFunctions::isDigit(*p2); }

	bool isNonZeroDigit() const { return CharacterFunctions::isDigit(*p2) && isNot('0'); }

	// restart iterator back to beginning of string
	void restart();

	// increments until the last new line character
	void skipNewLines();

	// increments until encountering new line character
	void skipToNewLine();

	// increment until encountering character
	void skipToChar(const juce_wchar & c);

	// increment until encountering any of characters in string
	void skipToChar(const String & s);

	// increment until encountering char and then increments until char is not encountered
	void skipPastChar(const juce_wchar & c);

	// increments until the last white space character
	void skipToSpace();

	// increments until encountering char or white space
	void skipToSpaceOrChar(const juce_wchar & c);

	// increments until the last non white space character
	void skipSpaces();

	// increment until not encountering char
	void skipChar(const juce_wchar & c);

	// increment until not encountering char
	void skipChar(const String & s);

	// increments until the last new line character and returns the substring
	String consumeToNewLine();

	// increments until the last white space character and returns the substring
	String consumeToSpace();

	// increments until the last white space character and returns the substring
	String consumeToNonSpace();

	// increments until the last digit character and returns the substring
	String consumeInt();

	String prepareForNaturalSort()
	{
		enum MODE { skipSpaces, zeroFound, consume };
		MODE mode = skipSpaces;

		while (!this->atEnd())
		{
			switch (mode)
			{
			case skipSpaces:
				if (isAnyOf(" _"))
				{
					move();
				}
				else if (is('0'))
				{
					move(); mode = zeroFound;
				}
				else
				{
					start(); move(); mode = consume;
				}
				continue;
			case zeroFound:
				if (is('0'))
				{
					move();
				}
				else if (isDigit())
				{
					start(); move(); mode = consume;
				}
				else
				{
					start(); back();
					if (isAnyOf("_ "))
					{
						remember(); move(); start(); mode = skipSpaces;
					}
					else
					{
						move(); mode = consume;
					}
				}
				continue;
			case consume:
				if (!isAnyOf(" _"))
				{
					move();
				}
				else
				{
					remember(); move(); start(); mode = skipSpaces;
				}
				continue;
			}
		}
		remember();
		return getMemory();
	}

	// increments to end minus stop_early position and returns the substring
	String consumeToEnd(int stop_early = 0);

	// increments until encountering char and returns the substring
	String consumeToChar(const juce_wchar & c);

	// increments until encountering any char in string and returns the substring
	String consumeToChar(const String & s);

	// increments until encountering any char in string, then increments until char is not encountered, and returns the substring 
	String consumePastChar(const String & s);

	// increments until encountering char or white space and returns the substring
	String consumeToSpaceOrChar(const juce_wchar & c);

	// increments until the last new line character and returns the substring without pre and post white space
	String consumeToNewLineTRIM();

	// increments until encounting char and returns the substring without pre and post white space
	String consumeToCharTRIM(const juce_wchar & c);

	// increments until encounting char or newline and returns the substring without pre and post white space
	String consumeToNewlineOrCharTRIM(const juce_wchar & c);

	String consumeToChar_IgnoreEscapes(const String & s, const juce_wchar & escape);

private:
	// private members

// used to tell if we are at end of string or not;
	int pos = 0;
	int len = 0;
	String in_str;

	struct sub_mem { CharPointer_UTF8 a, b; };

	vector<sub_mem> memory;

	// get substring of from iterators p1 and p2
	String get() { return String(p1, p2); }
};

class TagParser
{
public:
    TagParser(const String & in_str);

		template <class T> String buildString(T & object)
		{
			StringArray s;
			for (auto& t : tags)
			{
				if (t.property_name.isEmpty())
				{
					s.add(t.prefix);
					continue;
				}

				String temp = object.GetPropertyStringFromKey(t.property_name, t.use_value);

				if (temp.isEmpty() && t.is_optional)
					continue;
				s.add(t.prefix + temp + t.suffix);
			}

			return s.joinIntoString("");
		}

		String buildString(std::function<String(const String &, bool)> getPropertyFunction)
		{
			StringArray s;
			for (auto& t : tags)
			{
				if (t.property_name.isEmpty())
				{
					s.add(t.prefix);
					continue;
				}

				String temp = getPropertyFunction(t.property_name, t.use_value);

				if (temp.isEmpty() && t.is_optional)
					continue;
				s.add(t.prefix + temp + t.suffix);
			}

			return s.joinIntoString("");
		}

    size_t size() { return tags.size(); }  

private:
    struct tag_properties
    {
        String property_name;
        String prefix;
        String suffix;
        bool use_value;
        bool is_optional;
    };

    vector<tag_properties> tags;

    enum Mode { Begin, Escape, InsideBracket, InsideExpression, Literal, SaveResult };
    Mode mode = Begin;
		StringIterator2 iter;

    String property_name;
    String prefix;
    String suffix;
    bool use_value = false;
    bool is_optional = false;
};

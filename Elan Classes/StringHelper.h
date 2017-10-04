#pragma once

#pragma warning (disable : 4018) // '<': signed/unsigned mismatch
#pragma warning (disable : 4389) // '!=': signed/unsigned mismatch
#pragma warning (disable : 4244) // 'return': conversion from 'juce::juce_wchar' to 'const char'

#include "../JuceLibraryCode/JuceHeader.h"
#include <unordered_set>
#include <vector>

using std::unordered_set;
using std::vector;
using std::string;
using std::stringstream;
using juce::String;
using juce::juce_wchar;
using juce::CharPointer_UTF8;

String toS(const juce_wchar& c); // convert char to string
String toS(int i); // convert int to string

struct toN
{
    String s;
    toN(String const& s) : s(s) {}
    operator double() { return s.getDoubleValue(); }
    operator int() { return s.getIntValue(); }
    operator String() { return s; }
};

namespace STR 
{
bool isInt(const String& s); // check if string is a correctly-formatted integer
String toUpper(const String& s);
String toLower(const String& s);
vector<String> split(const String& s, const juce_wchar & delim);
vector<String> split(const String& s, const String & delim); // split by any delimiter
vector<String> splitPast(const String& s, const String & delim); // split by any delimiter but ignore delimiters encountered subsequently
vector<String> split(const String& s, char delim, size_t min_size, String defval);
}

namespace CHAr
{
bool is(const juce_wchar & c1, const juce_wchar & c2); // case-sensitive compare two characters
bool is_i(const juce_wchar & c1, const juce_wchar & c2); // case-insensitive compare two characters 
bool isWhiteSpace(const juce_wchar& c); // check if character is white space
bool isNewLine(const juce_wchar& c); // check if character is new line character
bool isDigit(const juce_wchar& c); // check if character is digit
bool isAnyOf(const juce_wchar& c, const String& list); // case sensitive check if character is in string 
bool isAnyOf_i(const juce_wchar& c, const String& list); // case-insensitive check if character is in string 
bool isUpper(const juce_wchar& c); // check if character is upper case
bool isLower(const juce_wchar& c); // check if character is lower case
juce_wchar toUpper(const juce_wchar& c); // convert character to upper case
juce_wchar toLower(const juce_wchar& c); // convert character to lower case
}

class StringIterator
{
public:
    StringIterator() {}
    StringIterator(String str) : in_str(str) { restart(); }

    // operator
    juce_wchar operator++() { return inc(); }
    juce_wchar operator[](int i) const { return in_str[i]; }
    void operator=(String s) { in_str = s; restart(); }

    // conversion
    operator const int() const { return pos; }
    operator const char() const { return *p2; }
    operator const juce_wchar() const { return *p2; }
    operator const String() const { return in_str; }

    // comparison
    bool operator!=(char c) const { return *p2 != c; }
    bool operator==(char c) const { return *p2 == c; }
    bool operator<(char c) const { return *p2 < c; }
    bool operator>(char c) const { return *p2 > c; }

    bool operator!=(int i) const { return pos != i; }
    bool operator==(int i) const { return pos == i; }
    bool operator<(int i) const { return pos < i; }
    bool operator>(int i) const { return pos > i; }

    bool operator!=(juce_wchar c) const { return *p2 != c; }
    bool operator==(juce_wchar c) const { return *p2 == c; }
    bool operator<(juce_wchar c) const { return *p2 < c; }
    bool operator>(juce_wchar c) const { return *p2 > c; }

    CharPointer_UTF8 p2 = in_str.getCharPointer();
    CharPointer_UTF8 p1 = p1;

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
    char inc() { ++pos; return *p2++; }

    // increment p2 iterator without returning character
    void move() { ++pos; ++p2; }

    // get length of string
    int length() { return len; }

    // check if iterator is reading the last character
    bool atLast() const { return pos == len - 1; }

    // check if iterator is at end minus i
    bool atEnd(int stop_early = 0) const { return pos >= (len - stop_early); }

    // check if current character is equal to any of characters in string
    bool anyOf(String s) const { return CHAr::isAnyOf(*p2, s); }

    // restart iterator back to beginning of string
    void restart();

    // increments until the last new line character
    void skipNewLines();

    // increments until encountering new line character
    void skipToNewLine();

    // increment until encountering character
    void skipToChar(const juce_wchar&  c);

    // increment until encountering any of characters in string
    void skipToChar(const String& s);

    // increment until encountering char and then increments until char is not encountered
    void skipPastChar(const juce_wchar&  c);

    // increments until the last white space character
    void skipToSpace();

    // increments until encountering char or white space
    void skipToSpaceOrChar(const juce_wchar&  c);

    // increments until the last non white space character
    void skipSpaces();

    // increment until not encountering char
    void skipChar(const juce_wchar& c);

    // increments until the last new line character and returns the substring
    String consumeToNewLine();

    // increments until the last white space character and returns the substring
    String consumeToSpace();

    // increments until the last white space character and returns the substring
    String consumeToNonSpace();

    // increments until the last digit character and returns the substring
    String consumeInt();

    // increments to end minus stop_early position and returns the substring
    String consumeToEnd(int stop_early = 0);

    // increments until encountering char and returns the substring
    String consumeToChar(const juce_wchar& c);

    // increments until encountering any char in string and returns the substring
    String consumeToChar(const String& s);

    // increments until encountering any char in string, then increments until char is not encountered, and returns the substring 
    String consumePastChar(const String& s);

    // increments until encountering char or white space and returns the substring
    String consumeToSpaceOrChar(const juce_wchar& c);

    // increments until the last new line character and returns the substring without pre and post white space
    String consumeToNewLineTRIM();

    // increments until encounting char and returns the substring without pre and post white space
    String consumeToCharTRIM(const juce_wchar& c);

    // increments until encounting char or newline and returns the substring without pre and post white space
    String consumeToNewlineOrCharTRIM(const juce_wchar& c);

    String consumeToChar_IgnoreEscapes(const String& s, const juce_wchar& escape);

private:
    // private members
    int pos; // current index in string
    int len;
    String in_str;   

    struct sub_mem{ CharPointer_UTF8 a, b; };

    vector<sub_mem> memory;

    // private functions    
      
    String get() { return String(p1, p2); } // get substring of from iterators p1 and p2
};
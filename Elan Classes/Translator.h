#pragma once

#include <map>

using std::map;

class Translator
{
public:
    Translator(String& s);
    Translator() {}

    Translator set(String& s) { return Translator(s); }
    map<String, String> dict;
    String def;

private:
    // members
};

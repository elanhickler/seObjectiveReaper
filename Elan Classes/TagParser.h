#pragma once

#include "StringHelper.h"

#include <functional>
#include <unordered_map>
#include <vector>

using std::function;
using std::unordered_map;
using std::vector;

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
    StringIterator iter;

    String property_name;
    String prefix;
    String suffix;
    bool use_value = false;
    bool is_optional = false;
};

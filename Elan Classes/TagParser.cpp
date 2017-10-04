#include "TagParser.h"
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
            case '<': iter.move(); mode = InsideExpression; continue;
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

            if (iter == '#') use_value = true;

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
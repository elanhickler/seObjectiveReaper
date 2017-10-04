#include "Translator.h"

Translator::Translator(String& s)
{
    StringIterator iter(s);
    StringIterator subiter;

    Array<String> lookup_arr;
    Array<String> convert_arr;
    String result;
    String convert;

    enum Mode {newDict, splitVar};
    Mode mode = newDict;

    for (;;)
    {
        switch (mode)
        {
        case newDict:
            result = iter.consumeToNewlineOrCharTRIM('=');
            if (iter != '=') { def = result; continue; }
            iter.skip();

            convert = iter.consumeToNewLineTRIM();

            mode = splitVar; 
            continue;
        case splitVar:
            subiter = result;

            while (!subiter.atEnd())
            {
                dict[subiter.consumeToCharTRIM(',')] = convert;
                subiter.skip();
            }

            iter.skipSpaces();
            if (iter.atEnd()) return;

            mode = newDict; 
            continue;            
        }
    }
}
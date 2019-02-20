#include "MidiHelper.h"
#include <map>
#include "StringHelper.h"

#undef ERROR

using std::map;
using std::invalid_argument;
using std::string;

enum class e
{
    empty_string,
    num_out_of_bounds,
    not_a_root,
    name_out_of_bounds,
    cannot_read_octave
};

map<e, std::string> err_map ={
    { e::empty_string, "Input string is empty" },
    { e::num_out_of_bounds ,"Midi number not within 0 to 127" },
    { e::not_a_root,"Midi root not one of ABCDEFG" },
    { e::name_out_of_bounds,"Midi name converts to number not within 0 to 127" },
    { e::cannot_read_octave , "Octave number could not be read" }
};

void ERROR(e err)
{
    throw (invalid_argument)err_map[err];
}

map<juce_wchar, int> root_to_value ={ { 'C', 0 },{ 'D',2 },{ 'E',4 },{ 'F',5 },{ 'G',7 },{ 'A',9 },{ 'B',11 } };
map<int, juce_wchar> value_to_root =
{
		{ -1, 'B' },
    { 0, 'C' },{ 1, 'C' },
    { 2, 'D' },{ 3, 'D' },
    { 4, 'E' },
    { 5, 'F' },{ 6, 'F' },
    { 7, 'G' },{ 8, 'G' },
    { 9, 'A' },{ 10, 'A' },
    { 11, 'B' },
};
Array<int> accidental_numbers = { 1,3,6,8,10 };

map<juce_wchar, int> accidental_to_value = { { 'b', -1  },{ '#', +1 } };
map<int, juce_wchar> value_to_accidental = { {  -1, 'b' },{  +1, '#' } };

MIDI::MIDI() {};
MIDI::MIDI(String s, int transpose)
{
    midi_number = -1;
    MIDI::read(s, transpose);
};
MIDI::MIDI(int i, int transpose)
{
    midi_number = -1;
    MIDI::read(i, transpose);
};

void MIDI::read(String s, int transpose)
{
    if (s.isEmpty()) ERROR(e::empty_string);

    if (STR::isInt(s)) { MIDI::read(s.getIntValue()); return; }

    enum MODE { get_root, get_accidental, get_sign, get_octave };
    MODE mode = get_root;

    StringIterator it(s);

    while (!it.atEnd())
    {
        switch (mode)
        {
        case get_root:
            if (!CHAr::isAnyOf_i(it, "ABCDEFG"))
							ERROR(e::not_a_root);
            root = root_to_value[CHAr::toUpper(it)];
            mode = get_accidental;
            ++it;
            continue;
        case get_accidental:
            if (!CHAr::isAnyOf_i(it, "B#"))
						{
							accidental = 0;
							mode = get_sign;
							continue;
						}
            accidental = accidental_to_value[CHAr::toLower(it)];
            mode = get_sign;
            ++it;
            continue;
        case get_sign:
            if (it != '-') { sign = 1; mode = get_octave; continue; }
            sign = -1;
            ++it;
            mode = get_octave;
            continue;
        case get_octave:
            String o = it.consumeToEnd();
            if (!STR::isInt(o)) ERROR(e::cannot_read_octave);
            MIDI::read((o.getIntValue() * sign + lowest_octave*-1) * 12 + root + accidental, transpose);
            return;
        }
    }
}
void MIDI::read(int i, int transpose)
{
    i+=transpose;
    if (i < 0 || i > 127) ERROR(e::num_out_of_bounds);
    midi_number = i;
}


void MIDI::format(const String& s)
{
    if (s.isEmpty()) ERROR(e::empty_string);

    enum MODE { get_root, get_accidental, get_sign };

    MODE mode = get_root;

    use_flat = false;
    use_capital_root = true;
    use_capital_accidental = false;
    always_show_sign = false;

    for (auto p = s.getCharPointer(); !p.isEmpty();)
    {
        switch (mode)
        {
        case get_root:
            if (!CHAr::isAnyOf_i(*p, "ABCDEFG"))
						{
							mode = get_accidental;
							continue;
						}

            use_capital_root = CHAr::isUpper(*p);
            ++p;
            mode = get_accidental;
            continue;

        case get_accidental:
            if (!CHAr::isAnyOf(*p, "bB#"))
						{
							mode = get_sign;
							continue;
						}
            use_capital_accidental = CHAr::isUpper(*p);
            use_flat = CHAr::isAnyOf(*p, "bB");
            ++p;
            mode = get_sign;
            continue;

        case get_sign:
            always_show_sign = *p == '+';
            return;
        }
    }
}

String MIDI::getName()
{
    midi_name = "";
    juce_wchar c;

    // get root
    int modulo = midi_number % 12;
    if (use_flat && !(modulo == 4 || modulo == 11))
			c = value_to_root[modulo+1];
    else
			c = value_to_root[modulo];
    c = use_capital_root ? CHAr::toUpper(c) : CHAr::toLower(c);
    midi_name += c;

    // get accidental
    if (accidental_numbers.contains(modulo))
    {
        c = use_flat ? 'b' : '#';
        c = use_capital_accidental ? CHAr::toUpper(c) : CHAr::toLower(c);
        midi_name += c;
    }

    // get numeral sign
    octave = midi_number / 12 + lowest_octave;
    if (always_show_sign && octave >= 0)
			midi_name += "+";

    // get octave	
    midi_name += toS(octave);

    return midi_name;
}

String MIDI::getClass()
{
	return String(String::charToString(value_to_root[root]) + String::charToString(value_to_accidental[accidental]));
}

int MIDI::getClassValue() { return root + accidental; }

int MIDI::getName(int num)
{
    read(num);
    return getNum();
}

int MIDI::getNum()
{
    return midi_number;
}

int MIDI::getNum(const String & name)
{
    read(name);
    return getNum();
}

String MIDI::applyOffset(int i)
{
    midi_number += i;
    return MIDI::getName();
}

void MIDI::setMidOct(int i)
{
    lowest_octave = i - 5;
}

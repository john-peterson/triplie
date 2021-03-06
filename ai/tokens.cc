/*
 *      tokens.cc
 *
 *      Copyright 2008 Gorgi Kosev <spion@spion.ws>. 2012 John Peterson.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <map>
#include <list>
#include <math.h>
#include <stdexcept>
#include <boost/regex.hpp>
#include "tokens.h"
#include "ConvertUTF.c"

struct UnicodeException : public std::runtime_error {

    UnicodeException(std::string error_message) : std::runtime_error(error_message) {
    }
};

double uniform_deviate(int seed) {
    return seed * (1.0 / (RAND_MAX + 1.0));
}

void tokenize(const string& str,
        vector<string>& tokens,
        const string& delimiters) {
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

void tokenizelossles(const string& str,
        vector<string>& tokens,
        const string& delimiters) {
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos + 1));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

string gettok(const std::string& str, long int n,
        const std::string& delimiters) {
    vector<string> tokens;
    long int x;
    tokenize(str, tokens, delimiters);
    x = (long int) tokens.size();
    if ((n >= 0) && (n < x)) return tokens[n];
    else {
        return std::string("");
    }
}

long int numtok(const std::string& str,
        const std::string& delimiters) {
    vector<string> tokens;
    tokenize(str, tokens, delimiters);
    return (long int) tokens.size();
}

long int findtok(const std::string& str, const std::string& token,
        const std::string& delimiters) {
    vector<string> tokens;
    long int x;
    long int i;
    tokenize(str, tokens, delimiters);
    x = (long int) tokens.size();
    for (i = 0; i < x; i++) {
        if (token == tokens[i]) {
            return i;
            break;
        }
    }
    return -1;
}

string subtokstring(const vector<string>& tokens, unsigned int n1, unsigned int n2, const string& c) {
    string res;
    unsigned int i;
    i = n1;
    res = "";
    while ((i <= n2) && (i < tokens.size())) {
        res = res + tokens[i] + c;
        i++;
    }
    if (res.size() > 1) {
        res.erase(res.size() - 1, 1);
    }
    return res;
}

//Locale conversion


namespace UtfConverter {

    std::wstring FromUtf8(const std::string& utf8string) {
        size_t widesize = utf8string.length();
        if (sizeof (wchar_t) == 2) {
            std::wstring resultstring;
            resultstring.resize(widesize + 1, L'\0');
            const UTF8* sourcestart = reinterpret_cast<const UTF8*> (utf8string.c_str());
            const UTF8* sourceend = sourcestart + widesize;
            UTF16* targetstart = reinterpret_cast<UTF16*> (&resultstring[0]);
            UTF16* targetend = targetstart + widesize;
#ifdef TRIP_DEBUG
            cout << "External UTF conversion call..." << endl;
#endif
            ConversionResult res = ConvertUTF8toUTF16(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
#ifdef TRIP_DEBUG
            cout << "External UTF conversion call end" << endl;
#endif	
            if (res != conversionOK) {
#ifdef TRIP_DEBUG
                std::cout << "Igonoring problem with unicode conversion s2w: "
                        << utf8string << endl;
#endif
                throw UnicodeException(string("Error converting s2w: ") + utf8string + "\n");
            }
            *targetstart = 0;
            return resultstring;
        } else if (sizeof (wchar_t) == 4) {
            std::wstring resultstring;
            resultstring.resize(widesize + 1, L'\0');
            const UTF8* sourcestart = reinterpret_cast<const UTF8*> (utf8string.c_str());
            const UTF8* sourceend = sourcestart + widesize;
            UTF32* targetstart = reinterpret_cast<UTF32*> (&resultstring[0]);
            UTF32* targetend = targetstart + widesize;
#ifdef TRIP_DEBUG
            cout << "External UTF conversion call" << endl;
#endif
            ConversionResult res = ConvertUTF8toUTF32(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
#ifdef TRIP_DEBUG
            cout << "External UTF conversion call end" << endl;
#endif
            if (res != conversionOK) {
#ifdef TRIP_DEBUG
                std::cout << "Igonoring problem with unicode conversion s2w: "
                        << utf8string << endl;
#endif
                throw UnicodeException(string("Error converting s2w: ") + utf8string + "\n");
            }
            *targetstart = 0;
            return resultstring;
        } else {
            throw std::exception();
        }
        return L"";
    }

    std::string ToUtf8(const std::wstring& widestring) {
        size_t widesize = widestring.length();

        if (sizeof (wchar_t) == 2) {
            size_t utf8size = 3 * widesize + 1;
            std::string resultstring;
            resultstring.resize(utf8size, '\0');
            const UTF16* sourcestart = reinterpret_cast<const UTF16*> (widestring.c_str());
            const UTF16* sourceend = sourcestart + widesize;
            UTF8* targetstart = reinterpret_cast<UTF8*> (&resultstring[0]);
            UTF8* targetend = targetstart + utf8size;
#ifdef TRIP_DEBUG
            cout << "External UTF deconversion call" << endl;
#endif
            ConversionResult res = ConvertUTF16toUTF8(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
            if (res != conversionOK) {
                throw UnicodeException(string("Error converting w2s: ") + resultstring + "\n");
            }
#ifdef TRIP_DEBUG
            cout << "External UTF deconversion call end" << endl;
#endif
            *targetstart = 0;
            return resultstring;
        } else if (sizeof (wchar_t) == 4) {
            size_t utf8size = 4 * widesize + 1;
            std::string resultstring;
            resultstring.resize(utf8size, '\0');
            const UTF32* sourcestart = reinterpret_cast<const UTF32*> (widestring.c_str());
            const UTF32* sourceend = sourcestart + widesize;
            UTF8* targetstart = reinterpret_cast<UTF8*> (&resultstring[0]);
            UTF8* targetend = targetstart + utf8size;
#ifdef TRIP_DEBUG
            cout << "External UTF deconversion call" << endl;
#endif
            ConversionResult res = ConvertUTF32toUTF8(&sourcestart, sourceend, &targetstart, targetend, strictConversion);
#ifdef TRIP_DEBUG
            cout << "External UTF deconversion call end" << endl;
#endif
            if (res != conversionOK) {
#ifdef TRIP_DEBUG
                cout << "External UTF deconversion call FAILED" << endl;
#endif
                throw UnicodeException(string("Error converting w2s"));
            }
#ifdef TRIP_DEBUG
            cout << "Result seems to be okay." << endl;
#endif
            *targetstart = 0;
#ifdef TRIP_DEBUG
            cout << "Exit function" << endl;
#endif
            return resultstring;
        } else {
            throw UnicodeException(string("wchar_t size invalid"));
        }
        return "";
    }
}

void lowercase(string& s) {
    /*
    wstring ws;
    #ifdef TRIP_DEBUG
            cout << "Unicode lowercaser..." << endl;
    #endif
    try
    {
            #ifdef TRIP_DEBUG
                    cout << "From UTF8..." << endl;
            #endif
            ws = UtfConverter::FromUtf8(s);
            for (unsigned i = 0; i < ws.size(); ++i)
                    ws[i] = towlower(ws[i]);
            #ifdef TRIP_DEBUG
                    cout << "To UTF8..." << endl;
            #endif
            s = UtfConverter::ToUtf8(ws);
            #ifdef TRIP_DEBUG
                    cout << "To UTF8 end" << endl;
            #endif
            s = s.substr(0,s.find_first_of('\0'));
            #ifdef TRIP_DEBUG
                    cout << "UTF lowercase success" << endl;
            #endif
		
    }
    catch (UnicodeException e)
    {
     */
    //fall-back to regular lowercasing.
    for (unsigned i = 0; i < s.size(); ++i)
        s[i] = tolower(s[i]);
    return;
    /*
            }
            #ifdef TRIP_DEBUG
                    cout << "End unicode lowercaser" << endl;
            #endif
     */

}

// find in words. require word part to be at least four letters or half the word length.
bool find_in_words(string& s, string t) {
	boost::sregex_iterator i(s.begin(), s.end(), boost::regex("\\b[\\w]+\\b", boost::regex::perl));
	boost::sregex_iterator j;
	string m;
	bool match = false;
	for (; i != j; i++) {
		if ((i->str().length() >=4 || i->str().length() >= t.length()/2) && t.find(i->str()) != string::npos) {
			match = true;
			m.append(i->str());
		}
	}
	s = boost::regex_replace(s, boost::regex(m), "");
	return match;
}
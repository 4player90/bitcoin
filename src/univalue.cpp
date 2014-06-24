// Copyright 2014 BitPay Inc.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <ctype.h>
#include <sstream>
#include "univalue.h"

using namespace std;

void UniValue::clear()
{
    typ = VNULL;
    val.clear();
    keys.clear();
    values.clear();
}

bool UniValue::setNull()
{
    clear();
    return true;
}

bool UniValue::setBool(bool val)
{
    clear();
    typ = (val ? VTRUE : VFALSE);
    return true;
}

static bool validNumStr(const string& s)
{
    bool seenDec = false;
    for (unsigned int i = 0; i < s.size(); i++) {
        switch (s[i]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            // do nothing
            break;

        case '.':
            if (seenDec)
                return false;
            seenDec = true;
            break;

        default:
            return false;
        }
    }

    return true;
}

bool UniValue::setNumStr(string val_)
{
    if (!validNumStr(val))
        return false;

    clear();
    typ = VNUM;
    val = val_;
    return true;
}

bool UniValue::setInt(int64_t val)
{
    string s;
    ostringstream oss;

    oss << val;

    return setNumStr(oss.str());
}

bool UniValue::setFloat(double val)
{
    string s;
    ostringstream oss;

    oss << val;

    return setNumStr(oss.str());
}

bool UniValue::setStr(string val_)
{
    clear();
    typ = VSTR;
    val = val_;
    return true;
}

bool UniValue::setArray()
{
    clear();
    typ = VARR;
    return true;
}

bool UniValue::setObject()
{
    clear();
    typ = VOBJ;
    return true;
}

bool UniValue::push(UniValue& val)
{
    if (typ != VARR)
        return false;

    values.push_back(val);
    return true;
}

bool UniValue::pushKV(string key, UniValue& val)
{
    if (typ != VOBJ)
        return false;

    keys.push_back(key);
    values.push_back(val);
    return true;
}

bool UniValue::getArray(std::vector<UniValue>& arr)
{
    if (typ != VARR)
        return false;

    arr = values;
    return true;
}

bool UniValue::getObject(std::map<std::string,UniValue>& obj)
{
    if (typ != VOBJ)
        return false;

    obj.clear();
    for (unsigned int i = 0; i < keys.size(); i++) {
        obj[keys[i]] = values[i];
    }

    return true;
}

int UniValue::findKey(const std::string& key)
{
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (keys[i] == key)
            return (int) i;
    }

    return -1;
}

bool UniValue::checkObject(const std::map<std::string,UniValue::VType>& t)
{
    for (std::map<std::string,UniValue::VType>::const_iterator it = t.begin();
         it != t.end(); it++) {
        int idx = findKey(it->first);
        if (idx < 0)
            return false;

        if (values[idx].getType() != it->second)
            return false;
    }

    return true;
}

UniValue UniValue::getByKey(const std::string& key)
{
    UniValue nullVal;

    if (typ != VOBJ)
        return nullVal;

    int index = findKey(key);
    if (index < 0)
        return nullVal;

    return values[index];
}

UniValue UniValue::getByIdx(unsigned int index)
{
    UniValue nullVal;

    if (typ != VOBJ && typ != VARR)
        return nullVal;
    if (index >= values.size())
        return nullVal;

    return values[index];
}


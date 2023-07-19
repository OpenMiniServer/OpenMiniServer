/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_STRING_H
#define HEADER_OPEN_STRING_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>

namespace open
{

struct OpenString
{
    static void Split(const std::string& line, const std::string& split, std::vector<std::string>& vect_items);
    static std::string Tolower(const std::string& str);
    static std::string Toupper(const std::string& str);
    static std::string ToString(double value);
    static std::string ToString(int32_t value);
    static std::string ToString(int64_t value);

    static std::string CodeTimeToKey(const std::string& code, int64_t dtime);
    static std::string TimeCodeToKey(int64_t dtime, const std::string& code);
    static std::string TimeToKey(int64_t dtime);

    static std::string UrlEncode(const std::string& str);
    static std::string UrlDecode(const std::string& str);

    static void MD5Hash(const std::string& input, std::string& output);
    static void TrimComment(const std::string& input, std::string& output);

    static bool GBKToUTF8(const std::string& strGBK, std::string& strUTF8);

};


};


#endif  //HEADER_OPEN_STRING_H
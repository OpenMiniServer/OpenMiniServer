/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/
#include "opentime.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include <time.h>
//#include <sstream>
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static unsigned short DayOfMonths[2][12] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static const char* WdayNames[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* MonthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

namespace open
{

OpenTime::OpenTime()
    :second_(0),
    minute_(0),
    hour_(0),
    day_(0),
    month_(0),
    year_(0),
    timezone_(OPENTIME_TIMEZONE),
    unixtime_(::time(0))
{
    unixtimeToDate();
}

OpenTime::OpenTime(int64_t unixtime, int timezone)
    :second_(0),
    minute_(0),
    hour_(0),
    day_(0),
    month_(0),
    year_(0), 
    timezone_(timezone),
    unixtime_(unixtime != 0 ? unixtime : ::time(0))
{
    unixtimeToDate();
}

OpenTime::OpenTime(int year, int mouth, int day, int hour, int minute, int second, int timezone)
    :second_(second),
    minute_(minute),
    hour_(hour),
    day_(day),
    month_(mouth),
    year_(year),
    timezone_(timezone),
    unixtime_(0)
{
    dateToUnixtime();
}

OpenTime::OpenTime(const OpenTime& openTime)
    :second_(0),
    minute_(0),
    hour_(0),
    day_(0),
    month_(0),
    year_(0)
{
    timezone_ = openTime.timezone_;
    unixtime_ = openTime.unixtime_;
    unixtimeToDate();
}

void OpenTime::operator=(const OpenTime& openTime)
{
    timezone_ = openTime.timezone_;
    unixtime_ = openTime.unixtime_;
    unixtimeToDate();
}

bool OpenTime::operator==(const OpenTime& openTime)
{
    return timezone_ == openTime.timezone_ && unixtime_ == openTime.unixtime_;
}

uint16_t OpenTime::wday()
{
    int64_t st = unixtime_ + timezone_ * 3600;
    st = (st - st % 86400) / 86400;
    uint16_t weekDay = (4 + st % 7) % 7;
    return weekDay;
}

void OpenTime::operator+=(int64_t st)
{
    unixtime_ += st;
    unixtimeToDate();
}

void OpenTime::operator-=(int64_t st)
{
    unixtime_ -= st;
    unixtimeToDate();
}

void OpenTime::addSecond(int second)
{
    unixtime_ += second;
    unixtimeToDate();
}

void OpenTime::addMinute(int minute)
{
    unixtime_ += (60 * minute);
    unixtimeToDate();
}

void OpenTime::addHour(int hour)
{
    unixtime_ += (3600 * hour);
    unixtimeToDate();
}

void OpenTime::addDay(int day)
{
    unixtime_ += day * 86400;
    unixtimeToDate();
}

void OpenTime::addMonth(int month)
{
    if (month >= 0)
    {
        for (size_t i = 0; i < month; ++i)
        {
            if (month_ == 12)
            {
                ++year_;
                month_ = 1;
            }
            else
            {
                ++month_;
            }
        }
    }
    else
    {
        month = -month;
        for (size_t i = 0; i < month; ++i)
        {
            if (month_ == 1)
            {
                month_ = 12;
                year_ -= 1;
            }
            else
            {
                month_ -= 1;
            }
        }
    }
    adjustMonth();
    dateToUnixtime();
}
void OpenTime::adjustMonth()
{
    if (day_ <= 28) return;
    if (month_ > 12) month_ = 12;
    int leap = ((year_ % 4 == 0 && year_ % 100 != 0) || year_ % 400 == 0) ? 1 : 0;
    unsigned int maxday = DayOfMonths[leap][month_ - 1];
    if (day_ > maxday)
        day_ = maxday;
}

void OpenTime::addYear(int year)
{
    year_ += year;
    dateToUnixtime();
}

void OpenTime::addQuarter(int quarter)
{
    addMonth(quarter * 3);
}

bool OpenTime::isQuarterLastDay()
{
    return (month_ == 3 && day_ == 31
        || month_ == 6 && day_ == 30
        || month_ == 9 && day_ == 30
        || month_ == 12 && day_ == 31);
}

void OpenTime::setTimezone(int timezone)
{
    timezone_ = timezone;
    unixtimeToDate();
}

void OpenTime::setUnixtime(int64_t unixtime)
{
    unixtime_ = unixtime;
    unixtimeToDate();
}

int64_t OpenTime::alignDay()
{
    return AlignDay(unixtime(), timezone_);
}

void OpenTime::unixtimeToDate()
{
    int64_t tempTS = unixtime_ + 3600 * timezone_;
    second_ = tempTS % 60;
    tempTS /= 60;
    minute_ = tempTS % 60;
    tempTS /= 60;
    hour_ = tempTS % 24;
    tempTS /= 24;
    year_ = 1970;
    for (int i = 0; i < 2; ++i)
    {
        if (tempTS >= 365)
        {
            ++year_;
            tempTS -= 365;
        }
        else
        {
            for (int j = 0; j < 12; ++j)
            {
                month_ = j + 1;
                if (tempTS >= DayOfMonths[0][j])
                {
                    tempTS -= DayOfMonths[0][j];
                }
                else
                {
                    day_ = (uint16_t)(tempTS + 1); 
                    return;
                }
            }
        }
    }
    if (tempTS >= 366)
    {
        ++year_;
        tempTS -= 366;
    }
    else
    {
        for (int j = 0; j < 12; ++j)
        {
            month_ = j + 1;
            if (tempTS >= DayOfMonths[1][j])
            {
                tempTS -= DayOfMonths[1][j];
            }
            else
            {
                day_ = (uint16_t)(tempTS + 1);
                return;
            }
        }
    }
    unsigned int years = (unsigned int)(tempTS / (365 * 4 + 1) * 4);
    tempTS %= 365 * 4 + 1;
    year_ += years;
    for (int i = 0; i < 3; ++i)
    {
        if (tempTS >= 365)
        {
            ++year_;
            tempTS -= 365;
        }
        else
        {
            for (int j = 0; j < 12; ++j)
            {
                month_ = j + 1;
                if (tempTS >= DayOfMonths[0][j])
                {
                    tempTS -= DayOfMonths[0][j];
                }
                else
                {
                    day_ = (uint16_t)(tempTS + 1); 
                    return;
                }
            }
        }
    }
    for (int j = 0; j < 12; ++j)
    {
        month_ = j + 1;
        if (tempTS >= DayOfMonths[1][j])
        {
            tempTS -= DayOfMonths[1][j];
        }
        else
        {
            day_ = (uint16_t)(tempTS + 1);
        }
    }
}

void OpenTime::dateToUnixtime()
{
    unixtime_ = second_;
    if (second_ > 59 || minute_ > 59 || hour_ > 23 
        || day_ > 31 || month_ > 12 || year_ - 1 < 1969) return;

    unixtime_ += minute_ * 60 + hour_ * 3600 + (day_ - 1) * 86400;
    //int isleak = year_ % 4 == 0 ? 1 : 0;
    int leap = ((year_ % 4 == 0 && year_ % 100 != 0) || year_ % 400 == 0) ? 1 : 0;
    for (int i = 0; i < month_ - 1; ++i)
    {
        unixtime_ += DayOfMonths[leap][i] * 86400;
    }
    if (year_ > 1970)
    {
        unixtime_ += 365 * 86400;
        if (year_ > 1971)
        {
            unixtime_ += 365 * 86400;
            if (year_ > 1972)
            {
                unixtime_ += 366 * 86400;
                int leftYear = year_ - 1973;
                unixtime_ += (leftYear / 4 * (365 * 4 + 1) + (leftYear % 4) * 365) * 86400;
            }
        }
    }
    unixtime_ -= timezone_ * 3600;
}

void OpenTime::fromIntTime(int64_t dateTime)
{
    if (dateTime < 20230213 * 10)
    {
        day_ = dateTime % 100;
        dateTime /= 100;
        month_ = dateTime % 100;
        dateTime /= 100;
        year_ = (uint32_t)dateTime;
        hour_ = 0;
        minute_ = 0;
        second_ = 0;
    }
    else
    {
        second_ = dateTime % 100;
        dateTime = (dateTime - second_) / 100;
        minute_ = dateTime % 100;
        dateTime = (dateTime - minute_) / 100;
        hour_ = dateTime % 100;
        dateTime = (dateTime - hour_) / 100;
        day_ = dateTime % 100;
        dateTime = (dateTime - day_) / 100;
        month_ = dateTime % 100;
        dateTime = (dateTime - month_) / 100;
        year_ = (uint32_t)dateTime;
    }
    dateToUnixtime();
}

int64_t OpenTime::toIntTime()
{
    int64_t intTime = year_ * 10000000000 + month_ * 100000000 + day_ * 1000000 
        + hour_ * 10000 + minute_ * 100 + second_;
    return intTime;
}

bool OpenTime::fromString(const std::string& strTime)
{
    int n[6] = { 0 };
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    int ret = sscanf_s(strTime.c_str(), "%04d-%02d-%02d %02d:%02d:%02d", &n[0], &n[1], &n[2], &n[3], &n[4], &n[5]);
#else
    int ret = sscanf(strTime.c_str(), "%04d-%02d-%02d %02d:%02d:%02d", &n[0], &n[1], &n[2], &n[3], &n[4], &n[5]);
#endif
    if (!ret) return false;
    year_ = n[0]; month_  = n[1]; day_    = n[2];
    hour_ = n[3]; minute_ = n[4]; second_ = n[5];
    dateToUnixtime();
    return true;
}

bool OpenTime::fromString(const std::string& strTime, const std::string& format)
{
    unsigned char tmp = 0;
    unsigned char tmp1 = 0;
    size_t isize = format.size();
    size_t ksize = strTime.size();
    size_t k = 0;
    size_t n = 0;
    std::string str;
    for (size_t i = 0; i < isize; ++i)
    {
        tmp = format[i];
        if (tmp != '%') continue;
        ++i;
        if (i >= isize)
        {
            break;
        }
        while ((strTime[k] < '0' || strTime[k] > '9') && k < ksize) ++k;
        if (k >= ksize) break;
        str.clear();
        tmp = format[i];
        if (tmp == 'Y')
        {
            for (n = 0; k < ksize && n < 4; ++n, ++k)
            {
                tmp1 = strTime[k];
                if (tmp1 < '0' || tmp1 > '9') break;
                str.push_back(tmp1);
            }
        }
        else
        {
            for (n = 0; k < ksize && n < 2; ++n, ++k)
            {
                tmp1 = strTime[k];
                if (tmp1 < '0' || tmp1 > '9') break;
                str.push_back(tmp1);
            }
        }
        switch (tmp)
        {
        case 's': second_ = atoi(str.c_str()); break;
        case 'm': minute_ = atoi(str.c_str()); break;
        case 'h': hour_ = atoi(str.c_str()); break;
        case 'D': day_ = atoi(str.c_str()); break;
        case 'M': month_ = atoi(str.c_str()); break;
        case 'Y': year_ = atoi(str.c_str()); break;
        default:
            break;
        }
    }
    dateToUnixtime();
    return true;
}

bool OpenTime::fromGMT(const std::string& strTime)
{
    std::string str;
    int idx = 0;
    bool isStart = true;
    unsigned char tmp = 0;
    size_t size = strTime.size();
    for (size_t i = 0; i < size; ++i)
    {
        tmp = strTime[i];
        if (isStart && tmp == ' ') continue;
        isStart = false;
        if (idx == 0)
        {
            if (tmp == ',')
            {
                ++idx;
                isStart = true;
            }
            continue;
        }
        if (tmp == ' ' || tmp == ':')
        {
            if (idx == 6)
            {
                second_ = atoi(str.c_str());
                break;
            }
            switch (idx)
            {
            case 1:
                day_ = atoi(str.c_str());
                break;
            case 2:
                month_ = 1;
                for (size_t k = 0; k < sizeof(MonthNames) / sizeof(MonthNames[0]); k++)
                {
                    if (MonthNames[k] == str)
                    {
                        month_ = (uint16_t)(k + 1); break;
                    }
                }
                break;
            case 3:
                year_ = atoi(str.c_str());
                break;
            case 4:
                hour_ = atoi(str.c_str());
                break;
            case 5:
                minute_ = atoi(str.c_str());
                break;
            default:
                break;
            }
            ++idx;
            str.clear();
            isStart = true;
        }
        else
        {
            str.push_back(tmp);
        }
    }
    if (idx != 6)
    {
        return false;
    }
    dateToUnixtime();
    return true;
}

// %Y-%M-%D %h:%m:%s
std::string OpenTime::toString(const std::string& format)
{
    if (format.empty()) return toString();
    if (format.size() == 1)
    {
        char tmp[256] = { 0 };
        snprintf(tmp, sizeof(tmp), "%04d%s%02d%s%02d%s%02d%s%02d%s%02d", 
            year_, format.c_str(), month_, format.c_str(), day_, format.c_str(), 
            hour_, format.c_str(), minute_, format.c_str(), second_);
        return tmp;
    }
    std::string str;
    int value = 0;
    unsigned char tmp = 0;
    char buffer[64] = { 0 };
    size_t size = format.size();
    for (size_t i = 0; i < size; ++i)
    {
        tmp = format[i];
        if (tmp != '%')
        {
            str.push_back(tmp);
            continue;
        }
        ++i;
        if (i >= size)
        {
            str.push_back(tmp);
            break;
        }
        tmp = format[i];
        switch (tmp)
        {
        case 's': value = second_; break;
        case 'm': value = minute_; break;
        case 'h': value = hour_; break;
        case 'D': value = day_; break;
        case 'M': value = month_; break;
        case 'Y': value = year_; break;
        default:
            str.push_back(format[i - 1]);
            str.push_back(tmp);
            continue;
        }
        snprintf(buffer, sizeof(buffer), "%02d", value);
        str.append(buffer);
    }
    return str;
}

std::string OpenTime::toGMT()
{
    OpenTime openTime(unixtime(), 0);
    uint32_t weekDay = openTime.wday();
    if (weekDay >= sizeof(WdayNames) / sizeof(WdayNames[0]))
    {
        return "";
    }
    if (openTime.month() - 1 >= sizeof(MonthNames) / sizeof(MonthNames[0]))
    {
        return "";
    }
    const char* monthName = MonthNames[month_ - 1];
    const char* wdayName = WdayNames[weekDay];
    char tmp[256] = { 0 };
    snprintf(tmp, sizeof(tmp), "%s, %02d %s %04d %02d:%02d:%02d GMT", 
        wdayName, (int)day_, monthName, (int)year_, (int)hour_, (int)minute_, (int)second_);
    return tmp;
}

std::string OpenTime::toString()
{
    char tmp[256] = { 0 };
    snprintf(tmp, sizeof(tmp), "%04d-%02d-%02d %02d:%02d:%02d", year_, month_, day_, hour_, minute_, second_);
    return tmp;
}

std::string OpenTime::toString(int32_t milliSecond)
{
    char tmp[256] = { 0 };
    snprintf(tmp, sizeof(tmp), "%04d-%02d-%02d %02d:%02d:%02d:%03d", year_, month_, day_, hour_, minute_, second_, milliSecond);
    return tmp;
}

int64_t OpenTime::AlignDay(int64_t unixtime, int timezone)
{
    unixtime += timezone * 3600;
    return unixtime - unixtime % 86400 - timezone * 3600;
}

std::string OpenTime::ToString(int64_t unixtime, int timezone)
{
    OpenTime dt(unixtime, timezone);
    return dt.toString();
}

std::string OpenTime::MilliToString(int64_t milliSecond, int timezone)
{
    OpenTime dt(milliSecond / 1000, timezone);
    return dt.toString(milliSecond % 1000);
}

void OpenTime::Sleep(int64_t milliSecond)
{
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    ::Sleep((DWORD)milliSecond);
#else
    // struct timespec ts;
    // ts.tv_sec = (time_t)(milliSecond / 1000);
    // ts.tv_nsec = (long)((milliSecond % 1000) * 1000000);
    // nanosleep(&ts, NULL);
    ::usleep(milliSecond * 1000);
#endif
}

int64_t OpenTime::Unixtime()
{
    return ::time(0);
}

int64_t OpenTime::MilliUnixtime()
{
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    //int64_t systemTime = 0;
    //::GetSystemTimeAsFileTime((LPFILETIME)&systemTime);
    //int64_t milliSecond = (systemTime / 10000000 - 11644473600LL) * 1000 + (systemTime / 10) % 1000000;
    //return milliSecond;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    struct tm tm;
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    time_t clock = mktime(&tm);
    struct timeval tv;
    tv.tv_sec = (long)clock;
    tv.tv_usec = wtm.wMilliseconds * 1000;
    int64_t milliSecond = ((unsigned long long)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000);
    return milliSecond;
#else
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    int64_t milliSecond = tv.tv_sec * 1000 + tv.tv_usec/1000;
    return milliSecond;
#endif
}

}

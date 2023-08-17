/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_TIME_H
#define HEADER_OPEN_TIME_H

#include <stdint.h>
#include <stdlib.h>
#include <string>

#ifndef OPENTIME_TIMEZONE
#define OPENTIME_TIMEZONE 8
#endif

namespace open
{

class OpenTime
{
    void adjustMonth();
public:
    uint16_t second_; // 0-59
    uint16_t minute_; // 0-59
    uint16_t hour_;   // 0-23
    uint16_t day_;    // 1-31
    uint16_t month_;  // 1-12
    uint32_t year_;   // 1970 - 2100
    uint64_t unixtime_;
    int16_t timezone_; // -17 - 17

    OpenTime();
    OpenTime(int64_t unixtime, int timezone = OPENTIME_TIMEZONE);
    OpenTime(int year, int mouth, int day, int hour, int minute, int second, int timezone = OPENTIME_TIMEZONE);
    OpenTime(const OpenTime& openTime);
    void operator=(const OpenTime& openTime);
    bool operator==(const OpenTime& openTime);

    inline const uint16_t& second() { return second_; }
    inline const uint16_t& minute() { return minute_; }
    inline const uint16_t& hour() { return hour_; }
    inline const uint16_t& day() { return day_; }
    inline const uint16_t& month() { return month_; }
    inline const uint32_t& year() { return year_; }
    // 0 - 6
    uint16_t wday();

    inline const uint64_t& unixtime() { return unixtime_; }
    void setUnixtime(int64_t unixtime = 0);
    inline void operator=(int64_t unixtime) { setUnixtime(unixtime); }
    inline void operator=(const std::string& strTime) { fromString(strTime); }

    // 2023-02-13 21:45:00 => 2023-05-13 00:00:00
    int64_t alignDay();

    // 2023-02-13 21:45:00 => 2023-02-13 21:45:01
    void addSecond(int second = 1);
    void operator+=(int64_t st);
    void operator-=(int64_t st);

    // 2023-02-13 21:45:00 => 2023-02-13 21:46:00
    void addMinute(int minute = 1);

    // 2023-02-13 21:45:00 => 2023-02-13 22:45:00
    void addHour(int hour = 1);

    // 2023-02-13 21:45:00 => 2023-02-14 21:45:00
    void addDay(int day = 1);

    // 2023-02-13 21:45:00 => 2023-03-13 21:45:00
    void addMonth(int month = 1);

    // 2023-02-13 21:45:00 => 2024-02-13 21:45:00
    void addYear(int year = 1);

    // 2023-02-13 21:45:00 => 2023-05-13 21:45:00
    void addQuarter(int quarter = 1);
    bool isQuarterLastDay();

    inline int16_t timezone() { return timezone_; }
    void setTimezone(int timezone = OPENTIME_TIMEZONE);

    // 1678715100 => 2023-05-13 21:45:00
    void unixtimeToDate();

    // 2023-05-13 21:45:00 => 1678715100
    void dateToUnixtime();

    // dateTime:20230213 or 20230213214500
    void fromIntTime(int64_t dateTime);
    int64_t toIntTime();

    // from 2023-05-13 21:45:00
    bool fromString(const std::string& strTime);
    // from %Y-%M-%D %h:%m:%s
    bool fromString(const std::string& strTime, const std::string& format);
    // "Mon, 20 Feb 2023 15:18:58 GMT"
    bool fromGMT(const std::string& strTime);

    // to 2023-05-13 21:45:00
    std::string toString();
    // to 2023-05-13 21:45:00:843
    std::string toString(int32_t milliSecond);
    // to %Y-%M-%D %h:%m:%s
    std::string toString(const std::string& format);
    // "Mon, 20 Feb 2023 15:18:58 GMT"
    std::string toGMT();

    inline void setSecond(int second)
    {
        second_ = (uint16_t)second;
        dateToUnixtime();
    }
    inline void setMinute(int minute)
    {
        minute_ = (uint16_t)minute;
        dateToUnixtime();
    }
    inline void setHour(int hour)
    {
        hour_ = (uint16_t)hour;
        dateToUnixtime();
    }
    inline void setDay(int day)
    {
        day_ = (uint16_t)day;
        dateToUnixtime();
    }
    inline void setMonth(int month)
    {
        month_ = (uint16_t)month;
        dateToUnixtime();
    }
    inline void setYear(int year)
    {
        year_ = (uint16_t)year;
        dateToUnixtime();
    }

    static int64_t AlignDay(int64_t unixtime, int timezone = OPENTIME_TIMEZONE);
    static std::string ToString(int64_t utctime, int timezone = OPENTIME_TIMEZONE);
    static std::string MilliToString(int64_t milliSecond, int timezone = OPENTIME_TIMEZONE);
    static void Sleep(int64_t milliSecond);
    // timestamp
    static int64_t Unixtime();
    // timestamp milliUnixtime
    static int64_t MilliUnixtime();

};

};

#endif /* HEADER_OPEN_TIME_H */

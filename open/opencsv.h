/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_CSV_H
#define HEADER_OPEN_CSV_H

#include <string>
#include <vector>

namespace open
{
class OpenCSV
{

public:
    typedef std::vector<std::string> Line;
    class CSVLine
    {
        Line line_;
        OpenCSV* csv_;
        CSVLine()
            :csv_(0) {}
        friend class OpenCSV;
    public:
        CSVLine(OpenCSV* csv)
            :csv_(csv) {}
        CSVLine(const CSVLine& csvline)
        {
            csv_ = csvline.csv_;
            if (!csvline.line_.empty())
            {
                line_ = csvline.line_;
            }
        }
        inline size_t size() { return line_.size(); }
        inline bool empty() { return line_.empty(); }
        inline Line& line() { return line_; }
        std::string& operator[](size_t idx);
        std::string& operator[](const std::string& key);
    };

protected:
    typedef const std::initializer_list<std::string> List;
    std::vector<CSVLine> lines_;
    std::string emptyStr_;
    friend class CSVLine;
public:

    OpenCSV() {}
    OpenCSV(List& list);
    void operator=(List& list);

    //OpenCSV(OpenCSV& csv);
    //void operator=(OpenCSV& csv);

    inline bool empty() { return lines_.empty(); }
    inline size_t size() { return lines_.size(); }

    CSVLine& operator[](size_t idx);
    void operator>>(std::string& output);
    void operator<<(const std::string& output);

    inline std::vector<CSVLine>& lines() { return lines_; }
};

};

#endif /* HEADER_OPEN_CSV_H */

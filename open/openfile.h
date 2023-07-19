/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_FILE_H
#define HEADER_OPEN_FILE_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include "openbuffer.h"


namespace open
{

class OpenFileBuffer
{
    bool isNeedFree_;
    void* data_;
    size_t size_;
public:
    OpenFileBuffer(void* data, size_t size, bool needFree = false)
        :isNeedFree_(needFree), data_(data), size_(size) 
    {}
    OpenFileBuffer()
        :isNeedFree_(false), data_(0), size_(0)
    {}
    ~OpenFileBuffer()
    {
        if (isNeedFree_)
        {
            isNeedFree_ = false;
            if (data_) free(data_);
        }
        data_ = 0;
        size_ = 0;
    }
    void resize(size_t size)
    {
        assert(data_ == 0);
        assert(size_ == 0);
        assert(!isNeedFree_);
        if (data_)
        {
            return;
        }
        size_ = size;
        isNeedFree_ = true;
        data_ = malloc(size + 1);
        if (data_)
        {
            memset(data_, 0, size + 1);
        }
    }
    inline void* data() { return data_; }
    inline size_t size() { return size_; }
    inline void* data() const { return data_; }
    inline size_t size() const { return size_; }
};

struct OpenWriteFile
{
    void* file_;
    std::string filePath_;

    OpenWriteFile(const std::string& filePath = "");
    ~OpenWriteFile();
    void setFilePath(const std::string& filePath);
    size_t write(const char* data, size_t size);
};

struct OpenFile
{
    static void GetFileExt(const std::string& filePath, std::string& fileName, std::string& fileExt);
    static int64_t ReadFile(const std::string& filePath, OpenFileBuffer& buffer, const char* m = "rb");
    static int64_t WriteFile(const std::string& filePath, const OpenFileBuffer& buffer, const char* m = "wb");

    static int64_t ReadFile(const std::string& filePath, std::string& buffer, const char* m = "rb");
    static int64_t WriteFile(const std::string& filePath, const std::string& buffer, const char* m = "wb");

    static int64_t ReadFile(const std::string& filePath, OpenBuffer& buffer, const char* m = "rb");
    static int64_t WriteFile(const std::string& filePath, OpenBuffer& buffer, const char* m = "wb");

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    static FILE* Open(const char* filePath, const char* mode);
    static void Close(FILE* fp);

    static bool IsFileExist(const std::string& filePath);

    static bool IsDir(const std::string& filePath);
    static bool MakeDir(const std::string& filePath);
#endif
};

};

#endif //HEADER_OPEN_FILE_H
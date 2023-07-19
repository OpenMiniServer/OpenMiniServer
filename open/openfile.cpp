/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/
#define _CRT_SECURE_NO_WARNINGS

#include "openfile.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

#include <windows.h>
#include <wchar.h>

#endif

namespace open
{

OpenWriteFile::OpenWriteFile(const std::string& filePath)
    :filePath_(filePath)
{
    file_ = 0;
    if (!filePath_.empty())
    {
        file_ = fopen(filePath.c_str(), "wb");
        if (!file_)
        {
            printf("OpenWriteFile::OpenWriteFile:%s\n", filePath.data());
            assert(false);
        }
    }
}
OpenWriteFile::~OpenWriteFile()
{
    if (file_)
    {
        fclose((FILE*)file_);
        file_ = 0;
    }
}
void OpenWriteFile::setFilePath(const std::string& filePath)
{
    if (file_)
    {
        fclose((FILE*)file_);
        file_ = 0;
    }
    filePath_ = filePath;
    if (!filePath_.empty())
    {
        file_ = fopen(filePath.c_str(), "wb");
        if (!file_)
        {
            printf("OpenWriteFile::setFilePath:%s\n", filePath.data());
            assert(false);
        }
    }
}
size_t OpenWriteFile::write(const char* data, size_t size)
{
    size_t ret = fwrite((void*)data, 1, size, (FILE*)file_);
    fflush((FILE*)file_);
    return ret;
}


///OpenFile
int64_t OpenFile::ReadFile(const std::string& filePath, OpenFileBuffer& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer.resize(len);
    size_t ret = fread((void*)buffer.data(), 1, len, f);
    fclose(f);
    return ret == 0 ? -1 : len;
}

int64_t OpenFile::WriteFile(const std::string& filePath, const OpenFileBuffer& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    size_t ret = fwrite((void*)buffer.data(), 1, buffer.size(), f);
    fclose(f);
    return ret == 0 ? -1 : buffer.size();
}

int64_t OpenFile::ReadFile(const std::string& filePath, std::string& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer.resize(len);
    size_t ret = fread((void*)buffer.data(), 1, len, f);
    fclose(f);
    return ret == 0 ? -1 : len;
}

int64_t OpenFile::WriteFile(const std::string& filePath, const std::string& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    size_t ret = fwrite((void*)buffer.data(), 1, buffer.size(), f);
    fclose(f);
    return ret == 0 ? -1 : buffer.size();
}

int64_t OpenFile::ReadFile(const std::string& filePath, OpenBuffer& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t ret = fread((void*)buffer.clearResize(len), 1, len, f);
    fclose(f);
    return ret == 0 ? -1 : len;
}

int64_t OpenFile::WriteFile(const std::string& filePath, OpenBuffer& buffer, const char* m)
{
    FILE* f = fopen(filePath.c_str(), m);
    if (!f) return -1;
    size_t ret = fwrite((void*)buffer.data(), 1, buffer.size(), f);
    fclose(f);
    return ret == 0 ? -1 : buffer.size();
}


void OpenFile::GetFileExt(const std::string& filePath, std::string& fileName, std::string& fileExt)
{
    std::string::size_type idx = filePath.find(".");
    if(idx != std::string::npos)
    {
        fileName = filePath.substr(0, idx);
        fileExt = filePath.substr(idx);
    }
    else
    {
        fileName = filePath;
    }
}

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)

std::wstring StringMultiByteToWideChar(const std::string& strA)
{
    std::wstring ret;
    if (!strA.empty())
    {
        int nNum = MultiByteToWideChar(CP_ACP, 0, strA.c_str(), -1, NULL, 0);
        if (nNum)
        {
            WCHAR* wideCharString = new WCHAR[nNum + 1];
            wideCharString[0] = 0;
            nNum = MultiByteToWideChar(CP_ACP, 0, strA.c_str(), -1, wideCharString, nNum + 1);
            ret = wideCharString;
            delete[] wideCharString;
        }
        else
        {
            printf("Wrong convert to WideChar code:0x%x \n", GetLastError());
        }
    }
    return ret;
}

std::string StringWideCharToUtf8(const std::wstring& strWideChar)
{
    std::string ret;
    if (!strWideChar.empty())
    {
        int nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, NULL, 0, NULL, FALSE);
        if (nNum)
        {
            char* utf8String = new char[nNum + 1];
            utf8String[0] = 0;

            nNum = WideCharToMultiByte(CP_UTF8, 0, strWideChar.c_str(), -1, utf8String, nNum + 1, NULL, FALSE);

            ret = utf8String;
            delete[] utf8String;
        }
        else
        {
            printf("Wrong convert to Utf8 code:0x%x \n", GetLastError());
        }
    }
    return ret;
}

std::string StringMultiByteToUtf8(const std::string& strA)
{
    std::wstring wret = StringMultiByteToWideChar(strA);
    return StringWideCharToUtf8(wret);
}

std::wstring StringUtf8ToWideChar(const std::string& strUtf8)
{
    std::wstring ret;
    if (!strUtf8.empty())
    {
        int nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
        if (nNum)
        {
            WCHAR* wideCharString = new WCHAR[nNum + 1];
            wideCharString[0] = 0;
            nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wideCharString, nNum + 1);
            ret = wideCharString;
            delete[] wideCharString;
        }
        else
        {
            printf("Wrong convert to WideChar code:0x%x\n", GetLastError());
        }
    }
    return ret;
}



FILE* OpenFile::Open(const char* filePath, const char* mode)
{
    std::string strUtf8 = filePath;
    std::wstring wfilePath = StringUtf8ToWideChar(strUtf8);
    strUtf8 = mode;
    std::wstring wmode = StringUtf8ToWideChar(strUtf8);
    FILE* fp = 0;
    errno_t err = _wfopen_s(&fp, wfilePath.c_str(), wmode.c_str());
    if (!fp)
    {
        return 0;
    }
    return fp;
}

void OpenFile::Close(FILE* fp)
{
    if (fp != 0)
    {
        fclose(fp);
    }
}

static wchar_t* mz_os_unicode_string_create(const char* string, int32_t encoding)
{
    wchar_t* string_wide = NULL;
    uint32_t string_wide_size = 0;

    string_wide_size = MultiByteToWideChar(encoding, 0, string, -1, NULL, 0);
    if (string_wide_size == 0)
        return NULL;
    string_wide = (wchar_t*)malloc((string_wide_size + 1) * sizeof(wchar_t));
    if (string_wide == NULL)
        return NULL;

    memset(string_wide, 0, sizeof(wchar_t) * (string_wide_size + 1));
    MultiByteToWideChar(encoding, 0, string, -1, string_wide, string_wide_size);

    return string_wide;
}

static void mz_os_unicode_string_delete(wchar_t** string)
{
    if (string != NULL) {
        free(*string);
        *string = NULL;
    }
}

#define MZ_ENCODING_CODEPAGE_437        (437)
#define MZ_ENCODING_CODEPAGE_932        (932)
#define MZ_ENCODING_CODEPAGE_936        (936)
#define MZ_ENCODING_CODEPAGE_950        (950)
#define MZ_ENCODING_UTF8                (65001)

bool OpenFile::IsDir(const std::string& filePath)
{
    if (filePath.empty()) return false;
    wchar_t* path_wide = mz_os_unicode_string_create(filePath.c_str(), MZ_ENCODING_UTF8);
    if (path_wide == NULL) return false;
    uint32_t attribs = GetFileAttributesW(path_wide);
    mz_os_unicode_string_delete(&path_wide);
    if (attribs != 0xFFFFFFFF) {
        if (attribs & FILE_ATTRIBUTE_DIRECTORY)
            return true;
    }
    return false;
}

bool OpenFile::MakeDir(const std::string& filePath)
{
    if (filePath.empty()) return false;
    /* Don't try to create a drive letter */
    if ((filePath[0] != 0) && (filePath.size() <= 3) && (filePath[1] == ':'))
    {
        return IsDir(filePath);
    }
    if (IsDir(filePath))
    {
        return true;
    }
    wchar_t* path_wide = mz_os_unicode_string_create(filePath.c_str(), MZ_ENCODING_UTF8);
    if (path_wide == NULL) return false;
    bool err = true;
    if (CreateDirectoryW(path_wide, NULL) == 0) {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            err = false;
    }
    mz_os_unicode_string_delete(&path_wide);
    return err;
}

bool OpenFile::IsFileExist(const std::string& filePath)
{
    if (filePath.empty()) {
        return false;
    }
    std::wstring wStrPath = StringUtf8ToWideChar(filePath);
    DWORD attr = GetFileAttributesW(wStrPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) 
    {
        return false;
    }
    return true;
}

#endif

};
/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/
#include "openjson.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <string>

#define PRINTF printf
#if (defined(_MSC_VER) && (_MSC_VER >= 1400 ))
inline int SNPRINTF(char* buffer, size_t size, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int result = vsnprintf_s(buffer, size, _TRUNCATE, format, va);
    va_end(va);
    return result;
}
#define SSCANF   sscanf_s
#else
#define SNPRINTF snprintf
#define SSCANF   sscanf
#endif

namespace open
{
static inline void doubleToStr(double v, char* buffer, int size)
{
    double tmp = floor(v);
    if (tmp == v) 
        SNPRINTF(buffer, size, "%ld", (long)v);
    else 
        SNPRINTF(buffer, size, "%g", v);
}

static inline bool strToDouble(const char* str, double* value)
{
    return SSCANF(str, "%lf", value) == 1 ? true : false;
}

static void int32ToStr(int32_t n, char* str, size_t size)
{
    if (str == 0 || size < 1) return;
    str[size - 1] = 0;
    if (size < 2) return;
    if (n == 0)
    {
        str[0] = '0';
        return;
    }
    size_t i = 0;
    char buf[128] = { 0 };
    int32_t tmp = n < 0 ? -n : n;
    while (tmp && i < 128)
    {
        buf[i++] = (tmp % 10) + '0';
        tmp = tmp / 10;
    }
    size_t len = n < 0 ? ++i : i;
    if (len > size)
    {
        len = size;
        i = len - 1;
    }
    str[i] = 0;
    while (1)
    {
        --i;
        if (i < 0 || buf[len - i - 1] == 0) break;
        str[i] = buf[len - i - 1];
    }
    if (i == 0) str[i] = '-';
}

static void int64ToStr(int64_t n, char* str, size_t size)
{
    if (str == 0 || size < 1) return;
    str[size - 1] = 0;
    if (size < 2) return;
    if (n == 0)
    {
        str[0] = '0';
        return;
    }
    size_t i = 0;
    char buf[128] = { 0 };
    int64_t tmp = n < 0 ? -n : n;
    while (tmp && i < 128)
    {
        buf[i++] = (tmp % 10) + '0';
        tmp = tmp / 10;
    }
    size_t len = n < 0 ? ++i : i;
    if (len > size)
    {
        len = size;
        i = len - 1;
    }
    str[i] = 0;
    while (1)
    {
        --i;
        if (i < 0 || buf[len - i - 1] == 0) break;
        str[i] = buf[len - i - 1];
    }
    if (i == 0) str[i] = '-';
}

static int32_t strToInt32(const char* str)
{
    const char* ptr = str;
    if (*ptr == '-' || *ptr == '+') ptr++;
    int32_t tmp = 0;
    while (*ptr != 0)
    {
        if ((*ptr < '0') || (*ptr > '9')) break;
        tmp = tmp * 10 + (*ptr - '0');
        ptr++;
    }
    if (*str == '-') tmp = -tmp;
    return tmp;
}

static int64_t strToInt64(const char* str)
{
    const char* ptr = str;
    if (*ptr == '-' || *ptr == '+') ptr++;
    int64_t temp = 0;
    while (*ptr != 0)
    {
        if ((*ptr < '0') || (*ptr > '9')) break;
        temp = temp * 10 + (*ptr - '0');
        ptr++;
    }
    if (*str == '-') temp = -temp;
    return temp;
}


//JsonBox
OpenJson::Box::Box()
{
}

OpenJson::Box::~Box()
{
    for (size_t i = 0; i < childs_.size(); i++)
    {
        if (childs_[i])
        {
            delete childs_[i];
        }
    }
    childs_.clear();
}

bool OpenJson::Box::remove(OpenJson* node)
{
    if (!node) return false;
    std::vector<OpenJson*>::iterator iter;
    for (iter = childs_.begin(); iter != childs_.end(); iter++)
    {
        if (*iter == node)
        {
            childs_.erase(iter);
            delete node;
            return true;
        }
    }
    return false;
}

//JsonContext
OpenJson::Context::Context()
:root_(0),
offset_(0),
data_(0),
size_(0)
{
}

OpenJson::Context::~Context()
{
}

void OpenJson::Context::startRead()
{
    size_ = rbuffer_.size();
    data_ = (char*)rbuffer_.data();
    offset_ = 0;
}

void OpenJson::Context::startWrite()
{
    wbuffer_.clear();
}

//Segment
OpenJson::Segment::Segment(SegmentType type)
{
    setType(type);
}
OpenJson::Segment::~Segment()
{
}
void OpenJson::Segment::setType(SegmentType type)
{
    type_ = type;
    value_.int64_ = 0;
}
void OpenJson::Segment::clear()
{
    value_.int64_  = 0;
}

void OpenJson::Segment::toString()
{
    switch (type_)
    {
    case NIL:
        content_ = "null";
        break;
    case BOOL:
        content_ = value_.bool_ ? "true" : "false";
        break;
    case INT32:
    {
        char buffer[64] = { 0 };
        int32ToStr(value_.int32_, buffer, sizeof(buffer));
        content_ = buffer;
    }
        break;
    case INT64:
    {
        char buffer[64] = { 0 };
        int64ToStr(value_.int64_, buffer, sizeof(buffer));
        content_ = buffer;
    }
        break;
    case DOUBLE:
    {
        char buffer[64] = { 0 };
        doubleToStr(value_.double_, buffer, sizeof(buffer));
        content_ = buffer;
    }
        break;
    case STRING:
        break;
    default:
        content_.clear();
        break;
    }
}

//OpenJson
bool OpenJson::EnableLog_ = true;
OpenJson OpenJson::NodeNull;
std::string OpenJson::StringNull;
OpenJson::OpenJson(JsonType type)
:type_(type),
context_(0),
wcontext_(0),
key_(0),
box_(0),
segment_(0),
idx_(0),
len_(0)
{
}

OpenJson::~OpenJson()
{
    clear();
}

OpenJson* OpenJson::createNode(unsigned char code)
{
    JsonType ctype = UNKNOWN;
    switch (code)
    {
        case '"':
        case '\'':
            ctype = STRING; break;
        case '{':
            ctype = OBJECT; break;
        case '[':
            ctype = ARRAY; break;
        default:
            ctype = NUMBER; break;
    }
    OpenJson* node = new OpenJson(ctype);
    return node;
}

OpenJson::JsonType OpenJson::codeToType(unsigned char code)
{
    JsonType ctype = UNKNOWN;
    switch (code)
    {
    case '"':
    case '\'':
        ctype = STRING; break;
    case '{':
        ctype = OBJECT; break;
    case '[':
        ctype = ARRAY; break;
    default:
        ctype = NUMBER; break;
    }
    return ctype;
}

const std::string& OpenJson::emptyString()
{
    if (context_)
    {
        context_->stringNull_.clear();
        return context_->stringNull_;
    }
    if (wcontext_)
    {
        wcontext_->stringNull_.clear();
        return wcontext_->stringNull_;
    }
    return OpenJson::StringNull;
}

const std::string& OpenJson::key()
{
    if (key_) return key_->s();
    return emptyString();
}

const char* OpenJson::data()
{
    if (context_ && context_->data_)
    {
        if (idx_ < context_->size_)
        {
            return context_->data_ + idx_;
        }
    }
    Log("JsonNode is Empty");
    return emptyString().c_str();
}

double OpenJson::stringToDouble()
{
    const char* str = data();
    double dval = 0;
    if (!str || strlen(str) == 0)
        dval = (float)(1e+300 * 1e+300) * 0.0F;
    else if (strcmp(str, "true") == 0)
        dval = 1.0;
    else if (strcmp(str, "false") == 0)
        dval = 0.0;
    else
        dval = atof(str);
    return dval;
}

int32_t OpenJson::stringToInt32()
{
    int32_t ret = atoi(data());
    return ret;
}

int64_t OpenJson::stringToInt64()
{
    int64_t ret = atoll(data());
    return ret;
}

const std::string& OpenJson::s()
{
    if (type_ == STRING)
    {
        if (!segment_)
        {
            segment_ = new Segment(Segment::STRING);
            segment_->content_ = data();
        }
        if (segment_->type_ == Segment::STRING)
        {
            return segment_->content_;
        }
        segment_->toString();
        return segment_->content_;
    }
    else if (type_ == NUMBER)
    {
        Log("JsonNode is no STRING");
        if (!segment_)
        {
            if (!context_ || !context_->data_ || len_ < 1)
            {
                return emptyString();
            }
            segment_ = new Segment(Segment::NIL);
            segment_->content_ = data();
            return segment_->content_;
        }
        if (segment_)
        {
            if (segment_->type_ != Segment::NIL)
            {
                segment_->toString();
            }
            return segment_->content_;
        }
    }
    else
    {
        Log("JsonNode is no STRING");
    }
    return emptyString();
}

double OpenJson::d(double def)
{
    if (type_ != NUMBER) 
    {
        Log("JsonNode is no NUMBER");
        return def;
    }
    if (segment_ == 0)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            return def;
        }
        segment_ = new Segment(Segment::DOUBLE);
        segment_->value_.double_ = stringToDouble();
    }
    if (segment_->type_ != Segment::DOUBLE)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            Log("JsonNode is no DOUBLE NUMBER");
        }
        else
        {
            segment_->setType(Segment::DOUBLE);
            segment_->value_.double_ = stringToDouble();
        }
    }
    switch (segment_->type_)
    {
    case OpenJson::Segment::BOOL:
        return segment_->value_.bool_;
    case OpenJson::Segment::INT32:
        return (double)segment_->value_.int32_;
    case OpenJson::Segment::INT64:
        return (double)segment_->value_.int64_;
    case OpenJson::Segment::DOUBLE:
        return segment_->value_.double_;
    case OpenJson::Segment::STRING:
        return atof(segment_->content_.c_str());
    default:
        break;
    }
    return def;
}

bool OpenJson::b(bool def)
{
    if (type_ != NUMBER)
    {
        Log("JsonNode is no NUMBER");
        return def;
    }
    if (segment_ == 0)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            return def;
        }
        segment_ = new Segment(Segment::BOOL);
        segment_->value_.bool_ = stringToDouble() != 0 ? true : false;
    }
    if (segment_->type_ != Segment::BOOL)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            Log("JsonNode is no BOOL NUMBER");
        }
        else
        {
            segment_->setType(Segment::BOOL);
            segment_->value_.bool_ = stringToDouble() != 0 ? true : false;
        }
    }
    switch (segment_->type_)
    {
    case OpenJson::Segment::BOOL:
        return segment_->value_.bool_;
    case OpenJson::Segment::INT32:
        return (bool)segment_->value_.int32_;
    case OpenJson::Segment::INT64:
        return (bool)segment_->value_.int64_;
    case OpenJson::Segment::DOUBLE:
        return (bool)segment_->value_.double_;
    case OpenJson::Segment::STRING:
        return segment_->content_.size() > 0;
    default:
        break;
    }
    return def;
}

int32_t OpenJson::i32(int32_t def)
{
    if (type_ != NUMBER)
    {
        Log("JsonNode is no NUMBER");
        return def;
    }
    if (segment_ == 0)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            return def;
        }
        segment_ = new Segment(Segment::INT32);
        segment_->value_.int32_ = stringToInt32();
    }
    if (segment_->type_ != Segment::INT32)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            Log("JsonNode is no INT32 NUMBER");
        }
        else
        {
            segment_->setType(Segment::INT32);
            segment_->value_.int32_ = stringToInt32();
        }
    }
    switch (segment_->type_)
    {
    case OpenJson::Segment::BOOL:
        return segment_->value_.bool_;
    case OpenJson::Segment::INT32:
        return segment_->value_.int32_;
    case OpenJson::Segment::INT64:
        return (int32_t)segment_->value_.int64_;
    case OpenJson::Segment::DOUBLE:
        return (int32_t)segment_->value_.double_;
    case OpenJson::Segment::STRING:
        return atoi(segment_->content_.c_str());
    default:
        break;
    }
    return def;
}

int64_t OpenJson::i64(int64_t def)
{
    if (type_ != NUMBER) 
    {
        Log("JsonNode is no NUMBER");
        return def;
    }
    if (segment_ && segment_->type_ == Segment::NIL)
    {
        delete segment_;
        segment_ = 0;
    }
    if (segment_ == 0)
    {
        if (!context_ || !context_->data_ || len_ < 1)
        {
            return def;
        }
        segment_ = new Segment(Segment::INT64);
        segment_->value_.int64_ = stringToInt64();
    }
    if (segment_->type_ != Segment::INT64)
    {
        Log("JsonNode is no INT64 NUMBER");
    }
    switch (segment_->type_)
    {
    case OpenJson::Segment::BOOL:
        return segment_->value_.bool_;
    case OpenJson::Segment::INT32:
        return segment_->value_.int32_;
    case OpenJson::Segment::INT64:
        return segment_->value_.int64_;
    case OpenJson::Segment::DOUBLE:
        return (int64_t)segment_->value_.double_;
    case OpenJson::Segment::STRING:
        return atoll(segment_->content_.c_str());
    default:
        break;
    }
    return def;
}

void OpenJson::operator=(const std::string& val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != STRING) type_ = STRING;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::STRING);
    const char* ptr = 0;
    for (size_t i = 0; i < val.size(); ++i)
    {
        if (val[i] == '"' || val[i] == '\'')
        {
            segment_->content_.push_back('\\');
        }
        segment_->content_.push_back(val[i]);
    }
}

void OpenJson::operator=(const char* val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != STRING) type_ = STRING;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::STRING);
    segment_->content_.clear();
    const char* ptr = 0;
    for (size_t i = 0; i < strlen(val); ++i)
    {
        ptr = val + i;
        if (*ptr == '"' || *ptr == '\'')
        {
            segment_->content_.push_back('\\');
        }
        segment_->content_.push_back(*ptr);
    }
}

void OpenJson::operator=(bool val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::BOOL);
    segment_->value_.bool_ = val;
}

void OpenJson::operator=(int32_t val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::INT32);
    segment_->value_.int32_ = val;
}

void OpenJson::operator=(uint32_t val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::INT32);
    segment_->value_.int32_ = val;
}

void OpenJson::operator=(int64_t val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::INT64);
    segment_->value_.int64_ = val;
}

void OpenJson::operator=(uint64_t val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container, not element");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::INT64);
    segment_->value_.int64_ = val;
}

void OpenJson::operator=(double val)
{
    if (type_ == OBJECT || type_ == ARRAY)
    {
        Log("JsonNode is a container");
        return;
    }
    if (type_ != NUMBER) type_ = NUMBER;
    if (segment_ == 0) segment_ = new Segment;
    segment_->setType(Segment::DOUBLE);
    segment_->value_.double_ = val;
}

OpenJson& OpenJson::array(size_t idx)
{
    if (type_ != ARRAY)
    {
        if (type_ == OBJECT)
        {
            Log("JsonNode must be ARRAY, not OBJECT");
        }
        type_ = ARRAY;
    }
    else
    {
        assert(box_);
    }
    if (!box_) box_ = new Box;
    if (idx >= box_->childs_.size())
    {
        box_->childs_.resize(idx + 1, 0);
    }
    OpenJson* child = box_->childs_[idx];
    if (!child)
    {
        child = new OpenJson();
        box_->childs_[idx] = child;
    }
    return *child;
}

OpenJson& OpenJson::object(const char* key)
{
    if (!key)
    {
        return NodeNull;
    }
    if (type_ != OBJECT)
    {
        if (type_ == ARRAY)
        {
            Log("JsonNode must be OBJECT, not ARRAY");
        }
        type_ = OBJECT;
    }
    else
    {
        assert(box_);
    }
    if (!box_) box_ = new Box;

    OpenJson* child = 0;
    for (size_t i = 0; i < box_->childs_.size(); ++i)
    {
        child = box_->childs_[i];
        if (child == 0) continue;
        if (strcmp(child->key().c_str(), key) == 0)
        {
            return *child;
        }
    }
    OpenJson* keyNode = new OpenJson(STRING);
    *keyNode = key;
    child = new OpenJson();
    child->key_ = keyNode;
    size_t i = 0;
    for (; i < box_->childs_.size(); ++i)
    {
        if (!box_->childs_[i])
        {
            box_->childs_[i] = child;
            break;
        }
    }
    if (i >= box_->childs_.size())
    {
        box_->childs_.push_back(child);
    }
    return *child;
}

void OpenJson::addNode(OpenJson* node)
{
    if (!node) return;
    if (type_ != OBJECT && type_ != ARRAY)
    {
        Log("JsonNode must be OBJECT or ARRAY");
        type_ = node->key_ ? OBJECT : ARRAY;
    }
    if (box_ == 0) box_ = new Box;
    box_->add(node);
}

void OpenJson::removeNode(size_t idx)
{
    if (box_ == 0) return;
    if (idx >= box_->childs_.size()) return;
    box_->remove(box_->childs_[idx]);
}

void OpenJson::removeNode(const char* key)
{
    if (box_ == 0) return;
    OpenJson* child = 0;
    for (size_t i = 0; i < box_->childs_.size(); ++i)
    {
        child = box_->childs_[i];
        if (child == 0) continue;
        if (strcmp(child->key().c_str(), key) == 0)
        {
            box_->remove(child);
            break;
        }
    }
}

void OpenJson::clear()
{
    if (segment_)
    {
        delete segment_;
        segment_ = 0;
    }
    if (key_)
    {
        delete key_;
        key_ = 0;
    }
    if (box_)
    {
        assert(type_ == OBJECT || type_ == ARRAY);
        delete box_;
        box_ = 0;
    }
    if (context_ != 0 && context_->root_ == this)
    {
        context_->root_ = 0;
        delete context_;
    }
    context_ = 0;
    if (wcontext_ != 0 && wcontext_->root_ == this)
    {
        wcontext_->root_ = 0;
        delete wcontext_;
    }
    wcontext_ = 0;
    type_ = EMPTY;
    idx_ = 0;
    len_ = 0;
}

void OpenJson::trimSpace()
{
    if (!context_) return;
    char code  = 0;
    for (size_t i = idx_; i < context_->size_; ++i)
    {
        code = context_->data_[i];
        if (code > ' ')
        {    
           idx_ = i; break;
        }
    }
}

unsigned char OpenJson::getCharCode()
{
    if (!context_) return 0;
    if (idx_ < context_->size_)
    {
        unsigned char tmp = (unsigned char)context_->data_[idx_];
        return tmp;
    }
    return 0;
}

unsigned char OpenJson::getChar()
{
    unsigned char code = getCharCode();
    if (code <= ' ')
    {
        trimSpace();
        code = getCharCode();
    }
    return code;
}

unsigned char OpenJson::checkCode(unsigned char charCode)
{
    unsigned char code = getCharCode();
    if (code != charCode)
    {
        trimSpace();
        code = getCharCode();
        if (code != charCode) return 0;
    }
    ++idx_;
    return code;
}

size_t OpenJson::searchCode(unsigned char code)
{
    char* data = context_->data_;
    for (size_t i = idx_; i < context_->size_; i++)
    {
        if (data[i] == code)
        {
            if (i > 0 && data[i - 1] != '\\') return i;
        }
    }
    return -1;
}

bool OpenJson::makeRContext()
{
    if (type_ != EMPTY)
    {
        if (context_ && context_->root_ != this)
        {
            PRINTF("OpenJson warn:JsonNode is no root or empty!");
            return false;
        }
    }
    else
    {
        if (context_ && context_->root_ != this)
        {
            PRINTF("OpenJson warn:JsonNode is no root or empty!");
            return false;
        };
    }
    clear();
    context_ = new Context();
    context_->root_ = this;
    context_->offset_ = 0;
    context_->rbuffer_.clear();
    return true;
}

bool OpenJson::decode(const std::string& buffer)
{
    if (!makeRContext()) return false;
    context_->rbuffer_ = buffer;
    context_->startRead();
    type_ = codeToType(getChar());
    try {
        read(context_, true);
    } catch (const char* error) {
        PRINTF("OpenJson warn:decode catch exception %s", error);
    }
    return true;
}

bool OpenJson::decodeFile(const std::string& filePath)
{
    if (!makeRContext()) return false;
    FILE* fp = 0;
#ifdef _MSC_VER
    fopen_s(&fp, filePath.c_str(), "rb");
#else
    fp = fopen(filePath.c_str(), "rb");
#endif
    if (fp == 0)
    {
#ifdef _MSC_VER
        char buffer[1024] = { 0 };
        strerror_s(buffer, sizeof(buffer), errno);
        PRINTF("OpenJson warn:decodeFile error:%s,%s\n", buffer, filePath.c_str());
#else
        PRINTF("OpenJson warn:decodeFile error:%s,%s\n", strerror(errno), filePath.c_str());
#endif
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    size_t ret = 0;
    char buff[1024 * 8] = { 0 };
    while (true)
    {
        ret = fread(buff, 1, sizeof(buff), fp);
        if (ret < 0)
        {
#ifdef _MSC_VER
            char buffer[1024] = { 0 };
            strerror_s(buffer, sizeof(buffer), errno);
            PRINTF("OpenJson warn:decodeFile error:%s,%s\n", buffer, filePath.c_str());
#else
            PRINTF("OpenJson warn:decodeFile error:%s,%s\n", strerror(errno), filePath.c_str());
#endif
            fclose(fp);
            return false;
        }
        else if(ret == 0) break;
        context_->rbuffer_.append(buff, ret);
    }
    fclose(fp);

    context_->startRead();
    type_ = codeToType(getChar());
    try {
        read(context_, true);
    }
    catch (const char* error) 
    {
        PRINTF("OpenJson warn:decodeFile catch exception %s", error);
    }
    return true;
}

const std::string& OpenJson::encode()
{
    if (wcontext_ == 0)
    {
        wcontext_ = new Context();
        wcontext_->root_ = this;
    }
    wcontext_->startWrite();
    write(wcontext_, true);
    return wcontext_->wbuffer_;
}

void OpenJson::encodeFile(const std::string& filePath)
{
    FILE* fp = 0;
#ifdef _MSC_VER
    fopen_s(&fp, filePath.c_str(), "wb");
#else
    fp = fopen(filePath.c_str(), "wb");
#endif
    if (fp == 0)
    {
#ifdef _MSC_VER
        char buffer[1024] = { 0 };
        strerror_s(buffer, sizeof(buffer), errno);
        PRINTF("OpenJson warn:encodeFile error:%s,%s\n", buffer, filePath.c_str());
#else
        PRINTF("OpenJson warn:encodeFile error:%s,%s\n", strerror(errno), filePath.c_str());
#endif
        return;
    }
    fseek(fp, 0, SEEK_SET);
    const std::string& buffer = encode();
    fwrite(buffer.data(), buffer.size(), 1, fp);
    fclose(fp);
}

void OpenJson::read(Context* context, bool isRoot)
{
    if (context_)
    {
        if (isRoot)
        {
            assert(context_ == context);
            assert(context_->root_ == this);
        }
        else
        {
            assert(context_->root_ != this);
            if (context_->root_ == this) return;
        }
    }
    len_     = 0;
    context_ = context;
    idx_     = context->offset_;
    switch (type_)
    {
    case EMPTY:
        break;
    case STRING:
        readString(); break;
    case NUMBER:
        readNumber(); break;
    case OBJECT:
        readObject(); break;
    case ARRAY:
        readArray(); break;
    case UNKNOWN:
        break;
    default:
        break;
    }
}
void OpenJson::readNumber()
{
    assert(type_ == NUMBER);
    unsigned char code = 0;
    size_t sidx = idx_;
    size_t len  = context_->size_;
    char* data  = context_->data_;
    for (; idx_ < len; idx_++)
    {
        code = data[idx_];
        if (code == ',' || code == '}' || code == ']')
        {
            idx_--;
            break;
        }
    }
    if (idx_ < sidx)
    {
        throwError("lost number value");
        return;
    }
    len_ = idx_ - sidx + 1;
    idx_ = sidx;
}
void OpenJson::readString()
{
    assert(type_ == STRING);
    unsigned char code = '"';
    if (!checkCode(code))
    {
        code = '\'';
        if (!checkCode(code))
        {
            throwError("lost '\"' or \"'\"");
            return;
        }
    }
    size_t sidx = idx_;
    size_t eidx = searchCode(code);
    if (eidx < 0)
    {
        throwError("lost '\"' or \"'\"");
        return;
    }
    idx_ = sidx;
    len_ = eidx - sidx + 1;
    context_->data_[eidx] = 0;
}
void OpenJson::readObject()
{
    assert(type_ == OBJECT);
    if (!checkCode('{'))
    {
        throwError("lost '{'");
        return;
    }
    unsigned char code = 0;
    OpenJson* keyNode  = 0;
    OpenJson* valNode  = 0;
    size_t oidx = idx_;
    while (idx_ < context_->size_)
    {
        code = getChar();
        if (code == 0)
        {
            throwError("lost '}'");
            return;
        }
        if (checkCode('}')) break;
        keyNode = createNode(code);
        if (keyNode->type_ != STRING)
        {
            throwError("lost key");
            return;
        }
        context_->offset_ = idx_;
        keyNode->read(context_);
        idx_ = keyNode->idx_ + keyNode->len_;
        if (!checkCode(':'))
        {
            throwError("lost ':'");
            return;
        }
        code    = getChar();
        valNode = createNode(code);
        valNode->key_ = keyNode;
        context_->offset_ = idx_;
        valNode->read(context_);
        idx_ = valNode->idx_ + valNode->len_;
        addNode(valNode);

        if (checkCode('}'))
        {
            context_->data_[idx_ - 1] = 0;
            break;
        }
        if (!checkCode(','))
        {
            throwError("lost ','");
            return;
        }
        context_->data_[idx_ - 1] = 0;
    }
    len_ = idx_ - oidx;
    idx_ = oidx;
}
void OpenJson::readArray()
{
    assert(type_ == ARRAY);
    if (!checkCode('['))
    {
        throwError("lost '['");
        return;
    }
    unsigned char code = 0;
    OpenJson* valNode  = 0;
    size_t oidx = idx_;
    while (idx_ < context_->size_)
    {
        code = getChar();
        if (code == 0)
        {
            throwError("lost ']'");
            return;
        }
        if (checkCode(']')) break;
        valNode = createNode(code);
        context_->offset_ = idx_;
        valNode->read(context_);
        idx_ = valNode->idx_ + valNode->len_;
        addNode(valNode);

        if (checkCode(']'))
        {
            context_->data_[idx_ - 1] = 0;
            break;
        }
        if (!checkCode(','))
        {
            throwError("lost ','");
            return;
        }
        context_->data_[idx_ - 1] = 0;
    }
    len_ = idx_ - oidx;
    idx_ = oidx;
}

void OpenJson::write(Context* context, bool isRoot)
{
    if (wcontext_)
    {
        if (isRoot)
        {
            assert(wcontext_ == context);
            assert(wcontext_->root_ == this);
        }
        else
        {
            assert(wcontext_->root_ != this);
            if (wcontext_->root_ == this) return;
        }
    }
    wcontext_ = context;
    switch (type_)
    {
    case EMPTY:
        break;
    case STRING:
        writeString(); break;
    case NUMBER:
        writeNumber(); break;
    case OBJECT:
        writeObject(); break;
    case ARRAY:
        writeArray(); break;
    case UNKNOWN:
        break;
    default:
        break;
    }
}
void OpenJson::writeNumber()
{
    assert(type_ == NUMBER);
    if (key_)
    {
        wcontext_->wbuffer_.append("\"" + key() + "\":");
    }
    if (segment_)
    {
        segment_->toString();
        wcontext_->wbuffer_.append(segment_->content_);
    }
    else
    {
        wcontext_->wbuffer_.append(data());
    }
}
void OpenJson::writeString()
{
    assert(type_ == STRING);
    if (key_)
    {
        wcontext_->wbuffer_.append("\"" + key() + "\":");
    }
    wcontext_->wbuffer_.append("\"" + s() + "\"");
}
void OpenJson::writeObject()
{
    assert(type_ == OBJECT);
    if (key_)
        wcontext_->wbuffer_.append("\"" + key() + "\":{");
    else
        wcontext_->wbuffer_.append("{");

    if (box_ != 0)
    {
        size_t idx = 0;
        size_t size = box_->size();
        for (size_t i = 0; i < size; ++i)
        {
            if (!(*box_)[i]) continue;
            if (idx > 0)
            {
                wcontext_->wbuffer_.append(",");
            }
            (*box_)[i]->write(wcontext_);
            ++idx;
        }
    }
    wcontext_->wbuffer_.append("}");
}
void OpenJson::writeArray()
{
    assert(type_ == ARRAY);
    if (key_)
        wcontext_->wbuffer_.append("\"" + key() + "\":[");
    else
        wcontext_->wbuffer_.append("[");
    
    if (box_ != 0)
    {
        size_t idx  = 0;
        size_t size = box_->size();
        for (size_t i = 0; i < size; ++i)
        {
            if (!(*box_)[i]) continue;
            if (idx > 0)
            {
                wcontext_->wbuffer_.append(",");
            }
            (*box_)[i]->write(wcontext_);
            ++idx;
        }
    }
    wcontext_->wbuffer_.append("]");
}

void OpenJson::EnableLog(bool enable)
{
    EnableLog_ = enable;
}

void OpenJson::Log(const char* format, ...)
{
    if (!EnableLog_) return;
    va_list ap;
    va_start(ap, format);
    char tmp[1024] = { 0 };
    vsnprintf(tmp, sizeof(tmp), format, ap);
    va_end(ap);
    PRINTF("OpenJson WARN:%s\n", tmp);
}
void OpenJson::throwError(const char* errMsg)
{
    static const char* InfoTags[6] = { "EMPTY", "STRING", "NUMBER", "OBJECT", "ARRAY", "UNKNOWN" };
    size_t len = sizeof(InfoTags) / sizeof(InfoTags[0]);
    const char* tab = type_ < len ? InfoTags[type_] : InfoTags[5];
    PRINTF("OpenJson:throwError [%s] Error: %s\n", tab, errMsg);

    char tmp[126] = { 0 };
    len = context_->size_ - context_->offset_;
    len = len > 64 ? 64 : len;
    memcpy(tmp, context_->data_ + idx_, len);
    PRINTF("OpenJson:throwError content:%s\n", tmp);
    throw errMsg;
}
};
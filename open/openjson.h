/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_JSON_H
#define HEADER_OPEN_JSON_H

#include <string>
#include <stddef.h>
#include <stdint.h>
#include <vector>

#ifdef _MSC_VER
#if _MSC_VER >= 1600 || defined(__MINGW32__)
#else
#if (_MSC_VER < 1300)
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed int        int32_t;
typedef unsigned char     uint8_t;
typedef unsigned short    uint16_t;
typedef unsigned int      uint32_t;
#else
typedef signed __int8     int8_t;
typedef signed __int16    int16_t;
typedef signed __int32    int32_t;
typedef unsigned __int8   uint8_t;
typedef unsigned __int16  uint16_t;
typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif
#endif // _MSC_VER

namespace open
{
class OpenJson
{
    enum JsonType
    {
        EMPTY = 0,
        STRING,
        NUMBER,
        OBJECT,
        ARRAY,
        UNKNOWN
    };
    class Box
    {
        std::vector<OpenJson*> childs_;
    public:
        Box();
        ~Box();
        inline void clear() { childs_.clear(); }
        inline bool empty() { return childs_.empty(); }
        inline size_t size() { return childs_.size(); }
        inline void add(OpenJson* node){ childs_.push_back(node); }
        inline OpenJson* operator[](size_t idx){ return childs_[idx]; }
        bool remove(OpenJson* node);
        friend class OpenJson;
    };
    class Context
    {
        char* data_;
        size_t size_;
        size_t offset_;

        OpenJson* root_;
        std::string rbuffer_;
        std::string wbuffer_;

        std::string stringNull_;
    public:
        Context();
        ~Context();
        void startRead();
        void startWrite();
        friend class OpenJson;
    };
    class Segment
    {
    public:
        enum SegmentType
        {
            NIL = 0,
            BOOL,
            INT32,
            INT64,
            DOUBLE,
            STRING
        };
        SegmentType type_;
        std::string content_;
        union {
            bool bool_;
            int32_t int32_;
            int64_t int64_;
            double double_;
        } value_;

        Segment(SegmentType type = NIL);
        ~Segment();

        void clear();
        void toString();
        void setType(SegmentType type);
    };

    JsonType type_;
    Context* context_;
    Context* wcontext_;
    size_t idx_;
    size_t len_;
    Box* box_;
    OpenJson* key_;
    Segment* segment_;

    void trimSpace();
    bool makeRContext();
    OpenJson* createNode(unsigned char code);
    JsonType codeToType(unsigned char code);
    unsigned char getCharCode();
    unsigned char getChar();
    unsigned char checkCode(unsigned char charCode);
    size_t searchCode(unsigned char charCode);
    void throwError(const char* errMsg);

    const char* data();
    int32_t stringToInt32();
    int64_t stringToInt64();
    double stringToDouble();

    OpenJson& array(size_t idx);
    OpenJson& object(const char* key);
    void addNode(OpenJson* node);
    void removeNode(size_t idx);
    void removeNode(const char* key);
    OpenJson(OpenJson& json) {}
    OpenJson(const OpenJson& json) {}
    void operator=(OpenJson& json) {}
    void operator=(const OpenJson& json) {}

    const std::string& emptyString();
    static OpenJson NodeNull;
    static std::string StringNull;
public:
    OpenJson(JsonType type = EMPTY);
    ~OpenJson();
    
    inline size_t size() { return box_ ? box_->size() : 0; }
    inline bool empty() { return box_ ? box_->empty() : true; }
    inline OpenJson& operator[] (int idx) { return array(idx); }
    inline OpenJson& operator[] (size_t idx) { return array(idx); }
    inline OpenJson& operator[] (const char* key) { return object(key); }
    inline OpenJson& operator[] (const std::string& key) { return object(key.c_str()); }

    inline void remove(int idx) { removeNode(idx); }
    inline void remove(size_t idx) { removeNode(idx); }
    inline void remove(const char* key) { removeNode(key); }
    inline void remove(const std::string& key) { removeNode(key.c_str()); }
    void clear();

    inline bool isNull() { return type_ == EMPTY; }
    inline bool isNumber() { return type_ == NUMBER; }
    inline bool isString() { return type_ == STRING; }
    inline bool isObject() { return type_ == OBJECT; }
    inline bool isArray() { return type_ == ARRAY; }

    bool b(bool def = false);
    int32_t i32(int32_t def = 0);
    int64_t i64(int64_t def = 0);
    double d(double def = 0);
    const std::string& s();
    const std::string& key();

    void operator=(bool val);
    void operator=(int32_t val);
    void operator=(uint32_t val);
    void operator=(int64_t val);
    void operator=(uint64_t val);
    void operator=(double val);
    void operator=(const char* val);
    void operator=(const std::string& val);
    
    bool decode(const std::string& buffer);
    bool decodeFile(const std::string& filePath);
    const std::string& encode();
    void encodeFile(const std::string& filePath);

    static void EnableLog(bool enable);
private:
    static bool EnableLog_;
    static void Log(const char* format, ...);
    void read(Context* context, bool isRoot = false);
    void readNumber();
    void readString();
    void readObject();
    void readArray();

    void write(Context* context, bool isRoot = false);
    void writeNumber();
    void writeString();
    void writeObject();
    void writeArray();
};
};
#endif /* HEADER_OPEN_JSON_H */

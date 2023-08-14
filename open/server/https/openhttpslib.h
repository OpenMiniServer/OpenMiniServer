/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_HTTPS_LIB_H
#define HEADER_OPEN_HTTPS_LIB_H

#include <assert.h>
#include <memory>
#include <string.h>
#include <map>
#include <vector>
#include <string>
#include "../../openbuffer.h"
#include "../../openserver.h"

namespace open
{

////////////OpenHttp//////////////////////
struct OpenHttp
{
    int fd_;
    int pid_;
    int uid_;
    int code_;
    int64_t clen_;
    bool isReq_;
    bool isHttps_;
    bool isChunked_;
    bool isClient_;
    bool isRemote_;
    bool isFinish_;
    std::string domain_;
    unsigned int port_;
    std::string method_;
    std::string path_;
    std::string host_;
    std::string url_;
    std::string ctype_;
    OpenBuffer head_;
    OpenBuffer body_;
    OpenBuffer buffer_;
    std::vector<std::string> paths_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> headers_;

    void splitPaths();
    const std::string& lookIp();
    inline const std::string& ip() { return ip_; }
    inline void setIp(const std::string& ip) { ip_ = ip; }

    inline void getHead(std::string& head) { head.append((const char*)head_.data(), head_.size()); }
    inline void getBody(std::string& body) { body.append((const char*)body_.data(), body_.size()); }
    inline std::string& setParam(const std::string& key) { return params_[key]; }
    std::string& header(const std::string& key);
    bool hasHeader(const std::string& key);
    void removeHeader(const std::string& key);
    inline std::string& operator[](const std::string& key) { return header(key); }

    OpenHttp();
    void parseUrl();
    void encodeReqHeader();
    void encodeRespHeader();
    virtual void decodeReqHeader();
    void decodeRespHeader();
    bool responseData(const char* data, size_t size);
    bool requestData(const char* data, size_t size);
    inline void operator=(const std::string& url) { url_ = url; }

    //bool send(void* data, size_t size);
    void clear()
    {
        code_ = -1;
        clen_ = 0;
        params_.clear();
        head_.clear();
        body_.clear();
        buffer_.clear();
    }

private:
    std::string ip_;
};

////////////OpenHttpResponse//////////////////////
struct OpenHttpResponse : public OpenHttp
{
    OpenHttpResponse() : OpenHttp() { isReq_ = false; }
    virtual void decodeReqHeader();
    void init();
    void response(const char* ctype, const char* buffer, size_t len);
    void response(const char* ctype, const std::string& buffer);
    void response(int code, const std::string& ctype, const std::string& buffer);
    void response404Html();
    void send();
    static const std::string GetContentType(const char* fileExt);
private:
    static const std::map<std::string, std::string> ContentTypes_;
};

////////////OpenHttpRequest//////////////////////
class OpenHttpRequest : public OpenHttp
{
    OpenHttpResponse response_;
public:
    OpenHttpRequest(bool isClient = true) : OpenHttp(), listenPort_(0) { isReq_ = true; if (isClient) init(); }
    OpenHttpResponse* rep() { return &response_; }
    void init()
    {
        port_ = 80;
        headers_["accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng";
        headers_["user-agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36(KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36";
        headers_["cache-control"] = "max-age=0";
        //headers_["accept-encoding"] = "gzip,deflate";
        headers_["accept-language"] = "en-us,zh;q=0.9";
        headers_["connection"] = "close";
    }
    int listenPort_;
};

typedef bool (*OpenHttpHandle)(OpenHttpRequest&, OpenHttpResponse&);

////////////OpenHttpClientMsg//////////////////////
struct OpenHttpClientMsg : public OpenMsgProtoMsg
{
    int type_;
    int custom_;
    std::shared_ptr<OpenHttpRequest> request_;

    static inline int MsgType() { return 0; }
    OpenHttpClientMsg() :OpenMsgProtoMsg(), type_(MsgType()) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpClientMsg::MsgId(); }
};

////////////OpenHttpClientSyncMsg//////////////////////
struct OpenHttpClientSyncMsg : public OpenHttpClientMsg
{
    OpenSync openSync_;
    static inline int MsgType() { return 1; }
    OpenHttpClientSyncMsg() :OpenHttpClientMsg() { type_ = MsgType(); }
};

////////////OpenHttpClientStreamMsg//////////////////////
struct OpenHttpClientStreamMsg : public OpenHttpClientMsg
{
    int pid_;
    int fd_;
    uint64_t uid_;
    static inline int MsgType() { return 2; }
    OpenHttpClientStreamMsg() :OpenHttpClientMsg(), pid_(-1), fd_(-1), uid_(-1) { type_ = MsgType(); }
};

////////////OpenHttpClientStreamNoticeMsg//////////////////////
struct OpenHttpClientStreamNoticeMsg : public OpenMsgProtoMsg
{
    int type_;
    int reqPid_;
    int reqFd_;
    int pid_;
    int fd_;
    uint64_t uid_;
    OpenBuffer buffer_;
    OpenHttpClientStreamNoticeMsg() : OpenMsgProtoMsg(), reqPid_(-1), reqFd_(-1), pid_(-1), fd_(-1), uid_(-1), type_(0) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpClientStreamNoticeMsg::MsgId(); }
};


/////////////////////////////OpenHttpServe//////////////////////
////////////////////////////////////////////////////////////////

////////////OpenHttpServerMsg//////////////////////
struct OpenHttpServerMsg : public OpenMsgProtoMsg
{
    bool isHttps_;
    std::string ip_;
    unsigned int port_;
    unsigned int port1_;
    std::string keyFile_;
    std::string certFile_;
    OpenHttpHandle handle_;
    std::vector<int> vectAccepts_;
    std::vector<int> vectClients_;

    OpenHttpServerMsg() :isHttps_(false), port_(80), port1_(0), handle_(0) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpServerMsg::MsgId(); }
};

////////////OpenHttpRegisterMsg//////////////////////
struct OpenHttpRegisterMsg : public OpenMsgProtoMsg
{
    int fd_;
    OpenHttpServerMsg serverInfo_;

    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpRegisterMsg::MsgId(); }
};

////////////OpenHttpAcceptMsg//////////////////////
struct OpenHttpAcceptMsg : public OpenMsgProtoMsg
{
    bool isHttps_;
    int clientId_;
    std::string ip_;
    unsigned int port_;
    unsigned int port1_;
    OpenHttpHandle handle_;

    OpenHttpAcceptMsg() : isHttps_(false), port_(80), port1_(0), handle_(0), clientId_(-1) {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpAcceptMsg::MsgId(); }
};

////////////OpenHttpNoticeMsg//////////////////////
struct OpenHttpNoticeMsg : public OpenMsgProtoMsg
{
    int fd_;
    OpenHttpAcceptMsg clientInfo_;

    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpNoticeMsg::MsgId(); }
};

////////////OpenHttpSendResponseMsg//////////////////////
struct OpenHttpSendResponseMsg : public OpenMsgProtoMsg
{
    int fd_;

    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return OpenHttpSendResponseMsg::MsgId(); }
};

};


#endif //HEADER_OPEN_HTTPS_LIB_H

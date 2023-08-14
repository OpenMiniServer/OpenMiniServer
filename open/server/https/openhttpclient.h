/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_HTTP_CLIENT_H
#define HEADER_OPEN_HTTP_CLIENT_H

#include <vector>
#include <set>
#include <string.h>
#include <unordered_map>
#include "openssl.h"
#include "openhttpslib.h"
#include "../../openfile.h"

namespace open
{

////////////OpenHttpClient//////////////////////
class OpenHttpClient : public OpenSocketServer
{
protected:
    struct Client
    {
        int fd_;
        int pid_;
        OpenHttpClient* server_;

        bool isClosed_;
        TlsContext* tls_;
        SslCtx* sslCtx_;
        TlsBuffer buffer_;
        std::shared_ptr<OpenHttpClientMsg> clientMsg_;
        OpenHttpClientStreamMsg* streamMsg_;

        bool isOpenCache_;
        bool isCreateSendFile_;
        OpenWriteFile cacheSendFile_;
        bool isCreateReceiveFile_;
        OpenWriteFile cacheReceiveFile_;

        Client();
        ~Client();
        bool start(std::shared_ptr<OpenHttpClientMsg>& clientMsg);
        void sendRequest();
        void sendRawBuffer();
        void sendBuffer();
        void open();
        void update(const char* data, size_t size);
        void onOpen();
        void onData(const char* data, size_t size);
        void close();
    };
    std::unordered_map<int, Client> mapFdToClient_;

    virtual void onSocketData(const OpenSocketMsg& msg);
    virtual void onSocketClose(const OpenSocketMsg& msg);
    virtual void onSocketOpen(const OpenSocketMsg& msg);
    virtual void onSocketError(const OpenSocketMsg& msg);
    virtual void onSocketWarning(const OpenSocketMsg& msg);
    virtual void onTimerProto(const OpenTimerProto& proto);

    SslCtx sslCtx_;
public:
    OpenHttpClient(const std::string& serverName, const std::string& args) :OpenSocketServer(serverName, args), sslCtx_(false) {}

    virtual ~OpenHttpClient();
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new OpenHttpClient(serverName, args);
    }
    void onMsgProto(OpenMsgProto& proto);

    bool sendHttp(std::shared_ptr<OpenHttpRequest>& request);
    virtual void onHttp(OpenHttpRequest& req, OpenHttpResponse& rep) {}

};


};


#endif   //HEADER_OPEN_COM_HTTP_CLIENT_H

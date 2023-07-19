/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_HTTP_SERVER_H
#define HEADER_OPEN_HTTP_SERVER_H

#include <vector>
#include <set>
#include <memory>
#include <unordered_map>
#include <string.h>
#include "openssl.h"
#include "openhttpslib.h"
#include "../../openfile.h"


namespace open
{


////////////OpenHttpServer//////////////////////
class OpenHttpServer : public OpenSocketServer
{
    OpenHttpServerMsg serverInfo_;

    int listen_fd_;
    int listen_fd1_;
    unsigned int balance_;
    std::set<int> setAgentId_;
    std::vector<int> vectAgentId_;

    virtual void onSocketAccept(const OpenSocketMsg& msg);
    virtual void onSocketClose(const OpenSocketMsg& msg);
    virtual void onSocketOpen(const OpenSocketMsg& msg);
    virtual void onSocketError(const OpenSocketMsg& msg);
    virtual void onSocketWarning(const OpenSocketMsg& msg);

public:
    OpenHttpServer(const std::string& serverName, const std::string& args);

    OpenHttpServer();
    virtual ~OpenHttpServer() {}
    virtual void onMsgProto(OpenMsgProto& proto);
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new OpenHttpServer(serverName, args);
    }
};

////////////OpenHttpAgent//////////////////////
class OpenHttpAgent : public OpenSocketServer
{
    struct Client
    {
        int fd_;
        int pid_;
        TlsContext* tls_;
        SslCtx* sslCtx_;
        TlsBuffer buffer_;
        OpenHttpAcceptMsg clientInfo_;
        std::shared_ptr<OpenHttpRequest> request_;
        OpenHttpAgent* server_;

        bool isOpenCache_;
        bool isCreateSendFile_;
        OpenWriteFile cacheSendFile_;
        bool isCreateReceiveFile_;
        OpenWriteFile cacheReceiveFile_;

        Client();
        ~Client();
        bool start();
        void sendClient();
        void sendResponse();
        void sendRawBuffer();
        void sendBuffer();
        void open();
        void update(const char* data, size_t size);
        void onOpen();
        void onData(const char* data, size_t size);
        void close();

        void onClientClientData(int reqPid, const char* data, size_t size);
        void onClientClientClose(int reqPid);
    };

    OpenHttpServerMsg serverInfo_;

    SslCtx sslCtx_;
    std::unordered_map<int, Client> mapClient_;

    virtual void onSocketData(const OpenSocketMsg& msg);
    virtual void onSocketClose(const OpenSocketMsg& msg);
    virtual void onSocketOpen(const OpenSocketMsg& msg);
    virtual void onSocketError(const OpenSocketMsg& msg);
    virtual void onSocketWarning(const OpenSocketMsg& msg);
public:
    OpenHttpAgent(const std::string& serverName, const std::string& args) :OpenSocketServer(serverName, args), sslCtx_(true) {}
    virtual ~OpenHttpAgent() {}
    void onMsgProto(OpenMsgProto& proto);
    virtual inline OpenServer* newServer(const std::string& serverName, const std::string& args)
    {
        return new OpenHttpAgent(serverName, args);
    }
    virtual void onHttp(OpenHttpRequest& req, OpenHttpResponse& rep);
};


};


#endif   //HEADER_OPEN_HTTP_SERVER_H

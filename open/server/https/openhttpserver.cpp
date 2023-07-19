//#include <map>
//#include <set>
//#include <memory>
//#include <string.h>
#include "openhttpserver.h"

namespace open
{

////////////OpenHttpServer//////////////////////
OpenHttpServer::OpenHttpServer(const std::string& serverName, const std::string& args)
    :OpenSocketServer(serverName, args), balance_(-1),
    listen_fd_(-1),
    listen_fd1_(-1) {}

void OpenHttpServer::onMsgProto(OpenMsgProto& proto)
{
    if (!proto.msg_) return;
    if (OpenHttpServerMsg::MsgId() == proto.msg_->msgId())
    {
        std::shared_ptr<OpenHttpServerMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpServerMsg>(proto.msg_);
        if (!protoMsg)
        {
            assert(false);
            return;
        }
        serverInfo_ = *protoMsg;
        assert(serverInfo_.port_ > 0 && serverInfo_.port_ < 0xffff);
        if (listen_fd_ >= 0)
        {
            assert(false);
            OpenSocket::Instance().close(pid_, listen_fd_);
        }
        listen_fd_ = OpenSocket::Instance().listen((uintptr_t)pid_, serverInfo_.ip_, serverInfo_.port_, 64);
        if (listen_fd_ < 0)
        {
            printf("OpenHttpServer listen failed. addr:%s:%d\n", serverInfo_.ip_.c_str(), serverInfo_.port_);
            assert(false);
            return;
        }
        OpenSocket::Instance().start((uintptr_t)pid_, listen_fd_);
        printf("OpenHttpServer Listen: %s:%d\n", serverInfo_.ip_.c_str(), serverInfo_.port_);
        assert(serverInfo_.port_ != serverInfo_.port1_);
        if (serverInfo_.port1_ > 0 && serverInfo_.port1_ < 0xffff)
        {
            if (listen_fd1_ >= 0)
            {
                assert(false);
                OpenSocket::Instance().close(pid_, listen_fd1_);
            }
            listen_fd1_ = OpenSocket::Instance().listen((uintptr_t)pid_, serverInfo_.ip_, serverInfo_.port1_, 64);
            if (listen_fd1_ < 0)
            {
                printf("OpenHttpServer listen failed. addr:%s:%d\n", serverInfo_.ip_.c_str(), serverInfo_.port1_);
                assert(false);
                return;
            }
            OpenSocket::Instance().start((uintptr_t)pid_, listen_fd1_);
            printf("OpenHttpServer Listen1: %s:%d\n", serverInfo_.ip_.c_str(), serverInfo_.port1_);
        }
        vectAgentId_ = serverInfo_.vectAccepts_;
        {
            auto protoMsg = std::shared_ptr<OpenHttpRegisterMsg>(new OpenHttpRegisterMsg);
            protoMsg->serverInfo_ = serverInfo_;
            auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
            proto->msg_ = protoMsg;
            int sendPid = 0;
            for (size_t i = 0; i < vectAgentId_.size(); i++)
            {
                sendPid = vectAgentId_[i];
                if (!send(sendPid, proto))
                {
                    printf("OpenHttpServer. send faild pid = %d\n", sendPid);
                }
            }
        }
    }
}

void OpenHttpServer::onSocketAccept(const OpenSocketMsg& msg)
{
    int accept_fd = msg.ud_;
    const std::string addr = msg.data();
    if (!vectAgentId_.empty())
    {
        size_t idx = addr.find(":");
        if (idx != std::string::npos)
        {
            auto protoMsg = std::shared_ptr<OpenHttpNoticeMsg>(new OpenHttpNoticeMsg);
            protoMsg->fd_ = accept_fd;

            protoMsg->clientInfo_.ip_ = addr.substr(0, idx);
            protoMsg->clientInfo_.port_ = atoi(addr.data() + idx + 1);
            protoMsg->clientInfo_.handle_ = serverInfo_.handle_;
            protoMsg->clientInfo_.isHttps_ = serverInfo_.isHttps_;
            if (msg.fd_ == listen_fd_)
            {
                protoMsg->clientInfo_.port1_ = serverInfo_.port_;
            }
            else
            {
                protoMsg->clientInfo_.port1_ = serverInfo_.port1_;
            }
            protoMsg->clientInfo_.clientId_ = -1;
            if (!serverInfo_.vectClients_.empty())
            {
                protoMsg->clientInfo_.clientId_ = serverInfo_.vectClients_[std::rand() % serverInfo_.vectClients_.size()];
            }
            if (balance_ >= vectAgentId_.size())
                balance_ = 0;
            int agentId = vectAgentId_[balance_++];
            auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
            proto->msg_ = protoMsg;
            if (send(agentId, proto))
            {
                printf("OpenHttpServer::accept:%s\n", addr.c_str());
                return;
            }
        }
    }
    printf("OpenHttpServer::accept close:%s\n", addr.c_str());
    OpenSocket::Instance().close(pid_, accept_fd);
}
void OpenHttpServer::onSocketClose(const OpenSocketMsg& msg) {}
void OpenHttpServer::onSocketOpen(const OpenSocketMsg& msg) {}
void OpenHttpServer::onSocketError(const OpenSocketMsg& msg)
{
    printf("OpenHttpServer::onSocketError [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
}
void OpenHttpServer::onSocketWarning(const OpenSocketMsg& msg)
{
    printf("OpenHttpServer::onSocketWarning [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
}

////////////OpenHttpAgent::Client//////////////////////
OpenHttpAgent::Client::Client()
    : fd_(-1), 
    pid_(-1), 
    tls_(0), 
    sslCtx_(0),
    server_(0),
    request_(std::shared_ptr<OpenHttpRequest>(new OpenHttpRequest(false)))
{
    isOpenCache_ = false;
    isCreateSendFile_ = false;
    isCreateReceiveFile_ = false;
}
OpenHttpAgent::Client::~Client()
{
    if (tls_) delete tls_;
}
bool OpenHttpAgent::Client::start()
{
    assert(sslCtx_);
    assert(request_);
    request_->clear();
    request_->setIp(clientInfo_.ip_);
    request_->port_ = clientInfo_.port_;
    request_->isHttps_ = clientInfo_.isHttps_;
#ifdef USE_OPEN_SSL
    assert(!clientInfo_.isHttps_);
#endif
    printf("OpenHttpAgent::Client::start[%s:%d]\n", request_->ip().c_str(), request_->port_);
    return true;
}
void OpenHttpAgent::Client::sendClient()
{
    request_->isClient_ = true;
    std::string url = (*request_)["client"];
    auto request = std::shared_ptr<OpenHttpRequest>(new OpenHttpRequest);
    request->method_ = request_->method_;
    request->url_ = url;
    if (request_->body_.size() > 0)
    {
        request->body_.pushBack(request_->body_.data(), request_->body_.size());
    }
    auto protoMsg = std::shared_ptr<OpenHttpClientStreamMsg>(new OpenHttpClientStreamMsg);
    protoMsg->pid_ = pid_;
    protoMsg->fd_  = fd_;
    protoMsg->uid_ = 8888;
    protoMsg->request_ = request;
    auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
    proto->msg_ = protoMsg;
    printf("OpenHttpAgent::Client::sendClient. pid_ = %d. send_pid = %d\n", pid_, clientInfo_.clientId_);
    if (!server_->send(clientInfo_.clientId_, proto))
    {
        printf("OpenHttpAgent::Client::sendClient. send faild. send_pid = %d\n", clientInfo_.clientId_);
        assert(false);
    }
}
void OpenHttpAgent::Client::sendResponse()
{
    auto& response = *request_->rep();
    if (request_->hasHeader("client"))
    {
        if (clientInfo_.clientId_ >= 0)
        {
            sendClient();
            return;
        }
        response.response(".html", "<html><body><h1>client close</h1></body></html>");
    }
    else
    {
        response.init();
        request_->listenPort_ = clientInfo_.port1_;
        request_->splitPaths();
        if (server_)
        {
            server_->onHttp(*request_, response);
        }
    }
    response.encodeRespHeader();
    buffer_.clear();
    buffer_.push(response.head_.data(), response.head_.size());
    buffer_.push(response.body_.data(), response.body_.size());
    buffer_.push("\r\n", strlen("\r\n"));
    if (buffer_.size() > 0)
    {
        sendBuffer();
    }
    else
    {
        assert(false);
    }

    //buffer_.clear();
    //buffer_.push("\r\n", strlen("\r\n"));
    //if (buffer_.size() > 0)
    //{
    //    sendBuffer();
    //}
    //else
    //{
    //    assert(false);
    //}

    //OpenSocket::Instance().close(pid_, fd_);
}
void OpenHttpAgent::Client::open()
{
    printf("OpenComHttpAgent::Client::open[%s:%d]\n", request_->ip().c_str(), request_->port_);
#ifdef USE_OPEN_SSL
    assert(!clientInfo_.isHttps_);
#endif
    if (!clientInfo_.isHttps_)
    {
        tls_ = 0;
        onOpen();
    }
    else
        tls_ = new TlsContext(sslCtx_, true);
}
void OpenHttpAgent::Client::onOpen()
{
}
void OpenHttpAgent::Client::onData(const char* data, size_t size)
{
    if (isOpenCache_)
    {
        if (!isCreateReceiveFile_)
        {
            OpenTime openTime;
            isCreateReceiveFile_ = true;
            std::string filePath = "./cache/agent_receive_" + std::to_string((int64_t)(void*)this) + "_" + openTime.toString("%Y_%M_%D_%h_%m_%s");
            cacheReceiveFile_.setFilePath(filePath);
        }
        cacheReceiveFile_.write(data, size);
    }
    if (request_->isFinish_)
    {
        return;
    }
    if (request_->requestData(data, size))
    {
        request_->isFinish_ = true;
        sendResponse();
    }
}
void OpenHttpAgent::Client::update(const char* data, size_t size)
{
    //printf("\nOpenHttpAgent::Client::update[%s:%d] size = %lld\n", request_->ip().data(), request_->port_, size);
    if (!tls_)
    {
        onData(data, size);
        return;
    }
    if (!tls_->isFinished())
    {
        //printf("OpenHttpAgent::Client::update[%s:%d] handshake1, size:%lld\n", request_->ip().data(), request_->port_, size);
        buffer_.clear();
        int ret = tls_->handshake(data, size, &buffer_);
        if (ret > 0)
        {
            if (buffer_.size() > 0)
            {
                sendRawBuffer();
            }
        }
        else if (ret == 0)
        {
            assert(buffer_.size() == 0);
            if (tls_->isFinished())
            {
                onOpen();

                buffer_.clear();
                tls_->read(0, 0, &buffer_);
                if (buffer_.size() > 0)
                {
                    onData((const char*)buffer_.data(), buffer_.size());
                }
            }
        }
        else
        {
            if (buffer_.size() > 0)
            {
                sendRawBuffer();
            }
        }
        //printf("OpenHttpAgent::Client::update[%s:%d] handshake2, size:%lld\n", request_->ip().data(), request_->port_, size);
    }
    else
    {
        //printf("OpenHttpAgent::Client::update[%s:%d] read1, size:%lld\n", request_->ip().data(), request_->port_, size);
        buffer_.clear();
        tls_->read(data, size, &buffer_);
        //printf("OpenHttpAgent::Client::update[%d] read2, size:%lld\n", request_->port_, size);
        if (buffer_.size() > 0)
        {
            onData((const char*)buffer_.data(), buffer_.size());
        }
    }
}
void OpenHttpAgent::Client::sendBuffer()
{
    if (isOpenCache_)
    {
        if (!isCreateSendFile_)
        {
            OpenTime openTime;
            isCreateSendFile_ = true;
            std::string filePath = "./cache/agent_send_" + std::to_string((int64_t)(void*)this) + "_" + openTime.toString("%Y_%M_%D_%h_%m_%s");
            cacheReceiveFile_.setFilePath(filePath);
        }
        cacheReceiveFile_.write((const char*)buffer_.data(), buffer_.size());
    }
    if (buffer_.size() == 0) return;
    int ret = 0;
    if (!tls_)
    {
        ret = OpenSocket::Instance().send(fd_, buffer_.data(), (int)buffer_.size());
    }
    else
    {
        TlsBuffer buffer;
        tls_->write((char*)buffer_.data(), buffer_.size(), &buffer);
        ret = OpenSocket::Instance().send(fd_, buffer.data(), (int)buffer.size());
    }
    if (ret > 0)
    {
        printf("[WARN]OpenHttpAgent::Client::sendBuffer ret = %d\n", ret);
        //assert(false);
    }
}
void OpenHttpAgent::Client::sendRawBuffer()
{
    if (buffer_.size() == 0) return;
    //printf("OpenHttpAgent::Client::[sendBuffer] size = %lld\n", buffer_.size());
    int ret = OpenSocket::Instance().send(fd_, buffer_.data(), (int)buffer_.size());
    if (ret > 0)
    {
        printf("[WARN]OpenHttpAgent::Client::sendRawBuffer ret = %d\n", ret);
        //assert(false);
    }
}
void OpenHttpAgent::Client::close()
{
    printf("OpenHttpAgent::Client::close[%s:%d]\n", request_->ip().c_str(), request_->port_);
}
void OpenHttpAgent::Client::onClientClientData(int reqPid, const char* data, size_t size)
{
    assert(clientInfo_.clientId_ == reqPid);
    buffer_.clear();
    buffer_.push(data, size);
    if (buffer_.size() > 0)
    {
        sendBuffer();
    }
    else
    {
        assert(false);
    }
}
void OpenHttpAgent::Client::onClientClientClose(int reqPid)
{
    assert(clientInfo_.clientId_ == reqPid);

    buffer_.clear();
    buffer_.push("\r\n", strlen("\r\n"));
    if (buffer_.size() > 0)
    {
        sendBuffer();
    }
    else
    {
        assert(false);
    }
    printf("OpenHttpAgent::Client::onClientClientClose[%s:%d]\n", request_->ip().c_str(), request_->port_);
    //OpenSocket::Instance().close(pid_, fd_);
}

////////////OpenHttpAgent//////////////////////
//bool OpenHttpAgent::start()
//{
//    if (!OpenSocketServer::start())
//    {
//        return false;
//    }
//    return true;
//}

void OpenHttpAgent::onMsgProto(OpenMsgProto& proto)
{
    if (!proto.msg_) return;
    if (OpenHttpClientStreamNoticeMsg::MsgId() == proto.msg_->msgId())
    {
        std::shared_ptr<OpenHttpClientStreamNoticeMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpClientStreamNoticeMsg>(proto.msg_);
        if (!protoMsg)
        {
            assert(false);
            return;
        }
        printf("OpenHttpAgent::onMsgProto type=%d\n", protoMsg->type_);
        if (protoMsg->pid_ != pid_)
        {
            assert(false);
            return;
        }
        auto iter = mapClient_.find(protoMsg->fd_);
        if (iter == mapClient_.end())
        {
            assert(false);
            return;
        }
        auto& client = iter->second;
        assert(client.fd_ == protoMsg->fd_);
        assert(client.pid_ == pid_);
        assert(protoMsg->uid_ == 8888);
        if (protoMsg->type_ == 0)
        {
            client.onClientClientData(protoMsg->reqPid_, (const char*)protoMsg->buffer_.data(), protoMsg->buffer_.size());
        }
        else
        {
            client.onClientClientClose(protoMsg->reqPid_);
        }
    }
    else if (OpenHttpNoticeMsg::MsgId() == proto.msg_->msgId())
    {
        std::shared_ptr<OpenHttpNoticeMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpNoticeMsg>(proto.msg_);
        if (!protoMsg)
        {
            assert(false);
            return;
        }
        int fd = protoMsg->fd_;
        auto iter = mapClient_.find(fd);
        if (iter != mapClient_.end())
        {
            assert(false);
            mapClient_.erase(iter);
            OpenSocket::Instance().close(pid_, fd);
            return;
        }
        auto& client = mapClient_[fd];
        client.server_ = this;
        client.fd_ = fd;
        client.pid_ = pid_;
        client.sslCtx_ = &sslCtx_;
        client.clientInfo_ = protoMsg->clientInfo_;
        client.start();
        //printf("OpenComHttpAgent::onMsgProto OpenHttpNoticeMsg fd=%d, pid_=%d\n", fd, pid_);

        OpenSocket::Instance().start(pid_, fd);
    }
    else if (OpenHttpRegisterMsg::MsgId() == proto.msg_->msgId())
    {
        std::shared_ptr<OpenHttpRegisterMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpRegisterMsg>(proto.msg_);
        if (!protoMsg)
        {
            assert(false);
            return;
        }
        serverInfo_ = protoMsg->serverInfo_;
        if (serverInfo_.isHttps_)
        {
            sslCtx_.setCert(serverInfo_.certFile_.data(), serverInfo_.keyFile_.data());
        }
    }
}

void OpenHttpAgent::onHttp(OpenHttpRequest& req, OpenHttpResponse& rep)
{
    if (serverInfo_.handle_)
    {
        serverInfo_.handle_(req, rep);
    }
}

void OpenHttpAgent::onSocketData(const OpenSocketMsg& msg)
{
    auto iter = mapClient_.find(msg.fd_);
    if (iter == mapClient_.end())
    {
        OpenSocket::Instance().close(pid_, msg.fd_);
        return;
    }
    auto& client = iter->second;
    assert(client.fd_ == msg.fd_);
    assert(client.pid_ == pid_);
    client.update(msg.data(), msg.size());
}

void OpenHttpAgent::onSocketClose(const OpenSocketMsg& msg)
{
    auto iter = mapClient_.find(msg.fd_);
    if (iter != mapClient_.end())
    {
        auto& client = iter->second;
        assert(client.fd_ == msg.fd_);
        assert(client.pid_ == pid_);
        client.close();

        mapClient_.erase(iter);
    }
}
void OpenHttpAgent::onSocketOpen(const OpenSocketMsg& msg)
{
    auto iter = mapClient_.find(msg.fd_);
    if (iter == mapClient_.end())
    {
        OpenSocket::Instance().close(pid_, msg.fd_);
        return;
    }
    auto& client = iter->second;
    //printf("OpenHttpAgent::onSocketOpen fd=%d, pid_=%d, client.fd_=%d, client.pid_=%d\n", msg.fd_, pid_, client.fd_, client.pid_);
    assert(client.fd_ == msg.fd_);
    assert(client.pid_ == pid_);
    client.open();
}
void OpenHttpAgent::onSocketError(const OpenSocketMsg& msg)
{
    printf("OpenHttpAgent::onSocketError [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
    auto iter = mapClient_.find(msg.fd_);
    if (iter != mapClient_.end())
    {
        auto& client = iter->second;
        assert(client.fd_ == msg.fd_);
        assert(client.pid_ == pid_);
        client.close();

        mapClient_.erase(iter);
    }
}

void OpenHttpAgent::onSocketWarning(const OpenSocketMsg& msg)
{
    printf("OpenHttpAgent::onSocketWarning [%s]:%s, warn_size=%d\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info(), msg.ud_);
    assert(false);
}


};
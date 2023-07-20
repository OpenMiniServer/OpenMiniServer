
#include "openhttpclient.h"

namespace open
{

////////////OpenHttpClient::Client//////////////////////
    OpenHttpClient::Client::Client() : fd_(-1), pid_(-1), tls_(0), sslCtx_(0), server_(0)
{
    streamMsg_ = 0;
    isClosed_ = false;

    isOpenCache_ = false;
    isCreateSendFile_ = false;
    isCreateReceiveFile_ = false;
}

    OpenHttpClient::Client::~Client()
{ 
    if (tls_)
    {
        delete tls_;
        tls_ = 0;
    }
}

bool OpenHttpClient::Client::start(std::shared_ptr<OpenHttpClientMsg>& clientMsg)
{
    assert(server_);
    assert(sslCtx_);
    clientMsg_ = clientMsg;
    streamMsg_ = 0;
    isClosed_ = false;
    if (clientMsg_ && clientMsg_->type_ == OpenHttpClientStreamMsg::MsgType())
    {
        streamMsg_ = dynamic_cast<OpenHttpClientStreamMsg*>(clientMsg_.get());
    }
    auto request = clientMsg_->request_;
    request->rep()->clear();
#ifndef USE_OPEN_SSL
    assert(!request->isHttps_);
#endif
    if (!request->isHttps_)
        tls_ = 0;
    else
        tls_ = new TlsContext(sslCtx_, false, request->host_.c_str());
    return true;
}

void OpenHttpClient::Client::sendRequest()
{
    auto& request = clientMsg_->request_;
    request->encodeReqHeader();
    buffer_.clear();
   
    auto& head = request->head_;
    buffer_.push(head.data(), head.size());
    //
    auto& body = request->body_;
    if (body.size() > 0)
    {
        buffer_.push(body.data(), (int)body.size());
    }
    buffer_.push("\r\n", (int)strlen("\r\n"));
    if (buffer_.size() > 0)
    {
        sendBuffer();
    }
    else
    {
        assert(false);
    }
}

void OpenHttpClient::Client::open()
{
    //printf("[HTTPClient]open[%s:%d]\n", clientMsg_->request_->domain_.c_str(), clientMsg_->request_->port_);
    if (!tls_)
    {
        onOpen();
        return;
    }
    buffer_.clear();
    if (tls_->handshake(0, 0, &buffer_) > 0)
    {
        sendRawBuffer();
    }
    else
    {
        assert(buffer_.size() == 0);
    }
}

void OpenHttpClient::Client::update(const char* data, size_t size)
{
    //printf("=========================>>\nOpenComHttpClient::Client::update size = %lld\n", size);
    if (isClosed_) return;
    if (!tls_)
    {
        onData(data, size);
        return;
    }
    if (!tls_->isFinished())
    {
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
            assert(buffer_.size() == 0);
        }
        return;
    }
    else
    {
        buffer_.clear();
        tls_->read(data, size, &buffer_);
        if (buffer_.size() > 0)
        {
            onData((const char*)buffer_.data(), buffer_.size());
        }
    }
    return;
}

void OpenHttpClient::Client::onOpen()
{
    sendRequest();
}

void OpenHttpClient::Client::onData(const char* data, size_t size)
{
    if (isOpenCache_)
    {
        if (!isCreateReceiveFile_)
        {
            OpenTime openTime;
            isCreateReceiveFile_ = true;
            std::string filePath = "./cache/client_receive_" + std::to_string((int64_t)(void*)this) + "_" + openTime.toString("%Y_%M_%D_%h_%m_%s");
            cacheReceiveFile_.setFilePath(filePath);
        }
        cacheReceiveFile_.write(data, size);
    }
    if (streamMsg_)
    {
        //printf("OpenComHttpClient::Client::onData[%s:%d] stream size=%d\n", clientMsg_->request_->domain_.c_str(), clientMsg_->request_->port_, (int)size);
        assert(clientMsg_->type_ == OpenHttpClientStreamMsg::MsgType());
        auto protoMsg = std::shared_ptr<OpenHttpClientStreamNoticeMsg>(new OpenHttpClientStreamNoticeMsg);
        protoMsg->reqPid_ = pid_;
        protoMsg->reqFd_ = fd_;
        protoMsg->pid_ = streamMsg_->pid_;
        protoMsg->fd_  = streamMsg_->fd_;
        protoMsg->uid_ = streamMsg_->uid_;
        protoMsg->type_ = 0;
        protoMsg->buffer_.pushBack(data, size);
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = protoMsg;
        //printf("[HTTPClient]onData[%s:%d] stream send_pid=%d\n",
        //    clientMsg_->request_->domain_.c_str(), clientMsg_->request_->port_, streamMsg_->pid_);
        if (!server_->send(streamMsg_->pid_, proto))
        {
            printf("[HTTPClient]onData. send faild send_pid = %d\n", streamMsg_->pid_);
            assert(false);
        }
        return;
    }
    auto response = clientMsg_->request_->rep();
    if (response->isFinish_)
    {
        return;
    }
    if (response->responseData(data, size))
    {
        response->isFinish_ = true;
        close();
        //OpenSocket::Instance().close(pid_, fd_);
    }
}

void OpenHttpClient::Client::sendBuffer()
{
    if (isOpenCache_)
    {
        if (!isCreateSendFile_)
        {
            OpenTime openTime;
            isCreateSendFile_ = true;
            std::string filePath = "./cache/client_send_" + std::to_string((int64_t)(void*)this) + "_" + openTime.toString("%Y_%M_%D_%h_%m_%s");
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
        printf("[WARN][HTTPClient]sendBuffer ret = %d\n", ret);
    }
}

void OpenHttpClient::Client::sendRawBuffer()
{
    if (buffer_.size() == 0) return;
    //printf("OpenComHttpClient::Client::[sendBuffer] size = %lld\n", buffer_.size());
    int ret = OpenSocket::Instance().send(fd_, buffer_.data(), (int)buffer_.size());
    if (ret > 0)
    {
        printf("[WARN][HTTPClient]sendRawBuffer ret = %d\n", ret);
        //assert(false);
    }
}

void OpenHttpClient::Client::close()
{
    if (isClosed_) return;
    isClosed_ = true;
    if (streamMsg_)
    {
        assert(clientMsg_->type_ == OpenHttpClientStreamMsg::MsgType());
        auto protoMsg = std::shared_ptr<OpenHttpClientStreamNoticeMsg>(new OpenHttpClientStreamNoticeMsg);
        protoMsg->reqPid_ = pid_;
        protoMsg->reqFd_ = fd_;
        protoMsg->pid_ = streamMsg_->pid_;
        protoMsg->fd_ = streamMsg_->fd_;
        protoMsg->uid_ = streamMsg_->uid_;
        protoMsg->type_ = 1;
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = protoMsg;
        //printf("[HTTPClient]close[%s:%d] stream send_pid = %d\n", 
        //    clientMsg_->request_->domain_.c_str(), clientMsg_->request_->port_, streamMsg_->pid_);
        if (!server_->send(streamMsg_->pid_, proto))
        {
            printf("[HTTPClient]close. send faild send_pid = %d\n", streamMsg_->pid_);
            assert(false);
        }
        return;
    }
    //printf("[HTTPClient]close[%s:%d]\n", clientMsg_->request_->domain_.c_str(), clientMsg_->request_->port_);
    if (clientMsg_->type_ == OpenHttpClientSyncMsg::MsgType())
    {
        std::shared_ptr<OpenHttpClientSyncMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpClientSyncMsg>(clientMsg_);
        if (protoMsg)
        {
            protoMsg->openSync_.wakeup();
        }
    }
    else if (clientMsg_->type_ == OpenHttpClientMsg::MsgType())
    {
        if (server_)
        {
            auto request = clientMsg_->request_;
            if (request)
            {
                server_->onHttp(*request, *request->rep());
            }
        }
    }
    else
    {
        assert(false);
    }
}

////////////OpenHttpClient//////////////////////
OpenHttpClient::~OpenHttpClient()
{
    auto iter = mapFdToClient_.begin();
    for (;iter != mapFdToClient_.end(); iter++)
        iter->second.close();

    mapFdToClient_.clear();
}

void OpenHttpClient::onMsgProto(OpenMsgProto& proto)
{
    if (!proto.msg_) return;
    if (OpenHttpClientMsg::MsgId() == proto.msg_->msgId())
    {
        std::shared_ptr<OpenHttpClientMsg> protoMsg = std::dynamic_pointer_cast<OpenHttpClientMsg>(proto.msg_);
        if (!protoMsg)
        {
            assert(false);
            return;
        }
        auto& request = protoMsg->request_;
        //printf("====>>OpenComHttpClient pid:%d, uid:%d\n", pid_, request->uid_);

        request->parseUrl();
        request->lookIp();
        // printf("[HTTPClient]OpenHttpClientMsg [%s]domain:%s, ip:%s\n", pname_.data(), request->domain_.data(), request->ip().data());
        if (request->ip().empty())
        {
            printf("[HTTPClient]OpenHttpClientMsg ip error. [%s]domain:%s, ip:%s\n", name_.data(), request->domain_.data(), request->ip().data());
            if (protoMsg->type_ == OpenHttpClientSyncMsg::MsgType())
            {
                std::shared_ptr<OpenHttpClientSyncMsg> protoMsg1 = std::dynamic_pointer_cast<OpenHttpClientSyncMsg>(protoMsg);
                if (protoMsg1)
                {
                    protoMsg1->openSync_.wakeup();
                }
            }
            else if (protoMsg->type_ == OpenHttpClientMsg::MsgType())
            {
            }
            return;
        }
        int fd = OpenSocket::Instance().connect(pid_, request->ip(), request->port_);
        auto& client = mapFdToClient_[fd];
        assert(client.fd_ == -1);
        client.server_ = this;
        client.fd_  = fd;
        client.pid_ = pid_;
        client.sslCtx_ = &sslCtx_;
        client.start(protoMsg);
    }
}

void OpenHttpClient::onSocketData(const OpenSocketMsg& msg)
{
    auto iter = mapFdToClient_.find(msg.fd_);
    if (iter == mapFdToClient_.end())
    {
        OpenSocket::Instance().close(pid_, msg.fd_);
        return;
    }
    auto& client = iter->second;
    assert(client.fd_ == msg.fd_);
    assert(client.pid_ == pid_);
    client.update(msg.data(), msg.size());
}
void OpenHttpClient::onSocketOpen(const OpenSocketMsg& msg)
{
    auto iter = mapFdToClient_.find(msg.fd_);
    if (iter == mapFdToClient_.end())
    {
        OpenSocket::Instance().close(pid_, msg.fd_);
        return;
    }
    auto& client = iter->second;
    assert(client.fd_ == msg.fd_);
    assert(client.pid_ == pid_);
    client.open();
}
void OpenHttpClient::onSocketClose(const OpenSocketMsg& msg)
{
    auto iter = mapFdToClient_.find(msg.fd_);
    if (iter != mapFdToClient_.end())
    {
        auto& client = iter->second;
        assert(client.fd_ == msg.fd_);
        assert(client.pid_ == pid_);
        client.close();

        mapFdToClient_.erase(iter);
    }
    //printf("====>>OpenHttpClient:onSocketClose  pid:%d, msg.fd_:%d\n", pid_, msg.fd_);
}
void OpenHttpClient::onSocketError(const OpenSocketMsg& msg)
{
    auto iter = mapFdToClient_.find(msg.fd_);
    if (iter != mapFdToClient_.end())
    {
        auto& client = iter->second;
        assert(client.fd_ == msg.fd_);
        assert(client.pid_ == pid_);
        client.close();

        mapFdToClient_.erase(iter);
    }
    printf("[HTTPClient]onSocketError [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
}
void OpenHttpClient::onSocketWarning(const OpenSocketMsg& msg)
{
    printf("[HTTPClient]onSocketWarning [%s]:%s, warn_size=%d\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info(), msg.ud_);
    assert(false);
}

void OpenHttpClient::onTimerProto(const OpenTimerProto& proto)
{
    //startInterval(3000);
}

bool OpenHttpClient::sendHttp(std::shared_ptr<OpenHttpRequest>& request)
{
    if (!request)
    {
        return false;
    }

    request->parseUrl();
    request->lookIp();
    if (request->ip().empty())
    {
        return false;
    }
    int fd = OpenSocket::Instance().connect(pid_, request->ip(), request->port_);
    auto& client = mapFdToClient_[fd];
    assert(client.fd_ == -1);
    client.server_ = this;
    client.fd_ = fd;
    client.pid_ = pid_;
    client.sslCtx_ = &sslCtx_;

    auto protoMsg = std::shared_ptr<OpenHttpClientMsg>(new OpenHttpClientMsg);
    protoMsg->request_ = request;
    client.start(protoMsg);
    return true;
}

};
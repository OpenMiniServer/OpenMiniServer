#include <string.h>
#include "openserver.h"
#include "opentime.h"

namespace open
{

////////////OpenServerPool//////////////////////
OpenServer::OpenServerPool OpenServer::OpenServerPool_;
OpenServer::OpenServerPool::~OpenServerPool()
{
    spinLock_.lock();
    mapClassCreate_.clear();
    auto iter1 = mapNameServer_.begin();
    for (; iter1 != mapNameServer_.end(); iter1++)
    {
        delete iter1->second;
    }
    mapNameServer_.clear();
    vectServer_.clear();
    spinSLock_.unlock();
}

OpenServer* OpenServer::OpenServerPool::startServer(const std::string& className, const std::string& serverName, const std::string& args)
{
    OpenServer* server = NULL;
    spinSLock_.lock();
    auto iter1 = mapNameServer_.find(serverName);
    if (iter1 != mapNameServer_.end())
    {
        spinSLock_.unlock();
        assert(false);
        server = iter1->second;
        return server;
    }
    auto iter = mapClassCreate_.find(className);
    if (iter == mapClassCreate_.end())
    {
        printf("OpenServerPool::startServer [%s] is not exist!\n", className.c_str());
        spinLock_.unlock();
        assert(false);
        return false;
    }
    
    server = iter->second(serverName, args);
    server->className_ = className;
    mapNameServer_[serverName] = server;
    vectServer_.push_back(server);
    spinSLock_.unlock();

    server->init();
    return server;
}

bool OpenServer::OpenServerPool::runAllServers()
{
    for (auto iter = vectServer_.begin(); iter != vectServer_.end(); iter++)
    {
        (*iter)->onInit();
    }
    for (auto iter = vectServer_.begin(); iter != vectServer_.end(); iter++)
    {
        (*iter)->start();
    }
    return true;
}

OpenServer* OpenServer::OpenServerPool::findServer(const std::string& serverName)
{
    OpenServer* server = NULL;
    spinSLock_.lock();
    auto iter = mapNameServer_.find(serverName);
    if (iter != mapNameServer_.end())
    {
        server = iter->second;
    }
    spinSLock_.unlock();
    return server;
}

void OpenServer::OpenServerPool::findServersByClassName(const std::string& className, std::vector<OpenServer*>& vectServer)
{
    OpenServer* server = NULL;
    spinSLock_.lock();
    for (auto iter = mapNameServer_.begin(); iter != mapNameServer_.end(); iter++)
    {
        server = iter->second;
        if (server->className_ == className)
        {
            vectServer.push_back(server);
        }
    }
    spinSLock_.unlock();
}


////////////OpenServer//////////////////////
OpenServer::OpenServer(const std::string& name, const std::string& args)
    :OpenThreadWorker(name),
    args_(args)
{
    registers(OpenMsgProto::ProtoType(), (OpenThreadHandle)&OpenServer::onMsgProto);
    registers(OpenTimerProto::ProtoType(), (OpenThreadHandle)&OpenServer::onTimerProto);
    registers(OpenSocketProto::ProtoType(), (OpenThreadHandle)&OpenServer::onSocketProto);
}
OpenServer::~OpenServer()
{
}

bool OpenServer::init()
{
    auto threadRef = OpenThread::Thread(name_);
    if (threadRef)
    {
        printf("OpenServer::init. [%s] is exist\n", name_.c_str());
        assert(false);
        return false;
    }
    threadRef = OpenThread::Create(name_);
    assert(threadRef);
    pid_ = -1;
    if (threadRef)
    {
        thread_ = OpenThread::GetThread(threadRef);
        pid_ = thread_->pid();
        assert(!thread_->isRunning());
        thread_->setCustom(this);
        return true;
    }
    return false;
}

bool OpenServer::start()
{
    auto threadRef = OpenThread::Thread(name_);
    if (!threadRef)
    {
        printf("OpenServer::init. [%s] is not exist\n", name_.c_str());
        assert(false);
        return false;
    }
    thread_ = OpenThread::GetThread(threadRef);
    assert(pid_ == thread_->pid());
    assert(!thread_->isRunning());
    thread_->setCustom(this);
    thread_->start(OpenThreader::Thread);
    return true;
}

void OpenServer::onMsgProto(OpenMsgProto& proto)
{
    if (pid_ == -1)
    {
        assert(pid_ != 0);
    }
}
void OpenServer::onTimerProto(const OpenTimerProto& proto)
{
    if (pid_ == -1)
    {
        printf("Need start OpenServer");
        assert(pid_ != 0);
    }
}
void OpenServer::onSocketProto(const OpenSocketProto& proto)
{
    if (pid_ == -1)
    {
        printf("Need start OpenServer");
        assert(pid_ != 0);
    }
}
bool OpenServer::startTime(int64_t deadline)
{
    return OpenTimerServer::StartTime(pid_, deadline);
}
bool OpenServer::startInterval(int64_t interval)
{
    return OpenTimerServer::StartInterval(pid_, interval);
}

OpenServer* OpenServer::StartServer(const std::string& className, const std::string& serverName, const std::string& args)
{
    return OpenServerPool_.startServer(className, serverName, args);
}

void OpenServer::RunServers()
{
    OpenServerPool_.runAllServers();
}

OpenServer* OpenServer::FindServer(const std::string& serverName)
{
    return OpenServerPool_.findServer(serverName);
}

void OpenServer::GetServersByClassName(const std::string& className, std::vector<open::OpenServer*>& vectServer)
{
    OpenServerPool_.findServersByClassName(className, vectServer);
}

void OpenServer::GetServers(const std::vector<std::string>& serverNames, std::vector<open::OpenServer*>& servers, int times)
{
    int count = times;
    open::OpenServer* server = 0;
    for (size_t i = 0; i < serverNames.size(); i++)
    {
        auto& serverName = serverNames[i];
        count = times;
        while (!server && count-- > 0)
        {
            server = open::OpenServer::FindServer(serverName);
            if (!server)
            {
                open::OpenThread::Sleep(200);
            }
            else
            {
                servers.push_back(server);
            }
        }
        server = 0;
    }
}


////////////OpenSocketServer//////////////////////
void OpenSocketServer::onSocketProto(const OpenSocketProto& proto)
{
    if (!proto.data_)
    {
        assert(false);
        return;
    }
    const OpenSocketMsg& msg = *proto.data_;
    switch (msg.type_)
    {
    case OpenSocket::ESocketData:
        onSocketData(msg); break;
    case OpenSocket::ESocketClose:
        onSocketClose(msg); break;
    case OpenSocket::ESocketError:
        onSocketError(msg); break;
    case OpenSocket::ESocketWarning:
        onSocketWarning(msg); break;
    case OpenSocket::ESocketOpen:
        onSocketOpen(msg); break;
    case OpenSocket::ESocketAccept:
        onSocketAccept(msg); break;
    case OpenSocket::ESocketUdp:
        onSocketUdp(msg); break;
    default:
        assert(false);
        break;
    }
}

void OpenSocketServer::onSocketData(const OpenSocketMsg& msg)
{
    assert(false);
}

void OpenSocketServer::onSocketClose(const OpenSocketMsg& msg)
{
    assert(false);
}

void OpenSocketServer::onSocketError(const OpenSocketMsg& msg)
{
    printf("OpenSocketServer::onSocketError [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
}

void OpenSocketServer::onSocketWarning(const OpenSocketMsg& msg)
{
    printf("OpenSocketServer::onSocketWarning [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), msg.info());
}

void OpenSocketServer::onSocketOpen(const OpenSocketMsg& msg)
{
    assert(false);
}

void OpenSocketServer::onSocketAccept(const OpenSocketMsg& msg)
{
    int accept_fd = msg.ud_;
    const std::string addr = msg.data();
    printf("OpenSocketServer::onSocketAccept [%s]:%s\n", OpenThread::ThreadName((int)msg.uid_).c_str(), addr.c_str());
}

void OpenSocketServer::onSocketUdp(const OpenSocketMsg& msg)
{
    assert(false);
}

////////////OpenTimerServer//////////////////////
void OpenTimerServer::onStart()
{
    auto proto = std::shared_ptr<OpenTimerProto>(new OpenTimerProto);
    proto->type_ = -1;
    send(pid_, proto);
}
void OpenTimerServer::onTimerProto(const OpenTimerProto& proto)
{
    if (proto.type_ >= 0)
    {
        //printf("OpenTimer::onOpenTimerProto[%d]%s\n", proto.srcPid_, OpenTime::MilliToString(proto.deadline_).data());
        mapTimerEvent_.insert({ proto.deadline_, proto.srcPid_ });
    }
    else
    {
        assert(proto.srcPid_ == pid_);
    }
    timeLoop();
}
void OpenTimerServer::timeLoop()
{
    int64_t curTime = OpenTime::MilliUnixtime();
    while (canLoop())
    {
        if (!mapTimerEvent_.empty())
        {
            while (!mapTimerEvent_.empty())
            {
                auto iter = mapTimerEvent_.begin();
                if (curTime < iter->first) break;

                auto proto = std::shared_ptr<OpenTimerProto>(new OpenTimerProto);
                proto->curTime_ = curTime;
                proto->deadline_ = iter->first;
                send(iter->second, proto);
                mapTimerEvent_.erase(iter);
            }
        }
        OpenThread::Sleep(timeInterval_);
        curTime += timeInterval_;
    }
}
bool OpenTimerServer::StartTime(int pid, int64_t deadline)
{
    assert(pid >= 0 && deadline >= 0);
    return TimerPool_.startTime(pid, deadline);
}
bool OpenTimerServer::StartInterval(int pid, int64_t interval)
{
    assert(pid >= 0 && interval >= 0);
    return TimerPool_.startTime(pid, OpenThread::MilliUnixtime() + interval);
}

////////////TimerPool//////////////////////
OpenTimerServer::TimerPool::TimerPool()
    :timer_(0)
{
}
OpenTimerServer::TimerPool::~TimerPool()
{
    if (timer_)
    {
        delete timer_;
        timer_ = 0;
    }
}
void OpenTimerServer::TimerPool::run()
{
    if (timer_) return;
    timer_ = new OpenTimerServer("OpenTimerServer");
    timer_->init();
    timer_->start();
}
bool OpenTimerServer::TimerPool::startTime(int pid, int64_t deadline)
{
    if (!timer_)
    {
        assert(false);
        return false;
    }
    auto proto = std::shared_ptr<OpenTimerProto>(new OpenTimerProto);
    proto->deadline_ = deadline;
    proto->srcPid_ = pid;
    bool ret = OpenThread::Send(timer_->pid(), proto);
    assert(ret);
    return ret;
}
OpenTimerServer::TimerPool OpenTimerServer::TimerPool_;


////////////OpenApp//////////////////////
void OpenApp::SocketFunc(const OpenSocketMsg* msg)
{
    if (!msg) return;
    if (msg->uid_ >= 0)
    {
        auto proto = std::shared_ptr<OpenSocketProto>(new OpenSocketProto);
        proto->srcPid_ = -1;
        proto->srcName_ = "OpenSocket";
        proto->data_ = std::shared_ptr<OpenSocketMsg>((OpenSocketMsg*)msg);
        if (!OpenThread::Send((int)msg->uid_, proto))
            printf("SocketFunc dispatch faild pid = %d\n", (int)msg->uid_);
    }
    else delete msg;
}

OpenApp OpenApp::OpenApp_;
OpenApp::OpenApp()
    :isRunning_(false)
{
}

OpenApp::~OpenApp()
{
}

void OpenApp::start()
{
    if (isRunning_) return;
    isRunning_ = true;
    OpenSocket::Start(OpenApp::SocketFunc);
}

void OpenApp::wait()
{
    OpenThread::ThreadJoinAll();
}

};
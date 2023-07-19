/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_OPEN_SERVER_H
#define HEADER_OPEN_SERVER_H

#include "opensocket.h"
#include "openthread.h"
#include "opentime.h"
#include "openjson.h"
#include <string.h>
#include <stdarg.h>
#include <map>

namespace open
{

////////////OpenSocketProto//////////////////////
struct OpenSocketProto : public OpenThreadProto
{
    std::shared_ptr<OpenSocketMsg> data_;
    static inline int ProtoType() { return 8888; }
    virtual inline int protoType() const { return OpenSocketProto::ProtoType(); }
};

////////////OpenTimerProto//////////////////////
struct OpenTimerProto : public OpenThreadProto
{
    int type_;
    int64_t deadline_;
    int64_t curTime_;
    OpenTimerProto() :OpenThreadProto(), type_(0), deadline_(0), curTime_(0) {}
    static inline int ProtoType() { return 8886; }
    virtual inline int protoType() const { return OpenTimerProto::ProtoType(); }
};

////////////OpenMsgProto//////////////////////
struct OpenMsgProtoMsg
{
    virtual ~OpenMsgProtoMsg() {}
    static inline int MsgId() { return (int)(int64_t)(void*) & MsgId; }
    virtual inline int msgId() const { return OpenMsgProtoMsg::MsgId(); }
};
struct OpenMsgProto : public OpenThreadProto
{
    virtual ~OpenMsgProto() {}
    std::shared_ptr<OpenMsgProtoMsg> msg_;
    static inline int ProtoType() { return 8887; }
    virtual inline int protoType() const { return OpenMsgProto::ProtoType(); }
};


////////////OpenServer//////////////////////
class OpenServer : public OpenThreadWorker
{
public:
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        assert(false);
        return new OpenServer(serverName, args);
    }
protected:
    class SpinLock
    {
    private:
        SpinLock(const SpinLock&) {};
        void operator=(const SpinLock) {};
    public:
        SpinLock() {};
        void lock() { while (flag_.test_and_set(std::memory_order_acquire)); }
        void unlock() { flag_.clear(std::memory_order_release); }
    private:
        std::atomic_flag flag_;
    };
    class OpenServerPool
    {
        SpinLock spinLock_;
        SpinLock spinSLock_;
        std::vector<OpenServer*> vectServer_;
        std::unordered_map<std::string, OpenServer*> mapNameServer_;
        std::unordered_map<std::string, OpenServer*(*)(const std::string&, const std::string&)> mapClassCreate_;
    public:
        virtual ~OpenServerPool();

        template <class T>
        bool registerServer(const std::string& className)
        {
            spinLock_.lock();
            auto iter = mapClassCreate_.find(className);
            if (iter != mapClassCreate_.end())
            {
                printf("OpenServerPool::registerServer [%s] is exist!\n", className.c_str());
                spinLock_.unlock();
                assert(false);
                return false;
            }
            mapClassCreate_[className] = &T::New;
            spinLock_.unlock();
            return true;
        }
        bool runAllServers();
        OpenServer* findServer(const std::string& serverName);
        void findServersByClassName(const std::string& className, std::vector<OpenServer*>& vectServer);
        OpenServer* startServer(const std::string& className, const std::string& serverName, const std::string& args);
    };
    static OpenServerPool OpenServerPool_;
protected:
    virtual bool init();
    virtual bool start();

    virtual void onInit() {}
    virtual void onStart() {}
    virtual void onStop() {}
    virtual void onMsgProto(OpenMsgProto& proto);
    virtual void onTimerProto(const OpenTimerProto& proto);
    virtual void onSocketProto(const OpenSocketProto& proto);
    OpenServer() : OpenThreadWorker("Unknow") {}

    bool startTime(int64_t deadline);
    bool startInterval(int64_t interval);
    //void log(const char* fmt, ...);

    std::string args_;
    std::string className_;
    std::unordered_map<std::string, int> mapServerNameToId_;
public:
    OpenServer(const std::string& name, const std::string& args);
    virtual ~OpenServer();
    inline const int pid()const { return pid_; }

    template <class T>
    bool sendMsgProto(int pid, std::shared_ptr<T>& protoMsg)
    {
        if (pid < 0)
        {
            assert(false);
            return false;
        }
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = protoMsg;
        if (!send(pid, proto))
        {
            printf("sendMsgProto faild. pid = %d\n", pid);
            return false;
        }
        return true;
    }

    template <class T>
    bool sendMsgProto(const std::string pname, std::shared_ptr<T>& protoMsg)
    {
        int pid = -1;
        auto iter = mapServerNameToId_.find(pname);
        if (iter == mapServerNameToId_.end())
        {
            OpenServer* server = FindServer(pname);
            if (server == 0)
            {
                assert(false);
                return false;
            }
            pid = server->pid();
            if (pid < 0)
            {
                assert(false);
                return false;
            }
            mapServerNameToId_[pname] = pid;
        }
        else
        {
            pid = iter->second;
        }
        if (pid < 0)
        {
            assert(false);
            return false;
        }
        auto proto = std::shared_ptr<OpenMsgProto>(new OpenMsgProto);
        proto->msg_ = protoMsg;
        if (!send(pid, proto))
        {
            printf("sendMsgProto2 faild. pid = %d\n", pid);
            return false;
        }
        return true;
    }

    template <class T>
    static bool RegisterServer(const std::string& className)
    {
        return OpenServerPool_.registerServer<T>(className);
    }

    static void RunServers();
    static OpenServer* FindServer(const std::string& serverName);
    static void GetServersByClassName(const std::string& className, std::vector<open::OpenServer*>& vectServer);
    static OpenServer* StartServer(const std::string& className, const std::string& serverName, const std::string& args);
    static void GetServers(const std::vector<std::string>& serverNames, std::vector<open::OpenServer*>& servers, int times = 10);
};


////////////OpenSocketServer//////////////////////
class OpenSocketServer : public OpenServer
{
protected:
    OpenSocketServer(const std::string& serverName, const std::string& args) :OpenServer(serverName, args){}

    virtual void onSocketData(const OpenSocketMsg& msg);
    virtual void onSocketClose(const OpenSocketMsg& msg);
    virtual void onSocketError(const OpenSocketMsg& msg);
    virtual void onSocketWarning(const OpenSocketMsg& msg);
    virtual void onSocketOpen(const OpenSocketMsg& msg);
    virtual void onSocketAccept(const OpenSocketMsg& msg);
    virtual void onSocketUdp(const OpenSocketMsg& msg);
    virtual void onSocketProto(const OpenSocketProto& proto);

};


////////////OpenTimerServer//////////////////////
class OpenTimerServer :public OpenServer
{
    int64_t timeInterval_;
    std::multimap<int64_t, int> mapTimerEvent_;
    OpenTimerServer(const std::string& name) : OpenServer(name, {}), timeInterval_(10) {}
protected:
    virtual void onStart();
private:
    void timeLoop();
    struct TimerPool
    {
        OpenTimerServer* timer_;
        TimerPool();
        ~TimerPool();
        void run();
        bool startTime(int pid, int64_t deadline);
    };
    static TimerPool TimerPool_;
public:
    virtual void onTimerProto(const OpenTimerProto& proto);
    static bool StartTime(int pid, int64_t deadline);
    static bool StartInterval(int pid, int64_t interval);
    static void Run() { TimerPool_.run(); }
};

////////////OpenApp//////////////////////
class OpenApp
{
    bool isRunning_;
    static void SocketFunc(const OpenSocketMsg* msg);
    static OpenApp OpenApp_;
public:
    OpenApp();
    virtual ~OpenApp();
    virtual void start();
    void wait();
    static inline OpenApp& Instance() { return OpenApp_; }
};

};

#endif  //HEADER_OPEN_SERVER_H

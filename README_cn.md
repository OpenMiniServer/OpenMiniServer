## OpenMiniServer
OpenMiniServer是一个超迷你、 超易用的C++高并发跨平台服务器框架。它基于Actor模型，提供了高性能的服务器功能，支持高并发和跨平台。

与其他C++服务器框架相比，OpenMiniServer更加轻量级，依赖更少的第三方库，对跨平台的支持也特别友好。

OpenMiniServer的目标是用尽可能少的C++代码实现高性能、高并发的服务器项目。它使用CMake构建系统实现跨平台支持，使同一份代码可以在不同的平台上开发和编译运行。可以在Windows环境下开发，无需要改动，就可以在Linux上编译出功能一样的程序。

为了开发OpenMiniServer开源项目，从零开发设计各基础库，如高并发socket库-OpenSocket、多线程库-OpenThread等。

OpenSocket是一个高性能的可复用IO库，是实现网络高并发的关键。OpenThread实现了Actor模式,大大简化了服务器业务逻辑的开发,可以轻松实现多核支持。


### 技术架构
1. 线程处理
OpenThread采用固定大小线程池实现高效线程管理。结合智能指针的线程安全特性，实现了OpenThread对象的无锁访问。

每个OpenThread对象在创建启动时，会创建一条线程并加入线程池中，以便统一管理该线程及其业务逻辑。
服务器计算业务根据CPU负载进行拆分，分发到多个OpenThread对象上，从而实现多核处理。

OpenThread通过条件锁实现线程间安全通信，构建Actor模型。多个OpenThread对象通过线程通信进行协作，处理复杂业务逻辑，实现简化开发工作，应对服务器高压处理业务需求。

OpenServer类是OpenMiniServer的核心类，它继承OpenThread，在OpenThread的基础上按照Actor模型进一步封装设计，提供更多便利的统一的接口。

2. 网络处理
OpenSocket是高性能socket库，提供高性能网络通信服务。请求socket服务时可以指定sessionID，OpenSocket返回网络消息就会携带此sessionID，根据sessionID可以把网络消息分发给申请者。

3. 线程与网络结合
每个OpenServer对象拥有唯一ID，把这个唯一ID当作sessionID处理，向OpenSocket请求socket服务，返回的网络消息会携带此ID，根据此ID可以找到OpenServer对象，把网络消息发给该OpenServer对象。

这就是OpenMiniServer框架的主要工作流，非常简单。


### 测试例子
OpenMiniServer设计的使用场景是大数据分析服务器，比如量化分析等。
在开始之前，先编译运行项目。

#### 1.编译和执行
请安装cmake工具，用cmake可以构建出VS或者XCode工程，就可以在vs或者xcode上编译运行。
源代码：https://github.com/OpenMiniServer/OpenMiniServer
```
#克隆项目
git clone https://github.com/OpenMiniServer/OpenMiniServer
cd ./OpenMiniServer
#创建build工程目录
mkdir build
cd build
cmake ..
#如果是windows系统，在该目录出现OpenMiniServer.sln，点击它就可以启动vs写代码调试
make
./OpenMiniServer
```
运行结果
```
OpenHttpServer Listen: 0.0.0.0:8080
start OpenServer
OpenHttpServer::accept:127.0.0.1:50285
OpenHttpAgent::Client::start[127.0.0.1:50285]
OpenComHttpAgent::Client::open[127.0.0.1:50285]
HTTP visit:127.0.0.1:50285 /api/stock?code=399001

csv content:
code,time,price
399001,2023-07-18,10.000000
399001,2023-07-19,20.000000
```

#### 2.项目文件
项目根目录文件很少，符合尽可能简单的设计目标。
```
demo             // 测试例子源代码
open             // OpenMiniServer的全部源代码
CMakeLists.txt   // CMake的主工程文件
```
如果OpenMiniServer需要使用https功能，就需要导入OpenSSL库，并加入编译宏定义USE_OPEN_SSL，即可。


#### 3.测试例子介绍
在demo文件夹下
```
msg        // 定义OpenServer通信消息
server     // 存放各种模块
util       // 通用源代码
app.h      // 唯一应用实例头文件 继承 open::OpenApp
app.cpp    // 唯一应用实例实现文件
```

demo/server有3个模块，centor、httpd和stock，它们最终继承OpenServer类，相当于有3种Actor，每种定制某种业务。
虽然它们的类名都一样，但可以靠namespace去区分，这样处理可以提高写代码效率。

1. stock模块主要负责下载股票数据，下载完就返回一条消息给请求者，它继承OpenHttpClient，拥有请求http功能；

2. centor模块主要负责控制，向stock模块发消息请求股票数据，然后收到股票数据后，把json格式的数据转成csv格式的数据；

3. httpd模块负责web服务，提供数据下载业务。
它有两种OpenServer，一种是httpa，继承OpenHttpAgent，可以处理接收和发送网络消息；另一种是httpd，继承OpenHttpServer，负责监听客户端连接，然后把连接发给httpa处理。

4. stock模块通过web请求向httpd模块下载股票数据。

5. 不同的业务，需要设计不同的OpenServer子类，把它们注册到OpenServerPool中，才能对它们进行启动，启动OpenServer的内部线程。
open::OpenApp负责这个事情，需要先对三种模块进行注册，每个模块绑定一个名字，然后用这个名字进行启动。

```C++
// 注册stock模块
open::OpenServer::RegisterServer<stock::Server>("stock");

// 启动两个stock模块： stock1和stock2。 相当于启动了两条线程，两条业务流水线。 当然，可以创建更多，实现多核处理同一业务。
open::OpenServer::StartServer("stock", "stock1", "");
open::OpenServer::StartServer("stock", "stock2", "");

```

##### 1.app源代码
App继承open::OpenApp，是一个单例类，主要负责注册OpenServer，然后对它们进行启动。
```C++
//导入centor、httpd和stock三个模块的头文件
#include "server/stock/server.h"
#include "server/httpd/httpa.h"
#include "server/httpd/httpd.h"
#include "server/centor/server.h"
//程序唯一应用实例
class App : public open::OpenApp
{
    static App TheApp_;
public:
    static inline App& Instance() { return TheApp_; }
    virtual void start()
    {
        OpenApp::start();
        //启动定时器模块
        open::OpenTimerServer::Run();

        //注册自定义模块
        //注册httpa::Server模块
        open::OpenServer::RegisterServer<httpa::Server>("httpa");
        open::OpenServer::RegisterServer<httpd::Server>("httpd");
        open::OpenServer::RegisterServer<stock::Server>("stock");
        open::OpenServer::RegisterServer<centor::Server>("centor");

        //启动4个httpa::Server对象，负责接收和发送客户端的网络消息
        open::OpenServer::StartServer("httpa", "httpa1", "");
        open::OpenServer::StartServer("httpa", "httpa2", "");
        open::OpenServer::StartServer("httpa", "httpa3", "");
        open::OpenServer::StartServer("httpa", "httpa4", "");

        //启动1个httpa::Server对象，负责监听客户端连接，并把连接发给httpa::Server对象处理
        open::OpenServer::StartServer("httpd", "httpd", "");

        //启动2个stock::Server对象，可以两条线程负债均衡处理同一业务，如果业务很大，CPU核数很多，可以多创建几个。
        open::OpenServer::StartServer("stock", "stock1", "");
        open::OpenServer::StartServer("stock", "stock2", "");

        //启动1个centor::Server对象
        open::OpenServer::StartServer("centor", "centor", "");

        //上述只是创建OpenServer，接下来启动它们，创建线程，处理各自的业务
        open::OpenServer::RunServers();
        printf("Start OpenMiniServer complete!\n");
    }
};
//应用实例对象
App App::TheApp_;
```

三种模块centor、httpd和stock，它们最终继承OpenServer类，而OpenServer类继承了OpenThread，也就是每个模块对象都有一条专属线程处理业务，无需考虑多线程问题。
每个OpenServer对象都是独立的，它们各自处理各自的事情，当需要协作时，只要互相发送消息即可。

接下来实现各个模块的源代码
##### 2.stock模块源代码

stock模块其实就是一个http请求客户端，OpenMiniServer提供了open::OpenHttpClient模块，可以简单实现http请求功能。
```C++
#include "open.h"
#include "msg/msg.h"
//用域名空间的名字来区分模块，模块名叫stock
namespace stock
{
// 继承open::OpenHttpClient，拥有请求http的能力，open::OpenHttpClient继承OpenServer
class Server : public open::OpenHttpClient
{
    typedef std::function<void(open::OpenHttpRequest&, open::OpenHttpResponse&)> HttpHandle;
public:
    Server(const std::string& name, const std::string& args)
        :open::OpenHttpClient(name, args)
    {
        sessionId_ = 0;
    }
    virtual ~Server() {}

    //每个模块都需要实现New函数，否则open::OpenServer::StartServer启动会失败
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new Server(serverName, args);
    }

    //它的父类OpenServer 启动以后，会启动它的线程，此线程启动成功，就会调用onStart方法
    virtual void onStart() {}

    //业务方法，请求股票数据，并通过回调函数返回结果
    bool reqStockData(const std::string& code, const HttpHandle& cb)
    {
        auto request = std::shared_ptr<open::OpenHttpRequest>(new open::OpenHttpRequest);
        request->method_ = "GET";
        request->url_ = "http://localhost:8080/api/stock?code=" + code;

        ++sessionId_;
        request->uid_ = sessionId_;
        mapHttpCalls_[sessionId_] = cb;
        sendHttp(request);
        return true;
    }

    //open::OpenHttpClient发送http请求，返回就调用此方法。通过sessionId到回到函数cb
    virtual void onHttp(open::OpenHttpRequest& req, open::OpenHttpResponse& rep)
    {
        int sessionId = req.uid_;
        auto iter = mapHttpCalls_.find(sessionId);
        if (iter != mapHttpCalls_.end())
        {
            iter->second(req, rep);
            mapHttpCalls_.erase(iter);
        }
    }

    //其他模块发送过来的消息。
    virtual void onMsgProto(open::OpenMsgProto& proto)
    {
        //接收处理centor模块的消息，请求股票数据
        if (StockRequestStockMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<StockRequestStockMsg> protoMsg = std::dynamic_pointer_cast<StockRequestStockMsg>(proto.msg_);
            if (!protoMsg) {
                assert(false); return;
            }
            //请求股票数据，http结果通过std::function返回
            reqStockData(protoMsg->code_, [=](open::OpenHttpRequest& req, open::OpenHttpResponse& rep) {
                auto sendProtoMsg = std::shared_ptr<StockResponseStockMsg>(new StockResponseStockMsg);
                sendProtoMsg->code_ = protoMsg->code_;
                rep.getBody(sendProtoMsg->stockData_);
                sendMsgProto<StockResponseStockMsg>(proto.srcName_, sendProtoMsg);
            });
        }
    }
protected:
    int sessionId_;
    std::unordered_map<int, HttpHandle> mapHttpCalls_;
};

};

```

##### 3.centor模块源代码
centor模块是一个controller角色。
```C++
#include "open.h"
#include "msg/msg.h"
//用域名空间的名字来区分模块，模块名叫centor
namespace centor
{
// 继承open::OpenServer
class Server : public open::OpenServer
{
public:
    Server(const std::string& name, const std::string& args)
        :open::OpenServer(name, args){}

    virtual ~Server() {}

    //每个模块都需要实现New函数，否则open::OpenServer::StartServer启动失败
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new Server(serverName, args);
    }

    //它的父类open::OpenServer启动以后，会启动它的线程，此线程启动成功，就会调用onStart方法
    void onStart()
    {
        //创建消息，请求指数399001的数据，在stock模块，有对它的处理
        auto protoMsg = std::shared_ptr<StockRequestStockMsg>(new StockRequestStockMsg);
        protoMsg->code_ = "399001";
        //把消息发给"stock1"绑定的对象。当然，也可以发给"stock2"，看谁比较空闲。
        sendMsgProto<StockRequestStockMsg>("stock1", protoMsg);
    }
    //接收stock模块返回的数据，
    virtual void onMsgProto(open::OpenMsgProto& proto)
    {
        if (StockResponseStockMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<StockResponseStockMsg> protoMsg = std::dynamic_pointer_cast<StockResponseStockMsg>(proto.msg_);
            if (!protoMsg)
            {
                assert(false); return;
            }
            //json parse
            open::OpenJson json;
            json.decode(protoMsg->stockData_);

            auto& nodeCode = json["code"];
            assert(nodeCode.isString());
            auto code = nodeCode.s();

            auto& nodeDatas = json["data"];
            assert(nodeDatas.size() == 2);

            //convert csv
            open::OpenCSV csv = { "code", "time", "price" };
            for (size_t i = 0; i < nodeDatas.size(); i++)
            {
                auto& nodeRow = nodeDatas[i];
                csv = { 
                    code,
                    nodeRow["time"].s(),
                    std::to_string(nodeRow["price"].d())
                };
            }
            std::string output;
            csv >> output;
            printf("\ncsv content:\n");
            printf("%s\n", output.data());
        }
    }
protected:
};
};

```

##### 4.httpd模块源代码
这个模块，有两种OpenServer：httpd和httpa，分别负责监听（listen）和处理业务（accept）。负责web下载服务。

httpd源代码
```C++
#include "open.h"

//用域名空间的名字来区分模块，模块名叫httpd，负责监听客户端连接
namespace httpd
{
// 继承open::OpenHttpServer，拥有监听网络端口的能力
class Server : public open::OpenHttpServer
{
public:
    Server::Server(const std::string& name, const std::string& args)
        :open::OpenHttpServer(name, args){}
    virtual ~Server() {}
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new Server(serverName, args);
    }
    virtual void onStart()
    {
        //创建消息，启动http监听：0.0.0.0:8080
        auto msg = std::shared_ptr<open::OpenHttpServerMsg>(new open::OpenHttpServerMsg);
        msg->ip_ = "0.0.0.0";
        //在CMakeLists.txt打开这个宏定义，可提供HTTPS服务，但编译的时候需要导入OpenSSL库
        #ifdef USE_OPEN_SSL
            msg->port_ = 443;
            msg->port1_ = 80;
            msg->isHttps_ = 1;
            msg->keyFile_ = "/xx/www.xx.com.key";
            msg->certFile_ = "/xx/www.xx.com.crt";
        #else
            msg->port_ = 8080;
            msg->port1_ = 0;
            msg->isHttps_ = 0;
        #endif
        msg->handle_ = 0;

        //获取所有属于模块”httpa“的open::OpenServer对象。注意，不能对它们进行delete操作
        std::vector<open::OpenServer*> servers;
        open::OpenServer::GetServersByClassName("httpa", servers);
        for (size_t i = 0; i < servers.size(); i++)
        {
            msg->vectAccepts_.push_back(servers[i]->pid());
        }
        open::OpenMsgProto proto;
        proto.msg_ = msg;
        onMsgProto(proto);
    }
protected:
};
};
```

httpa源代码
```C++
#include "open.h"
#include "util/http_util.h"

//用域名空间的名字来区分模块，模块名叫httpa，处理httpa发过来的客户端连接
namespace httpa
{

typedef open::OpenHttpRequest Req;
typedef open::OpenHttpResponse Rep;
typedef void(*HttpHandle)(Req* req, Rep* rep);

//处理客户端的http请求
class Handle
{
    //  /index.html
    void OnIndex(Req* req, Rep* rep)
    {
        auto html = Dom::DomCreate();
        auto& body = html->child("body");
        auto& h1 = body.create("h1");
        h1 = "Welcome OpenServer, Thanks.";
        std::string buffer;
        html->echo(buffer);
        rep->response(200, ".html", buffer);
    }

    //  /api/stock
    void OnApiStock(Req* req, Rep* rep)
    {
        //{
        //    "code": "xxxxx",
        //    "data" : [
        //       {"time": "2023-07-18", "price" : 10}
        //       {"time": "2023-07-19", "price": 20}
        //    ]
        //}
        auto& code = req->params_["code"];

        open::OpenJson json;
        json["code"] = code;
        auto& nodeData = json["data"];

        auto& row0 = nodeData[0];
        row0["time"] = "2023-07-18";
        row0["price"] = 10;

        auto& row1 = nodeData[1];
        row1["time"] = "2023-07-19";
        row1["price"] = 20;

        auto& buffer = json.encode();
        rep->response(200, ".json", buffer);
    }

    typedef void (Handle::* HttpCall)(Req* req, Rep* rep);
    std::unordered_map<std::string, HttpCall> mapRouteHandles;
public:
    Handle()
    {
        mapRouteHandles["/"] = (HttpCall)&Handle::OnIndex;
        mapRouteHandles["/index.html"] = (HttpCall)&Handle::OnIndex;
        mapRouteHandles["/api/stock"] = (HttpCall)&Handle::OnApiStock;
    }

    ~Handle() {}
    void onCallBack(Req* req, Rep* rep)
    {
        printf("HTTP visit:%s:%d %s \n", req->ip().data(), req->port_, req->url_.data());
        if (req->url_ == "robots.txt")
        {
            rep->body_ = "User-agent: *\nDisallow: / \n";
            rep->code_ = 200;
            rep->ctype_ = "text/plain;charset=utf-8";
            return;
        }
        HttpCall handle = 0;
        auto iter = mapRouteHandles.find(req->path_);
        if (mapRouteHandles.end() != iter)
        {
            handle = iter->second;
        }
        if (!handle)
        {
            handle = mapRouteHandles["/"];
        }
        (this->*handle)(req, rep);
    }
};

// 继承open::OpenHttpAgent，拥有处理客户端连接的能力
class Server : public open::OpenHttpAgent
{
public:
    Server(const std::string& name, const std::string& args)
        :open::OpenHttpAgent(name, args)
    {
    }
    virtual ~Server() {}
    static OpenServer* New(const std::string& serverName, const std::string& args)
    {
        return new Server(serverName, args);
    }

    virtual void onStart() {}

    //处理客户端的http请求
    virtual void onHttp(open::OpenHttpRequest& req, open::OpenHttpResponse& rep)
    {
        handle_.onCallBack(&req, &rep);
    }
protected:
    Handle handle_;
};

};
```

##### 4.OpenSocket和OpenThread的结合
在open::OpenApp::SocketFun方法中，处理OpenSocket的消息

```C++
//把OpenSocket的消费派发给绑定的OpenServer
void OpenApp::SocketFunc(const OpenSocketMsg* msg)
{
    if (!msg) return;
    if (msg->uid_ >= 0)
    {
        auto proto = std::shared_ptr<OpenSocketProto>(new OpenSocketProto);
        proto->srcPid_ = -1;
        proto->srcName_ = "OpenSocket";
        proto->data_ = std::shared_ptr<OpenSocketMsg>((OpenSocketMsg*)msg);
        //msg->uid_ 是请求者OpenServer的ID
        if (!OpenThread::Send((int)msg->uid_, proto))
            printf("SocketFunc dispatch faild pid = %d\n", (int)msg->uid_);
    }
    else delete msg;
}

//
void OpenApp::start()
{
    if (isRunning_) return;
    isRunning_ = true;
    //启动网络线程，并设置网络处理函数OpenApp::SocketFunc
    OpenSocket::Start(OpenApp::SocketFunc);
}

```


### 技术特点
1. OpenMiniServer极为轻巧简洁,通过自主设计的网络库和多线程库实现高性能服务器功能,代码量非常少却能发挥强大效果。

2. 采用CMake跨平台构建系统,实现写一次代码,随处编译运行的跨平台支持,不受限于特定系统环境。

3. 基于Actor模型设计,可以轻松实现高效的多核并行处理,配合Nginx负载均衡,可以便捷构建高可用的服务器集群。

4. 开发环境部署极为简单,第三方依赖库很少,一旦掌握Actor模型,使用OpenMiniServer构建服务器会变得非常容易。

总体来说, OpenMiniServer是一个迷你、轻巧、高效、跨平台的C++服务器框架，非常适合需要快速构建复杂服务器项目的开发者。它极简的代码风格和Actor模式设计可以提高开发效率，是值得推荐的高性能服务器解决方案。



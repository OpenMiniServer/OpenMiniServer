## OpenMiniServer
OpenMiniServer is an ultra-mini, ultra-easy-to-use C++ high concurrency cross-platform server framework. 

It is based on the Actor model and provides high-performance server capabilities with support for high concurrency and cross-platform functionality.

Compared to other C++ server frameworks, OpenMiniServer is more lightweight with fewer third-party library dependencies and has exceptionally friendly cross-platform support.

The goal of OpenMiniServer is to implement high-performance, high-concurrency server projects with as little C++ code as possible. 

It uses the CMake build system to achieve cross-platform support, allowing the same codebase to be developed and compiled on different platforms. 

One can develop on Windows and compile the same code on Linux without any changes.

To develop the OpenMiniServer, various foundational libraries have been built from scratch, such as the high-concurrency socket library OpenSocket, the multi-threading library OpenThread, etc.

OpenSocket is a high-performance reusable IO library that is key to enabling network high concurrency. OpenThread implements the Actor model, greatly simplifying server business logic development and easily enabling multi-core support. 


### Technical Principles
1. Thread Processing
OpenThread uses a fixed-size thread pool for efficient thread management. Combined with the thread-safety of smart pointer. it enables lock-free access to OpenThread objects.

When an OpenThread object is created, it starts a thread and adds it to the pool for unified management of the thread and its business logic.

Server computation tasks are split up based on CPU load and distributed to multiple OpenThread objects to achieve multi-core processing.

OpenThread uses condition variables to enable inter-thread safe communication and build the Actor model. Multiple OpenThread objects cooperate through thread messaging to handle complex business logic, simplifying development and handling high-load server scenarios.

The OpenServer class is the core class of OpenMiniServer. It inherits from OpenThread and further encapsulates it per the Actor model to provide more convenient unified interfaces.

2. Network Processing
OpenSocket is a high-performance socket library that provides high-performance network communication services. A sessionID can be specified when requesting socket services. OpenSocket will return network messages carrying this sessionID, allowing the messages to be dispatched to the requester based on the ID.

3. Combining Thread and Network
Each OpenServer object has a unique ID. This ID is used as the sessionID when requesting socket services from OpenSocket. Returned network messages carry this ID, allowing the OpenServer object to be located and the messages to be sent to it.

This is the main workflow of the OpenMiniServer framework, which is very simple.

### Test Example
The intended use case of OpenMiniServer is for big data analytics servers, such as quantitative analysis.
Before getting started, first compile and run the project.

#### 1.Compilation and execution
Please install the cmake tool. With cmake you can build a VS or XCode project and compile and run it on VS or XCode.
Source code：https://github.com/OpenMiniServer/OpenMiniServer
```
#Clone the project
git clone https://github.com/OpenMiniServer/OpenMiniServer
cd ./OpenMiniServer
#Create a build project directory
mkdir build
cd build
cmake ..
# If it's win32, OpenMiniServer.sln will appear in this directory. Click it to start VS for coding and debugging.
make
./OpenMiniServer
```

Running results
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

#### 2.Project Files
There are vary few files in the project root directory, aligning with the goal of keeping things as simple as possible.
```
demo             // Source code for test examples
open             // All source code for OpenMiniServer 
CMakeLists.txt   // Main CMake project file
```

If https functionality is needed for OpenMiniServer, the OpenSSL library can be imported and the compilation macro definition USE_OPEN_SSL can be added.  


#### 3.Test Example Introduction
Under the demo folder:
```
msg        // Defines communication messages for OpenServer
server     // Stores various modules
util       // Common source code
app.h      // Header file for the single app instance, inherits open::OpenApp
app.cpp    // Implementation file for the single app instance
```
There are 3 modules under demo/server folder - centor, httpd and stock. They ultimately inherit the OpenServer class, effectively acting as 3 different Actors, each customizing certain business logic.

Although their class names are the same, they can be distinguished by namespace, improving coding efficiency.
1. The stock module is mainly responsible for downloading stock data. After downloading, it returns a message to the requester. It inherits OpenHttpClient for http request capabilities.

2. The centor module is mainly for control. It sends messages to the stock module requesting stock data. After receiving the data, it converts the json formatted data to csv format.

3. The httpd module is responsible for web services, providing data download capabilities.
   It has two OpenServer types - httpa, which inherits OpenHttpAgent to handle receiving and sending network messages, and httpd, which inherits OpenHttpServer to listen for client connections and forward them to httpa for handling.

4. The stock module uses web requests to the httpd module to download stock data.

5. Different business logic requires designing different OpenServer subclasses and registering them in the OpenServerPool for startup and initializing their internal threads.

The open::OpenApp handles this by first registering the three modules, binding each to a name, then using the name to start them. 

```C++
// Register the stock module
open::OpenServer::RegisterServer<stock::Server>("stock");

// Start two stock modules: stock1 and stock2. This effectively starts two threads, two business pipelines. Of course, more can be created to achieve multi-core processing of the same business logic.
open::OpenServer::StartServer("stock", "stock1", "");
open::OpenServer::StartServer("stock", "stock2", "");

```

##### 1.App source code
App inherits open::OpenApp and is a singleton class, mainly responsible for registering OpenServers and then starting them.
```C++
// Import header files for the centor, httpd and stock modules
#include "server/stock/server.h"
#include "server/httpd/httpa.h"
#include "server/httpd/httpd.h"
#include "server/centor/server.h"

//The single application instance 
class App : public open::OpenApp
{
    static App TheApp_;
public:
    static inline App& Instance() { return TheApp_; }
    virtual void start()
    {
        OpenApp::start();
        //Start the timer module
        open::OpenTimerServer::Run();

        //Register custom modules
        //Register the httpa::Server module   
        open::OpenServer::RegisterServer<httpa::Server>("httpa");
        open::OpenServer::RegisterServer<httpd::Server>("httpd");
        open::OpenServer::RegisterServer<stock::Server>("stock");
        open::OpenServer::RegisterServer<centor::Server>("centor");

        //Start 4 httpa::Server objects to handle receiving and sending client network messages
        open::OpenServer::StartServer("httpa", "httpa1", "");
        open::OpenServer::StartServer("httpa", "httpa2", "");
        open::OpenServer::StartServer("httpa", "httpa3", "");
        open::OpenServer::StartServer("httpa", "httpa4", "");

        //Start 1 httpd::Server object to listen for client connections and forward to httpa::Server  
        open::OpenServer::StartServer("httpd", "httpd", "");

        //Start 2 stock::Server objects for load balancing the same business logic over two threads. More can be created to scale over multiple CPU cores.
        open::OpenServer::StartServer("stock", "stock1", "");
        open::OpenServer::StartServer("stock", "stock2", "");

        //Start 1 centor::Server object
        open::OpenServer::StartServer("centor", "centor", "");

        //The above just creates the OpenServers, next we start them to spin up threads and process their logic.
        open::OpenServer::RunServers();
        printf("Start OpenMiniServer complete!\n");
    }
};
//The application instance object
App App::TheApp_;
```

The three modules - centor, httpd and stock, ultimately inherit the OpenServer class, which in turn inherits OpenThread. 

This means each module object has its own dedicated thread to process business logic without having to worry about multi-threading issues.
Each OpenServer object is independent, handling its own logic. 

When they need to collaborate, they simply send messages to each other.
Next we will look at the source code implementation of each module. 

##### 2.Stock module source code 
The stock module is actually an HTTP request client. OpenMiniServer provides the open::OpenHttpClient module, which can simply implement HTTP request functionality.
```C++
#include "open.h"
#include "msg/msg.h"

// Use namespace to distinguish modules, this one is called stock 
namespace stock
{
// Inherit open::OpenHttpClient to gain HTTP request ability. open::OpenHttpClient inherits from OpenServer
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

	//Each module needs to implement New function, otherwise open::OpenServer::StartServer will fail
	static OpenServer* New(const std::string& serverName, const std::string& args)
	{
		return new Server(serverName, args);
	}

	//After its parent OpenServer starts, it will start its thread. Thread start success calls onStart method 
	virtual void onStart() {}

	//Business logic - Request stock data and return result via callback 
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

    //Called when open::OpenHttpClient sends a request and gets response. Map back to callback via sessionId
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

    //Messages from other modules 
    virtual void onMsgProto(open::OpenMsgProto& proto)
    {
    	//Handle message from centor module, stock data request   
        if (StockRequestStockMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<StockRequestStockMsg> protoMsg = std::dynamic_pointer_cast<StockRequestStockMsg>(proto.msg_);
            if (!protoMsg) {
                assert(false); return;
            }
            //Request stock data, return HTTP result via std::function
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

##### 3.Centor module source code 
The centor module plays the role of a controller. 
```C++
#include "open.h"
#include "msg/msg.h"
// Use namespace to distinguish modules, this one is called centor
namespace centor
{
// Inherit open::OpenServer
class Server : public open::OpenServer
{
public:
	Server(const std::string& name, const std::string& args)
		:open::OpenServer(name, args){}

	virtual ~Server() {}

	//Each module needs to implement New function, otherwise open::OpenServer::StartServer will fail
	static OpenServer* New(const std::string& serverName, const std::string& args)
	{
		return new Server(serverName, args);
	}

	// After its parent open::OpenServer starts, it will start its thread. Thread start success calls onStart method
    void onStart()
    {
    	//Create message to request index 399001 data from stock module  
        auto protoMsg = std::shared_ptr<StockRequestStockMsg>(new StockRequestStockMsg);
        protoMsg->code_ = "399001";
        //send message to "stock1" object. Can also send to "stock2" if less busy.
        sendMsgProto<StockRequestStockMsg>("stock1", protoMsg);
    }
    //Receive data returned from stock module
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

##### 4.Httpd module source code 
This module has two OpenServers: httpd and httpa, responsible for listening and handling business logic separately. It is responsible for web download services.
httpd listens for incoming requests.
httpa accepts and processes requests.

This separation of concerns provides better performance and scalability. httpd focuses on fast connection acceptance. httpa focuses on request processing. They can scale independently. 

httpd source code
```C++
#include "open.h"

// Use namespace to distinguish module, this one is called httpd, responsible for listening to client connections
namespace httpd
{
// Inherit open::OpenHttpServer, has ability to listen on network ports
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
    	// Create message to start http listening on 0.0.0.0:8080
        auto msg = std::shared_ptr<open::OpenHttpServerMsg>(new open::OpenHttpServerMsg);
        msg->ip_ = "0.0.0.0";
        //Enable HTTPS service in CMakeLists.txt by defining USE_OPEN_SSL, needs OpenSSL library when compiling
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

        //Get all open::OpenServer objects belonging to module "httpa". Do not delete them.
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

httpa source code
```C++
#include "open.h"
#include "util/http_util.h"

//Use namespace to distinguish modules, this module is called httpa, handles client connections sent from httpd.
namespace httpa
{

typedef open::OpenHttpRequest Req;
typedef open::OpenHttpResponse Rep;
typedef void(*HttpHandle)(Req* req, Rep* rep);

//Handles client http requests.
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

// Inherit open::OpenHttpAgent, has ability to handle client connections.
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

	//Handles client http requests.
	virtual void onHttp(open::OpenHttpRequest& req, open::OpenHttpResponse& rep)
	{
		handle_.onCallBack(&req, &rep);
	}
protected:
	Handle handle_;
};

};
```

##### 4.Combination of OpenSocket and OpenThread
Handle OpenSocket messages in open::OpenApp::SocketFun method

```C++
//Dispatch OpenSocket consumption to bound OpenServer
void OpenApp::SocketFunc(const OpenSocketMsg* msg)
{
    if (!msg) return;
    if (msg->uid_ >= 0)
    {
        auto proto = std::shared_ptr<OpenSocketProto>(new OpenSocketProto);
        proto->srcPid_ = -1;
        proto->srcName_ = "OpenSocket";
        proto->data_ = std::shared_ptr<OpenSocketMsg>((OpenSocketMsg*)msg);
        //msg->uid_ the ID of the requesting OpenServer
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
    //Start network thread, set network handler to OpenApp::SocketFunc
    OpenSocket::Start(OpenApp::SocketFunc);
}

```
- SocketFunc dispatches OpenSocket messages to the bound OpenServer based on uid_.
- Start() starts the network thread and sets SocketFunc as the callback.
- The network thread receives socket events and calls SocketFunc.
- SocketFunc looks up the requesting OpenServer by uid and dispatches the SocketMsg to it.
So OpenServers can receive socket events through this dispatch mechanism


### Technical features
1. OpenMiniServer is extremely compact and concise. It achieves high-performance server capabilities through its network and multi-threading libraries with very little code yet powerful effects.

2. It adopts the CMake cross-platform build system to achieve "write once, compile anywhere" cross-platform support, not limited to specific system environments.

3. Based on the Actor model design, it can easily achieve efficient multi-core parallel processing. Combined with Nginx load balancing, it can conveniently build highly available server clusters.

4. The development environment setup is extremely simple with very few third-party library dependencies. Once the Actor model is mastered, building servers with OpenMiniServer becomes very easy.

In summary, OpenMiniServer is a miniature, lightweight, efficient, cross-platform C++ server framework that is ideal for developers who need to quickly build complex server projects. Its minimalist code style and Actor model design can improve development efficiency. It is a highly recommendable high-performance server solution. 

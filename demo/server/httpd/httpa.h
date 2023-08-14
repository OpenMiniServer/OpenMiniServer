
#pragma once


#include "open.h"
#include "util/http_util.h"

namespace httpa
{

typedef open::OpenHttpRequest Req;
typedef open::OpenHttpResponse Rep;
typedef bool(*HttpHandle)(Req* req, Rep* rep);

//Handle
class Handle
{
    //  /index.html
    bool OnIndex(Req* req, Rep* rep)
    {
        auto html = Dom::DomCreate();
        auto& body = html->child("body");
        auto& h1 = body.create("h1");
        h1 = "Welcome OpenServer, Thanks.";

        std::string buffer;
        html->echo(buffer);
        rep->response(200, ".html", buffer);
        rep->send();
        // return false, need rep->send()
        return false;
    }

    //  /api/stock
    bool OnApiStock(Req* req, Rep* rep)
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
        // return true, don't need rep->send();
        return true;
    }

    typedef bool (Handle::* HttpCall)(Req* req, Rep* rep);
    std::unordered_map<std::string, HttpCall> mapRouteHandles;
public:
    Handle()
    {
        mapRouteHandles["/"] = (HttpCall)&Handle::OnIndex;
        mapRouteHandles["/index.html"] = (HttpCall)&Handle::OnIndex;
        mapRouteHandles["/api/stock"] = (HttpCall)&Handle::OnApiStock;
    }

    ~Handle() { }
    bool onCallBack(Req* req, Rep* rep)
    {
        printf("HTTP visit:%s:%d %s \n", req->ip().data(), req->port_, req->url_.data());
        if (req->url_ == "robots.txt")
        {
            rep->body_ = "User-agent: *\nDisallow: / \n";
            rep->code_ = 200;
            rep->ctype_ = "text/plain;charset=utf-8";
            return true;
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
        return (this->*handle)(req, rep);
    }
};

//Server
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
	virtual bool onHttp(open::OpenHttpRequest& req, open::OpenHttpResponse& rep)
	{
		return handle_.onCallBack(&req, &rep);
	}
protected:
	Handle handle_;
};

};




#pragma once

#include "open.h"
#include <functional>
#include <unordered_map>
#include "msg/msg.h"


namespace stock
{

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
	static OpenServer* New(const std::string& serverName, const std::string& args)
	{
		return new Server(serverName, args);
	}
	virtual void onStart() {}

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

    virtual void onMsgProto(open::OpenMsgProto& proto)
    {
        if (StockRequestStockMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<StockRequestStockMsg> protoMsg = std::dynamic_pointer_cast<StockRequestStockMsg>(proto.msg_);
            if (!protoMsg)
            {
                assert(false); return;
            }
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



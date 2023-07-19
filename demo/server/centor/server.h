
#pragma once

#include "open.h"
#include "msg/msg.h"


namespace centor
{

class Server : public open::OpenServer
{
public:
	Server(const std::string& name, const std::string& args)
		:open::OpenServer(name, args){}

	virtual ~Server() {}

	static OpenServer* New(const std::string& serverName, const std::string& args)
	{
		return new Server(serverName, args);
	}

    void onStart()
    {
        auto protoMsg = std::shared_ptr<StockRequestStockMsg>(new StockRequestStockMsg);
        protoMsg->code_ = "399001";
        //or sendMsgProto<StockRequestStockMsg>(ServerNameStock2, protoMsg);
        sendMsgProto<StockRequestStockMsg>(ServerNameStock1, protoMsg);
    }

    virtual void onMsgProto(open::OpenMsgProto& proto)
    {
        if (StockResponseStockMsg::MsgId() == proto.msg_->msgId())
        {
            std::shared_ptr<StockResponseStockMsg> protoMsg = std::dynamic_pointer_cast<StockResponseStockMsg>(proto.msg_);
            if (!protoMsg)
            {
                assert(false);
                return;
            }
            open::OpenJson json;
            json.decode(protoMsg->stockData_);

            auto& nodeCode = json["code"];
            assert(nodeCode.isString());
            auto code = nodeCode.s();

            auto& nodeDatas = json["data"];
            assert(nodeDatas.size() == 2);

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



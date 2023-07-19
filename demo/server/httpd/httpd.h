
#pragma once


#include "open.h"


namespace httpd
{

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
        auto msg = std::shared_ptr<open::OpenHttpServerMsg>(new open::OpenHttpServerMsg);
        msg->ip_ = "0.0.0.0";
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

        std::vector<open::OpenServer*> servers;
        open::OpenServer::GetServersByClassName(ServerClassHttpA, servers);
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



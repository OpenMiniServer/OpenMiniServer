
#pragma once

#include "open.h"

#include "server/stock/server.h"
#include "server/httpd/httpa.h"
#include "server/httpd/httpd.h"
#include "server/centor/server.h"

class App : public open::OpenApp
{

    static App TheApp_;
public:
    static inline App& Instance() { return TheApp_; }

    virtual void start()
    {
        OpenApp::start();

        open::OpenTimerServer::Run();

        //Register Server
        open::OpenServer::RegisterServer<httpa::Server>(ServerClassHttpA);
        open::OpenServer::RegisterServer<httpd::Server>(ServerClassHttpD);
        open::OpenServer::RegisterServer<stock::Server>(ServerClassStock);
        open::OpenServer::RegisterServer<centor::Server>(ServerClassCentor);

        //Start Server
        open::OpenServer::StartServer(ServerClassHttpA, "httpa1", "");
        open::OpenServer::StartServer(ServerClassHttpA, "httpa2", "");
        open::OpenServer::StartServer(ServerClassHttpA, "httpa3", "");
        open::OpenServer::StartServer(ServerClassHttpA, "httpa4", "");
        open::OpenServer::StartServer(ServerClassHttpD, "httpd", "");

        open::OpenServer::StartServer(ServerClassStock, ServerNameStock1, "");
        open::OpenServer::StartServer(ServerClassStock, ServerNameStock2, "");

        open::OpenServer::StartServer(ServerClassCentor, ServerNameCentor, "");

        open::OpenServer::RunServers();
        printf("start OpenServer\n");
    }
};

App App::TheApp_;

/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#ifndef HEADER_MSG_PROTO_H
#define HEADER_MSG_PROTO_H

#include "open.h"


static const std::string ServerClassHttpA = "httpa";
static const std::string ServerClassHttpD = "httpd";
static const std::string ServerClassStock = "stock";
static const std::string ServerClassCentor = "centor";

static const std::string ServerNameStock1 = "stock1";
static const std::string ServerNameStock2 = "stock2";
static const std::string ServerNameCentor = "centor";


////////////StockRequestStockMsg//////////////////////
struct StockRequestStockMsg : public open::OpenMsgProtoMsg
{
    std::string code_;

    StockRequestStockMsg() :OpenMsgProtoMsg() {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return StockRequestStockMsg::MsgId(); }
};


////////////StockResponseStockMsg//////////////////////
struct StockResponseStockMsg : public open::OpenMsgProtoMsg
{
    std::string code_;
    std::string stockData_;

    StockResponseStockMsg() :OpenMsgProtoMsg() {}
    static inline int MsgId() { return (int)(int64_t)(void*)&MsgId; }
    virtual inline int msgId() const { return StockResponseStockMsg::MsgId(); }
};



#endif   //HEADER_MSG_PROTO_H


#include "openhttpslib.h"
#include "../../openstring.h"
#include "../../opensocket.h"

namespace open
{

static std::map<int, std::string> httpStatusMsg;

static void init_http_status_msg(){
    httpStatusMsg[100] = "Continue";
    httpStatusMsg[101] = "Switching Protocols";
    httpStatusMsg[200] = "OK";
    httpStatusMsg[201] = "Created";
    httpStatusMsg[202] = "Accepted";
    httpStatusMsg[203] = "Non-Authoritative Information";
    httpStatusMsg[204] = "No Content";
    httpStatusMsg[205] = "Reset Content";
    httpStatusMsg[206] = "Partial Content";
    httpStatusMsg[300] = "Multiple Choices";
    httpStatusMsg[301] = "Moved Permanently";
    httpStatusMsg[302] = "Temporarily Moved";
    httpStatusMsg[303] = "See Other";
    httpStatusMsg[304] = "Not Modified";
    httpStatusMsg[305] = "Use Proxy";
    httpStatusMsg[307] = "Temporary Redirect";
    httpStatusMsg[400] = "Bad Request";
    httpStatusMsg[401] = "Unauthorized";
    httpStatusMsg[402] = "Payment Required";
    httpStatusMsg[403] = "Forbidden";
    httpStatusMsg[404] = "Not Found";
    httpStatusMsg[405] = "Method Not Allowed";
    httpStatusMsg[406] = "Not Acceptable";
    httpStatusMsg[407] = "Proxy Authentication Required";
    httpStatusMsg[408] = "Request Time-out";
    httpStatusMsg[409] = "Conflict";
    httpStatusMsg[410] = "Gone";
    httpStatusMsg[411] = "Length Required";
    httpStatusMsg[412] = "Precondition Failed";
    httpStatusMsg[413] = "Request Entity Too Large";
    httpStatusMsg[414] = "Request-URI Too Large";
    httpStatusMsg[415] = "Unsupported Media Type";
    httpStatusMsg[416] = "Requested range not satisfiable";
    httpStatusMsg[417] = "Expectation Failed";
    httpStatusMsg[500] = "Internal Server Error";
    httpStatusMsg[501] = "Not Implemented";
    httpStatusMsg[502] = "Bad Gateway";
    httpStatusMsg[503] = "Service Unavailable";
    httpStatusMsg[504] = "Gateway Time-out";
    httpStatusMsg[505] = "HTTP Version not supported";
}

static const std::string& GetHttpStatus(int code)
{
    if (httpStatusMsg.empty())
        init_http_status_msg();

    auto iter = httpStatusMsg.find(code);
    if (iter != httpStatusMsg.end())
        return iter->second;

    static std::string empty = "Unknown";
    return empty;
}


////////////OpenHttp//////////////////////
OpenHttp::OpenHttp() 
    :code_(-1), 
    clen_(0), 
    isReq_(false), 
    port_(0), 
    isHttps_(false), 
    isChunked_(false), 
    isRemote_(false),
    isClient_(false),
    isFinish_(false)
{
}
std::string& OpenHttp::header(const std::string& key)
{
    std::string lowerKey;
    for (size_t i = 0; i < key.size(); ++i) lowerKey.push_back(key[i]);
    return headers_[lowerKey];
}
bool OpenHttp::hasHeader(const std::string& key)
{
    std::string lowerKey;
    for (size_t i = 0; i < key.size(); ++i) lowerKey.push_back(key[i]);
    return headers_.find(lowerKey) != headers_.end();
}
void OpenHttp::removeHeader(const std::string& key)
{
    std::string lowerKey;
    for (size_t i = 0; i < key.size(); ++i) lowerKey.push_back(key[i]);
    headers_.erase(lowerKey);
}
void OpenHttp::parseUrl()
{
    const std::string url = url_;
    if (url.empty()) return;
    int len = (int)url.length();
    char* ptr = (char*)url.c_str();
    if (len >= 8)
    {
        static const std::string strHttp = "http://";
        static const std::string strHttps = "https://";
        if (memcmp(ptr, strHttp.data(), strHttp.size()) == 0)
        {
            isHttps_ = false;
            port_ = 80;
            ptr += strHttp.size();
        }
        else if (memcmp(ptr, strHttps.data(), strHttps.size()) == 0)
        {
            isHttps_ = true;
            port_ = 443;
            ptr += strHttps.size();
        }
        else
        {
            assert(false);
        }
    }
    else
    {
        port_ = 80;
    }
    const char* tmp = strstr(ptr, "/");
    path_.clear();
    if (tmp != 0)
    {
        path_.append(tmp);
        host_.clear();
        host_.append(ptr, tmp - ptr);
    }
    else
    {
        host_ = ptr;
    }
    if (headers_["host"].empty())
    {
        headers_["host"] = host_;
    }

    domain_.clear();
    ptr = (char*)host_.c_str();
    tmp = strstr(ptr, ":");
    if (tmp != 0)
    {
        domain_.append(ptr, tmp - ptr);
        tmp += 1;
        port_ = atoi(tmp);
    }
    else
    {
        domain_ = ptr;
    }
}

const std::string& OpenHttp::lookIp()
{
    ip_ = OpenSocket::DomainNameToIp(domain_);
    return ip_;
}

void OpenHttp::splitPaths()
{
    OpenString::Split(path_, "/", paths_);
    auto& paths = paths_;
    while (!paths.empty() && paths[0].empty()) paths.erase(paths.begin());
    while (!paths.empty() && paths.back().empty()) paths.pop_back();
}

void OpenHttp::encodeReqHeader()
{
    if (path_.empty()) path_ = "/";
    if (method_.empty()) method_ = "GET";
    head_.clear();

    std::string strPath = path_;
    if (!params_.empty())
    {
        if (method_ != "POST")
        {
            const char* ptr = strstr(strPath.data(), "?");
            if (!ptr) strPath.push_back('?');
            auto iter = params_.begin();
            for (; iter != params_.end(); iter++)
            {
                if (strPath.back() == '?')
                    strPath.append(iter->first + "=" + iter->second);
                else
                    strPath.append("&" + iter->first + "=" + iter->second);
            }
        }
        else
        {
            body_.clear();
            auto iter = params_.begin();
            for (; iter != params_.end(); iter++)
            {
                if (body_.size() == 0)
                    body_.pushBack(iter->first + "=" + iter->second);
                else
                    body_.pushBack("&" + iter->first + "=" + iter->second);
            }
        }
    }

    head_.pushBack(method_ + " " + strPath + " HTTP/1.1\r\n");
    auto iter = headers_.begin();
    for (; iter != headers_.end(); iter++)
    {
        head_.pushBack(iter->first + ": " + iter->second + "\r\n");
    }
    if (body_.size() > 0)
    {
        head_.pushBack("content-length:" + std::to_string(body_.size()) + "\r\n");
    }
    head_.pushBack("\r\n");
}

void OpenHttp::encodeRespHeader()
{
    head_.clear();
    head_.pushBack("HTTP/1.1 " + std::to_string(code_) + " " + GetHttpStatus(code_) + "\r\n");
    if (!headers_.empty())
    {
        auto iter = headers_.begin();
        for (; iter != headers_.end(); iter++)
            head_.pushBack(iter->first + ": " + iter->second + "\r\n");
    }
    auto iter = headers_.find("content-type");
    if (iter == headers_.end())
    {
        if (!ctype_.empty())
        {
            head_.pushBack("content-type: " + ctype_ + "\r\n");
        }
    }
    if (body_.size() > 0)
        head_.pushBack("content-length: " + std::to_string(body_.size()) + "\r\n");

    head_.pushBack("\r\n");
}

void OpenHttp::decodeReqHeader()
{
    if (!headers_.empty() || head_.size() < 12) return;
    const char* head = (const char*)head_.data();
    const char* ptr = strstr(head, "\r\n");
    if (!ptr) return;

    std::string line;
    line.append(head, ptr - head);

    url_.clear();
    method_.clear();
    int state = 0;
    for (size_t k = 0; k < line.size(); ++k)
    {
        if (state == 0)
        {
            if (line[k] != ' ')
            {
                method_.push_back(line[k]);
                continue;
            }
            state = 1;
            while (k < line.size() && line[k] == ' ') ++k;
            if (line[k] != ' ') --k;
        }
        else
        {
            if (line[k] != ' ')
            {
                url_.push_back(line[k]);
                continue;
            }
            break;
        }
    }

    {
        const char* tmp = strstr(url_.data(), "?");
        if (tmp)
        {
            path_.append(url_.data(), tmp - url_.data());
            //?x
            std::string param = tmp + 1;
            std::vector<std::string> vectItems;
            OpenString::Split(param, "&", vectItems);
            std::string key;
            std::string value;
            for (size_t i = 0; i < vectItems.size(); i++)
            {
                auto& item = vectItems[i];
                tmp = strstr(item.c_str(), "=");
                if (!tmp) continue;
                key.clear();
                key.append(item.c_str(), tmp - item.c_str());
                while (!key.empty() && key[0] == ' ') key.erase(key.begin());
                while (!key.empty() && key.back() == ' ') key.pop_back();
                if (key.empty()) continue;
                for (size_t x = 0; x < key.size(); x++)
                    key[x] = std::tolower(key[x]);

                value = tmp + 1;
                while (!value.empty() && value[0] == ' ') value.erase(value.begin());
                while (!value.empty() && value.back() == ' ') value.pop_back();
                params_[key] = value;
            }
        }
        else
        {
            path_ = url_;
            params_.clear();
        }
    }


    code_ = 200;

    line.clear();
    int k = -1;
    int j = -1;
    std::string key;
    std::string value;
    headers_.clear();
    for (size_t i = ptr - head + 2; i < head_.size() - 1; i++)
    {
        if (head[i] == '\r' && head[i + 1] == '\n')
        {
            if (j > 0)
            {
                k = 0;
                while (k < line.size() && line[k] == ' ') ++k;
                while (k >= 0 && line.back() == ' ') line.pop_back();
                value = line.data() + j + 1;
                while (j >= 0 && line[j] == ' ') j--;
                key.clear();
                key.append(line.data(), j);
                while (!key.empty() && key[0] == ' ') key.erase(key.begin());
                while (!key.empty() && key.back() == ' ') key.pop_back();
                if (key.empty()) continue;
                for (size_t x = 0; x < key.size(); x++)
                    key[x] = std::tolower(key[x]);

                while (!value.empty() && value[0] == ' ') value.erase(value.begin());
                while (!value.empty() && value.back() == ' ') value.pop_back();
                headers_[key] = value;
            }
            ++i;
            j = -1;
            line.clear();
            continue;
        }
        line.push_back(head[i]);
        if (j < 0 && line.back() == ':')
        {
            j = (int)line.size() - 1;
        }
    }
    clen_ = -1;
    auto iter = headers_.find("content-length");
    if (iter != headers_.end())
    {
        auto& strLen = iter->second;
        if (!strLen.empty())
        {
            clen_ = std::atoi(strLen.c_str());
        }
    }
}

void OpenHttp::decodeRespHeader()
{
    if (!headers_.empty() || head_.size() < 12) return;
    const char* head = (const char*)head_.data();
    const char* ptr = strstr(head, "\r\n");
    if (!ptr) return;

    code_ = 0;
    clen_ = 0;
    std::string line;
    line.append(head, ptr - head);
    for (size_t i = 0; i < line.size(); i++)
    {
        if (line[i] == ' ')
        {
            while (i < line.size() && line[i] == ' ') ++i;
            code_ = std::atoi(line.data() + i);
            break;
        }
    }
    if (code_ <= 0) return;
    line.clear();
    int k = -1;
    int j = -1;
    std::string key;
    std::string value;
    headers_.clear();
    for (size_t i = ptr - head + 2; i < head_.size() - 1; i++)
    {
        if (head[i] == '\r' && head[i + 1] == '\n')
        {
            if (j > 0)
            {
                k = 0;
                while (k < line.size() && line[k] == ' ') ++k;
                while (k >= 0 && line.back() == ' ') line.pop_back();
                value = line.data() + j + 1;
                while (j >= 0 && line[j] == ' ') j--;
                key.clear();
                key.append(line.data(), j);
                for (size_t x = 0; x < key.size(); x++)
                    key[x] = std::tolower(key[x]);


                headers_[key] = value;
            }
            ++i;
            j = -1;
            line.clear();
            continue;
        }
        line.push_back(head[i]);
        if (j < 0 && line.back() == ':')
        {
            j = (int)line.size() - 1;
        }
    }
    clen_ = -1;
    auto iter = headers_.find("content-length");
    if (iter != headers_.end())
    {
        auto& strLen = iter->second;
        if (!strLen.empty())
        {
            clen_ = std::atoi(strLen.c_str());
        }
    }
}

bool OpenHttp::requestData(const char* data, size_t size)
{
    assert(isReq_);
    if (code_ == -1)
    {
        head_.pushBack(data, size);
        unsigned char* tmp = head_.data();
        size_t i = 0;
        for (; i < head_.size() - 3; i++)
        {
            if (tmp[i] == '\r' && tmp[i + 1] == '\n' && tmp[i + 2] == '\r' && tmp[i + 3] == '\n')
                break;
        }
        if (i >= 8192)
        {
            return true;
        }
        if (i >= head_.size() - 3) return false;
        i += 4;
        body_.clear();
        buffer_.clear();
        int64_t leftNum = head_.size() - i;
        if (leftNum > 0)
        {
            buffer_.pushBack(0, leftNum);
            head_.popBack(buffer_.data(), buffer_.size());
        }
        code_ = 0;
        decodeReqHeader();

        auto iter = headers_.find("transfer-encoding");
        isChunked_ = false;
        if (iter != headers_.end())
        {
            isChunked_ = iter->second.find("chunked") != std::string::npos;
        }
        if (isChunked_)
            clen_ = -1;
        else if (buffer_.size() > 0)
        {
            body_.pushBack(buffer_.data(), buffer_.size());
            buffer_.clear();
        }
    }
    else
    {
        if (isChunked_)
            buffer_.pushBack(data, size);
        else
            body_.pushBack(data, size);
    }
    if (isChunked_)
    {
        while (true)
        {
            if (clen_ == -1)
            {
                //0xXXX\r\nxxxx\r\n
                const char* tmp = (const char*)buffer_.data();
                int64_t i = 0;
                int64_t size = buffer_.size();
                for (; i < size - 2; i++)
                {
                    if (tmp[i] == '\r' && tmp[i + 1] == '\n')
                        break;
                }
                // no finish
                if (i >= size - 1)
                {
                    //too length
                    if (i > 64) return true;
                    return false;
                }
                clen_ = strtol(tmp, 0, 16);
                // pop len data
                buffer_.popFront(0, i + 2);
                //finish
                if (clen_ == 0)
                {
                    tmp = (const char*)buffer_.data();
                    if (tmp[0] == '\r' && tmp[1] == '\n')
                    {
                        buffer_.popFront(0, 2);
                        assert(buffer_.size() == 0);
                    }
                    else
                    {
                        assert(false);
                    }
                    buffer_.clear();
                    return true;
                }
            }
            // data + \r\n. enough data.
            else
            {
                if (clen_ + 2 <= (int64_t)buffer_.size())
                {
                    body_.pushBack(buffer_.data(), clen_);
                    buffer_.popFront(0, clen_);
                    const char* tmp = (const char*)buffer_.data();
                    if (tmp[0] == '\r' && tmp[1] == '\n')
                    {
                        buffer_.popFront(0, 2);
                    }
                    else
                    {
                        assert(false);
                        return false;
                    }
                    clen_ = -1;
                }
                ////no enough data
                else
                {
                    return false;
                }
            }
        }
        return false;
    }

    //body_
    unsigned char* tmp = body_.data();
    int len = (int)body_.size();
    if (clen_ > 0)
    {
        if (clen_ <= len)
        {
            if (tmp[len - 2] == '\r' && tmp[len - 1] == '\n')
            {
                clen_ = len;
            }
            return true;
        }
    }
    else
    {
        if (!isChunked_)
        {
            if (clen_ <= 0)
            {
                body_.clear();
                return true;
            }
        }
        if (len > 2)
        {
            if (tmp[len - 2] == '\r' && tmp[len - 1] == '\n')
            {
                clen_ = len;
                return true;
            }
        }
    }
    return false;
}

bool OpenHttp::responseData(const char* data, size_t size)
{
    //cacheFile_
    assert(!isReq_);
    if (code_ == -1)
    {
        head_.pushBack(data, size);
        unsigned char* tmp = head_.data();
        size_t i = 0;
        for (; i < head_.size() - 3; i++)
        {
            if (tmp[i] == '\r' && tmp[i + 1] == '\n' && tmp[i + 2] == '\r' && tmp[i + 3] == '\n')
                break;
        }
        if (i >= 8192)
        {
            return true;
        }
        //printf("OpenHttp::responseData[[receive header]]\n");
        if (i >= head_.size() - 3) return false;
        i += 4;
        body_.clear();
        buffer_.clear();
        int64_t leftNum = head_.size() - i;
        if (leftNum > 0)
        {
            buffer_.pushBack(0, leftNum);
            head_.popBack(buffer_.data(), buffer_.size());
        }
        code_ = 0;
        decodeRespHeader();

        auto& chunked = header("transfer-encoding");
        isChunked_ = chunked.find("chunked") != std::string::npos;
        if (isChunked_)
        {
            //printf("OpenHttp::responseData[[chunked]]\n");
            clen_ = -1;
        }
        else if (buffer_.size() > 0)
        {
            //printf("OpenHttp::responseData[[common]] len = %ld\n", clen_);
            body_.pushBack(buffer_.data(), buffer_.size());
            buffer_.clear();
        }
    }
    else
    {
        if (isChunked_)
            buffer_.pushBack(data, size);
        else
            body_.pushBack(data, size);
    }
    if (isChunked_)
    {
        while (true)
        {
            if (clen_ == -1)
            {
                //0xXXX\r\nxxxx\r\n
                const char* tmp = (const char*)buffer_.data();
                int64_t i = 0;
                int64_t size = buffer_.size();
                for (; i < size - 2; i++)
                {
                    if (tmp[i] == '\r' && tmp[i + 1] == '\n')
                        break;
                }
                // no finish
                if (i >= size - 1)
                {
                    //too length
                    if (i > 64) return true;
                    return false;
                }
                clen_ = strtol(tmp, 0, 16);
                // pop len data
                buffer_.popFront(0, i + 2);
                //finish
                if (clen_ == 0)
                {
                    tmp = (const char*)buffer_.data();
                    if (tmp[0] == '\r' && tmp[1] == '\n')
                    {
                        buffer_.popFront(0, 2);
                        assert(buffer_.size() == 0);
                    }
                    else
                    {
                        //assert(false);
                    }
                    buffer_.clear();
                    return true;
                }
            }
            // data + \r\n. enough data.
            else
            {
                if (clen_ + 2 <= (int64_t)buffer_.size())
                {
                    body_.pushBack(buffer_.data(), clen_);
                    buffer_.popFront(0, clen_);
                    const char* tmp = (const char*)buffer_.data();
                    if (tmp[0] == '\r' && tmp[1] == '\n')
                    {
                        buffer_.popFront(0, 2);
                    }
                    else
                    {
                        //assert(false);
                        return false;
                    }
                    clen_ = -1;
                }
                ////no enough data
                else
                {
                    return false;
                }
            }
        }
        return false;
    }

    //body_
    unsigned char* tmp = body_.data();
    size_t len = body_.size();
    //printf("OpenHttp::responseData[[body_]] clen_ = %ld, body len = %d\n", clen_, len);
    if (clen_ > 0)
    {
        if (clen_ <= (int64_t)len)
        {
            if (tmp[len - 2] == '\r' && tmp[len - 1] == '\n')
            {
                clen_ = len;
            }
            return true;
        }
    }
    else 
    {
        if (len > 2)
        {
            if (tmp[len - 2] == '\r' && tmp[len - 1] == '\n')
            {
                clen_ = len;
                return true;
            }
        }
    }
    return false;
}


////////////OpenHttpResponse//////////////////////
void OpenHttpResponse::decodeReqHeader()
{
    OpenHttp::decodeReqHeader();
}

////////////OpenHttpRequest//////////////////////

const std::map<std::string, std::string> OpenHttpResponse::ContentTypes_ = {
    {".html", "text/html;charset=utf-8"},
    {".css", "text/css;charset=utf-8"},
    {".plain", "text/plain;charset=utf-8"},
    {".xml", "text/xml;charset=utf-8"},
    {".json", "application/json;charset=utf-8"},
    {".js", "application/x-javascript;charset=utf-8"},
    {".log", "text/plain;charset=utf-8"},
    {".download", "application/octet-stream;charset=utf-8"},

    {".gif", "image/gif"},
    {".jpg", "image/jpg"},
    {".png", "image/png"},
    {".jpeg", "image/jpeg"},
    {".webp", "image/webp"},

    {".tif", "image/tiff"},
    {".001", "application/x-001"},
    {".301", "application/x-301"},
    {".323", "text/h323"},
    {".906", "application/x-906"},
    {".907", "drawing/907"},
    {".a11", "application/x-a11"},
    {".acp", "audio/x-mei-aac"},
    {".ai", "application/postscript"},
    {".aif", "audio/aiff"},
    {".aifc", "audio/aiff"},
    {".aiff", "audio/aiff"},
    {".anv", "application/x-anv"},
    {".asa", "text/asa"},
    {".asf", "video/x-ms-asf"},
    {".asp", "text/asp"},
    {".asx", "video/x-ms-asf"},
    {".au", "audio/basic"},
    {".avi", "video/avi"},
    {".biz", "text/xml;charset=utf-8"},
    {".bmp", "application/x-bmp"},
    {".bot", "application/x-bot"},
    {".c4t", "application/x-c4t"},
    {".c90", "application/x-c90"},
    {".cal", "application/x-cals"},
    {".cdf", "application/x-netcdf"},
    {".cdr", "application/x-cdr"},
    {".cel", "application/x-cel"},
    {".cer", "application/x-x509-ca-cert"},
    {".cg4", "application/x-g4"},
    {".cgm", "application/x-cgm"},
    {".cit", "application/x-cit"},
    {".class", "java/*"},
    {".cml", "text/xml;charset=utf-8"},
    {".cmp", "application/x-cmp"},
    {".cmx", "application/x-cmx"},
    {".cot", "application/x-cot"},
    {".crl", "application/pkix-crl"},
    {".crt", "application/x-x509-ca-cert"},
    {".csi", "application/x-csi"},
    {".cut", "application/x-cut"},
    {".dbf", "application/x-dbf"},
    {".dbm", "application/x-dbm"},
    {".dbx", "application/x-dbx"},
    {".dcd", "text/xml;charset=utf-8"},
    {".dcx", "application/x-dcx"},
    {".der", "application/x-x509-ca-cert"},
    {".dgn", "application/x-dgn"},
    {".dib", "application/x-dib"},
    {".dll", "application/x-msdownload"},
    {".doc", "application/msword"},
    {".dot", "application/msword"},
    {".drw", "application/x-drw"},
    {".dtd", "text/xml;charset=utf-8"},
    {".dwf", "application/x-dwf"},
    {".dwg", "application/x-dwg"},
    {".dxb", "application/x-dxb"},
    {".dxf", "application/x-dxf"},
    {".emf", "application/x-emf"},
    {".eml", "message/rfc822"},
    {".ent", "text/xml;charset=utf-8"},
    {".epi", "application/x-epi"},
    {".eps", "application/x-ps"},
    {".eps", "application/postscript"},
    {".etd", "application/x-ebx"},
    {".exe", "application/x-msdownload"},
    {".fax", "image/fax"},
    {".fif", "application/fractals"},
    {".fo", "text/xml;charset=utf-8"},
    {".frm", "application/x-frm"},
    {".g4", "application/x-g4"},
    {".gbr", "application/x-gbr"},
    {".gl2", "application/x-gl2"},
    {".gp4", "application/x-gp4"},
    {".hgl", "application/x-hgl"},
    {".hmr", "application/x-hmr"},
    {".hpg", "application/x-hpgl"},
    {".hpl", "application/x-hpl"},
    {".hqx", "application/mac-binhex40"},
    {".hrf", "application/x-hrf"},
    {".hta", "application/hta"},
    {".htc", "text/x-component"},
    {".htm", "text/html;charset=utf-8"},
    {".htt", "text/webviewhtml"},
    {".htx", "text/html;charset=utf-8"},
    {".icb", "application/x-icb"},
    {".ico", "image/x-icon"},
    {".ico", "application/x-ico"},
    {".iff", "application/x-iff"},
    {".ig4", "application/x-g4"},
    {".igs", "application/x-igs"},
    {".iii", "application/x-iphone"},
    {".img", "application/x-img"},
    {".ins", "application/x-internet-signup"},
    {".isp", "application/x-internet-signup"},
    {".java", "java/*;charset=utf-8"},
    {".jfif", "image/jpeg"},
    {".jpe", "image/jpeg"},
    {".jpe", "application/x-jpe"},
    {".jsp", "text/html;charset=utf-8"},
    {".la1", "audio/x-liquid-file"},
    {".lar", "application/x-laplayer-reg"},
    {".latex", "application/x-latex"},
    {".lavs", "audio/x-liquid-secure"},
    {".lbm", "application/x-lbm"},
    {".lmsff", "audio/x-la-lms"},
    {".ls", "application/x-javascript;charset=utf-8"},
    {".ltr", "application/x-ltr"},
    {".m1v", "video/x-mpeg"},
    {".m2v", "video/x-mpeg"},
    {".m3u", "audio/mpegurl"},
    {".m4e", "video/mpeg4"},
    {".mac", "application/x-mac"},
    {".man", "application/x-troff-man"},
    {".math", "text/xml;charset=utf-8"},
    {".mdb", "application/msaccess"},
    {".mdb", "application/x-mdb"},
    {".mfp", "application/x-shockwave-flash"},
    {".mht", "message/rfc822"},
    {".mhtml", "message/rfc822"},
    {".mi", "application/x-mi"},
    {".mid", "audio/mid"},
    {".midi", "audio/mid"},
    {".mil", "application/x-mil"},
    {".mml", "text/xml;charset=utf-8"},
    {".mnd", "audio/x-musicnet-download"},
    {".mns", "audio/x-musicnet-stream"},
    {".mocha", "application/x-javascript;charset=utf-8"},
    {".movie", "video/x-sgi-movie"},
    {".mp1", "audio/mp1"},
    {".mp2", "audio/mp2"},
    {".mp2v", "video/mpeg"},
    {".mp3", "audio/mp3"},
    {".mp4", "video/mpeg4"},
    {".mpa", "video/x-mpg"},
    {".mpe", "video/x-mpeg"},
    {".mpeg", "video/mpg"},
    {".mpg", "video/mpg"},
    {".mpga", "audio/rn-mpeg"},
    {".mps", "video/x-mpeg"},
    {".mpv", "video/mpg"},
    {".mpv2", "video/mpeg"},
    {".mtx", "text/xml;charset=utf-8"},
    {".mxp", "application/x-mmxp"},
    {".net", "image/pnetvue"},
    {".nrf", "application/x-nrf"},
    {".nws", "message/rfc822"},
    {".odc", "text/x-ms-odc;charset=utf-8"},
    {".out", "application/x-out"},
    {".p10", "application/pkcs10"},
    {".p12", "application/x-pkcs12"},
    {".p7b", "application/x-pkcs7-certificates"},
    {".p7c", "application/pkcs7-mime"},
    {".p7m", "application/pkcs7-mime"},
    {".p7r", "application/x-pkcs7-certreqresp"},
    {".p7s", "application/pkcs7-signature"},
    {".pc5", "application/x-pc5"},
    {".pci", "application/x-pci"},
    {".pcl", "application/x-pcl"},
    {".pcx", "application/x-pcx"},
    {".pdf", "application/pdf"},
    {".pfx", "application/x-pkcs12"},
    {".pgl", "application/x-pgl"},
    {".pic", "application/x-pic"},
    {".pko", "application/vnd.pko"},
    {".pl", "application/x-perl"},
    {".plg", "text/html;charset=utf-8"},
    {".pls", "audio/scpls"},
    {".plt", "application/x-plt"},
    {".pot", "application/vnd.ms-powerpoint"},
    {".ppa", "application/vnd.ms-powerpoint"},
    {".ppm", "application/x-ppm"},
    {".pps", "application/vnd.ms-powerpoint"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".ppt", "application/x-ppt"},
    {".pr", "application/x-pr"},
    {".prf", "application/pics-rules"},
    {".prn", "application/x-prn"},
    {".prt", "application/x-prt"},
    {".ps", "application/x-ps"},
    {".ps", "application/postscript"},
    {".ptn", "application/x-ptn"},
    {".pwz", "application/vnd.ms-powerpoint"},
    {".r3t", "text/vnd.rn-realtext3d"},
    {".ra", "audio/vnd.rn-realaudio"},
    {".ram", "audio/x-pn-realaudio"},
    {".ras", "application/x-ras"},
    {".rat", "application/rat-file"},
    {".rdf", "text/xml"},
    {".red", "application/x-red"},
    {".rgb", "application/x-rgb"},
    {".rjs", "application/vnd.rn-realsystem-rjs"},
    {".rjt", "application/vnd.rn-realsystem-rjt"},
    {".rlc", "application/x-rlc"},
    {".rle", "application/x-rle"},
    {".rm", "application/vnd.rn-realmedia"},
    {".rmf", "application/vnd.adobe.rmf"},
    {".rmi", "audio/mid"},
    {".rmj", "application/vnd.rn-realsystem-rmj"},
    {".rmm", "audio/x-pn-realaudio"},
    {".rmp", "application/vnd.rn-rn_music_package"},
    {".rms", "application/vnd.rn-realmedia-secure"},
    {".rmvb", "application/vnd.rn-realmedia-vbr"},
    {".rmx", "application/vnd.rn-realsystem-rmx"},
    {".rnx", "application/vnd.rn-realplayer"},
    {".rp", "image/vnd.rn-realpix"},
    {".rpm", "audio/x-pn-realaudio-plugin"},
    {".rsml", "application/vnd.rn-rsml"},
    {".rt", "text/vnd.rn-realtext;charset=utf-8"},
    {".rtf", "application/msword"},
    {".rtf", "application/x-rtf"},
    {".rv", "video/vnd.rn-realvideo"},
    {".sam", "application/x-sam"},
    {".sat", "application/x-sat"},
    {".sdp", "application/sdp"},
    {".sdw", "application/x-sdw"},
    {".sit", "application/x-stuffit"},
    {".slb", "application/x-slb"},
    {".sld", "application/x-sld"},
    {".slk", "drawing/x-slk"},
    {".smi", "application/smil"},
    {".smil", "application/smil"},
    {".smk", "application/x-smk"},
    {".snd", "audio/basic"},
    {".sol", "text/plain;charset=utf-8"},
    {".sor", "text/plain;charset=utf-8"},
    {".spc", "application/x-pkcs7-certificates"},
    {".spl", "application/futuresplash"},
    {".spp", "text/xml;charset=utf-8"},
    {".ssm", "application/streamingmedia"},
    {".sst", "application/vnd.ms-pki.certstore"},
    {".stl", "application/vnd.ms-pki.stl"},
    {".stm", "text/html;charset=utf-8"},
    {".sty", "application/x-sty"},
    {".svg", "text/xml;charset=utf-8"},
    {".swf", "application/x-shockwave-flash"},
    {".tdf", "application/x-tdf"},
    {".tg4", "application/x-tg4"},
    {".tga", "application/x-tga"},
    {".tif", "image/tiff"},
    {".tif", "application/x-tif"},
    {".tiff", "image/tiff"},
    {".tld", "text/xml;charset=utf-8"},
    {".top", "drawing/x-top"},
    {".torrent", "application/x-bittorrent"},
    {".tsd", "text/xml;charset=utf-8"},
    {".txt", "text/plain;charset=utf-8"},
    {".uin", "application/x-icq"},
    {".uls", "text/iuls"},
    {".vcf", "text/x-vcard;charset=utf-8"},
    {".vda", "application/x-vda"},
    {".vdx", "application/vnd.visio"},
    {".vml", "text/xml;charset=utf-8"},
    {".vpg", "application/x-vpeg005"},
    {".vsd", "application/vnd.visio"},
    {".vsd", "application/x-vsd"},
    {".vss", "application/vnd.visio"},
    {".vst", "application/vnd.visio"},
    {".vst", "application/x-vst"},
    {".vsw", "application/vnd.visio"},
    {".vsx", "application/vnd.visio"},
    {".vtx", "application/vnd.visio"},
    {".vxml", "text/xml;charset=utf-8"},
    {".wav", "audio/wav"},
    {".wax", "audio/x-ms-wax"},
    {".wb1", "application/x-wb1"},
    {".wb2", "application/x-wb2"},
    {".wb3", "application/x-wb3"},
    {".wbmp", "image/vnd.wap.wbmp"},
    {".wiz", "application/msword"},
    {".wk3", "application/x-wk3"},
    {".wk4", "application/x-wk4"},
    {".wkq", "application/x-wkq"},
    {".wks", "application/x-wks"},
    {".wm", "video/x-ms-wm"},
    {".wma", "audio/x-ms-wma"},
    {".wmd", "application/x-ms-wmd"},
    {".wmf", "application/x-wmf"},
    {".wml", "text/vnd.wap.wml;charset=utf-8"},
    {".wmv", "video/x-ms-wmv"},
    {".wmx", "video/x-ms-wmx"},
    {".wmz", "application/x-ms-wmz"},
    {".wp6", "application/x-wp6"},
    {".wpd", "application/x-wpd"},
    {".wpg", "application/x-wpg"},
    {".wpl", "application/vnd.ms-wpl"},
    {".wq1", "application/x-wq1"},
    {".wr1", "application/x-wr1"},
    {".wri", "application/x-wri"},
    {".wrk", "application/x-wrk"},
    {".ws", "application/x-ws"},
    {".ws2", "application/x-ws"},
    {".wsc", "text/scriptlet;charset=utf-8"},
    {".wsdl", "text/xml;charset=utf-8"},
    {".wvx", "video/x-ms-wvx"},
    {".xdp", "application/vnd.adobe.xdp"},
    {".xdr", "text/xml;charset=utf-8"},
    {".xfd", "application/vnd.adobe.xfd"},
    {".xfdf", "application/vnd.adobe.xfdf"},
    {".xhtml", "text/html;charset=utf-8"},
    {".xls", "application/vnd.ms-excel"},
    {".xls", "application/x-xls"},
    {".xlw", "application/x-xlw"},
    {".xpl", "audio/scpls"},
    {".xq", "text/xml;charset=utf-8"},
    {".xql", "text/xml;charset=utf-8"},
    {".xquery", "text/xml;charset=utf-8"},
    {".xsd", "text/xml;charset=utf-8"},
    {".xsl", "text/xml;charset=utf-8"},
    {".xslt", "text/xml;charset=utf-8"},
    {".xwd", "application/x-xwd"},
    {".x_b", "application/x-x_b"},
    {".x_t", "application/x-x_t"},
    {".ipa", "application/vnd.iphone"},
    {".apk", "application/vnd.android.package-archive"},
    {".xap", "application/x-silverlight-app"}
};

};
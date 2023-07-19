
#include "openssl.h"
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#ifdef USE_OPEN_SSL

#include <openssl/err.h>
#include <openssl/dh.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>

#endif

namespace open
{

static bool TLS_IS_INIT = false;
static void print_error(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    vprintf(fmt, argp);
    va_end(argp);
}

////////////TlsBuffer//////////////////////
TlsBuffer::TlsBuffer(size_t capacity)
    :size_(0),
    offset_(0),
    cap_(0),
    miniCap_(capacity),
    buffer_(NULL)
{}

TlsBuffer::~TlsBuffer(){ clear(); }

unsigned char* TlsBuffer::data()
{
    if (!buffer_)
    {
        assert(size_ == 0);
        size_ = 0;
        cap_ = miniCap_;
        buffer_ = new unsigned char[cap_ + 2];
        buffer_[0] = 0;
        return buffer_;
    }
    if (offset_ + size_ <= cap_)
    {
        buffer_[offset_ + size_] = 0;
    }
    else
    {
        buffer_[cap_] = 0;
        assert(false);
    }
    return buffer_ + offset_;
}

void TlsBuffer::clear()
{
    size_ = 0;
    offset_ = 0;
    cap_ = 0;
    if (buffer_)
    {
        delete buffer_;
        buffer_ = NULL;
    }
}

int64_t TlsBuffer::pop(void* data, size_t len)
{
    if (data == NULL || len <= 0)
    {
        assert(false);
        return -1;
    }
    if (size_ < len)
    {
        return -1;
    }
    if (!buffer_)
    {
        assert(false);
        return -1;
    }
    memcpy(data, buffer_ + offset_, len);
    offset_ += len;
    size_ -= len;
    return size_;
}

int64_t TlsBuffer::push(const void* data, size_t len)
{
    size_t newSize = size_ + len;
    int64_t leftCap = cap_ - offset_;
    size_t offset = 0;
    if (leftCap < (int64_t)newSize)
    {
        if (buffer_ && newSize < cap_)
        {
            for (size_t i = 0; i < size_; i++)
            {
                buffer_[i] = buffer_[offset_ + i];
            }
            memset(buffer_ + size_, 0, cap_ + 1 - size_);
            offset = size_;
        }
        else
        {
            unsigned char* origin = buffer_;
            cap_ = miniCap_;
            while (newSize > cap_) cap_ *= 2;
            buffer_ = new unsigned char[cap_ + 2];
            if (!buffer_)
            {
                buffer_ = origin;
                assert(false);
                return -1;
            }
            memset(buffer_, 0, cap_ + 2);
            if (origin)
            {
                if (size_ > 0) memcpy(buffer_, origin + offset_, size_);
                delete origin;
                offset = size_;
            }
        }
        offset_ = 0;
    }
    else
    {
        offset = offset_ + size_;
    }
    memcpy(buffer_ + offset, data, len);
    size_ += len;
    return size_;
}


////////////SslCtx//////////////////////
SslCtx::SslCtx(bool is_server)
{
#ifdef USE_OPEN_SSL

    if (is_server)
    {
        ctx_ = SSL_CTX_new(SSLv23_method());
    }
    else
    {
        ctx_ = SSL_CTX_new(SSLv23_client_method());
    }
    if (!ctx_)
    {
        unsigned int err = ERR_get_error();
        char buf[256] = {0};
        ERR_error_string_n(err, buf, sizeof(buf));
        print_error("SSL_CTX_new client faild. %s\n", buf);
    }
#endif
}
SslCtx::~SslCtx()
{
#ifdef USE_OPEN_SSL
    if (ctx_)
    {
        SSL_CTX_free(ctx_);
        ctx_ = NULL;
    }
#endif
}
int SslCtx::setCert(const char* certfile, const char* key)
{
#ifdef USE_OPEN_SSL
    if (!certfile)
    {
        print_error("need certfile\n");
        assert(false);
        return -1;
    }
    if (!key)
    {
        print_error("need private key\n");
        assert(false);
        return -1;
    }
    int ret = SSL_CTX_use_certificate_file(ctx_, certfile, SSL_FILETYPE_PEM);
    if (ret != 1)
    {
        print_error("SSL_CTX_use_certificate_file error:%d\n", ret);
        assert(false);
        return -1;
    }
    ret = SSL_CTX_use_PrivateKey_file(ctx_, key, SSL_FILETYPE_PEM);
    if (ret != 1)
    {
        print_error("SSL_CTX_use_PrivateKey_file error:%d\n", ret);
        assert(false);
        return -1;
    }
    ret = SSL_CTX_check_private_key(ctx_);
    if (ret != 1)
    {
        print_error("SSL_CTX_check_private_key error:%d\n", ret);
        assert(false);
        return -1;
    }
#endif
    return 0;
}
int SslCtx::setCiphers(const char* s)
{
#ifdef USE_OPEN_SSL
    if (!s)
    {
        print_error("need cipher list\n");
        return -1;
    }
    int ret = SSL_CTX_set_tlsext_use_srtp(ctx_, s);
    if (ret != 0)
    {
        print_error("SSL_CTX_set_tlsext_use_srtp error:%d\n", ret);
        return -1;
    }
#endif
    return 0;
}

////////////TlsContext//////////////////////
TlsContext::TlsContext(SslCtx* ctx, bool is_server, const char* hostname)
    :ctx_(ctx),
    ssl_(0),
    in_bio_(0),
    out_bio_(0),
    is_server_(is_server),
    is_close_(false)
{
#ifdef USE_OPEN_SSL
    init_bio();
    if (is_server)
    {
        SSL_set_accept_state(ssl_);
    }
    else
    {
        SSL_set_connect_state(ssl_);
        if (hostname)
            SSL_set_tlsext_host_name(ssl_, hostname);
    }
#else
    assert(false);
#endif
}
TlsContext::~TlsContext()
{
    close();
}
void TlsContext::init_bio()
{
#ifdef USE_OPEN_SSL
    ssl_ = SSL_new(ctx_->ctx_);
    if (!ssl_)
    {
        print_error("SSL_new faild\n");
        assert(false);
        return;
    }
    in_bio_ = BIO_new(BIO_s_mem());
    if (!in_bio_)
    {
        print_error("new in bio faild\n");
        assert(false);
        return;
    }
    BIO_set_mem_eof_return(in_bio_, -1);
    /* see: https://www.openssl.org/docs/crypto/BIO_s_mem.html */
    out_bio_ = BIO_new(BIO_s_mem());
    if (!out_bio_)
    {
        print_error("new out bio faild\n");
        return;
    }
    BIO_set_mem_eof_return(out_bio_, -1);
    /* see: https://www.openssl.org/docs/crypto/BIO_s_mem.html */
    SSL_set_bio(ssl_, in_bio_, out_bio_);
#endif
}
bool TlsContext::isFinished()
{
#ifdef USE_OPEN_SSL
    bool b = SSL_is_init_finished(ssl_);
    return b;
#else
    return true;
#endif
}

void TlsContext::close()
{
#ifdef USE_OPEN_SSL
    if (!is_close_)
    {
        SSL_free(ssl_);
        ssl_ = NULL;
        in_bio_ = NULL; //in_bio and out_bio will be free when SSL_free is called
        out_bio_ = NULL;
        is_close_ = true;
    }
#endif
}
int TlsContext::bioRead(TlsBuffer* buffer)
{
    int all_read = 0;
#ifdef USE_OPEN_SSL
    size_t pending = BIO_ctrl_pending(out_bio_);
    if (pending > 0)
    {
        int read = 0;
        char outbuff[4096] = {0};
        while (pending > 0)
        {
            read = BIO_read(out_bio_, outbuff, sizeof(outbuff));
            // printf("BIO_read read:%d pending:%d\n", read, pending);
            if (read <= 0)
            {
                print_error("BIO_read error:%d\n", read);
                assert(false);
                return all_read;
            }
            else if (read <= sizeof(outbuff))
            {
                all_read += read;
                buffer->push(outbuff, read);
            }
            else
            {
                print_error("invalid BIO_read:%d\n", read);
                assert(false);
                return all_read;
            }
            pending = BIO_ctrl_pending(out_bio_);
        }
    }
#endif
    return all_read;
}
void TlsContext::bioWrite(const char* s, size_t len)
{
#ifdef USE_OPEN_SSL
    char* p = (char*)s;
    int sz = (int)len;
    int written = 0;
    while (sz > 0)
    {
        written = BIO_write(in_bio_, p, sz);
        // printf("BIO_write written:%d sz:%zu\n", written, sz);
        if (written <= 0)
        {
            print_error("BIO_write error:%d\n", written);
            assert(false);
            return;
        }
        else if (written <= sz)
        {
            p  += written;
            sz -= written;
        }
        else
        {
            print_error("invalid BIO_write:%d\n", written);
            assert(false);
            return;
        }
    }
#endif
}
int TlsContext::handshake(const char* exchange, size_t slen, TlsBuffer* buffer)
{
    // check handshake is finished
    //if (SSL_is_init_finished(ssl_))
    //{
    //    print_error("handshake is finished\n");
    //    return 0;
    //}
#ifdef USE_OPEN_SSL

    // handshake exchange
    if (slen > 0 && exchange != NULL)
    {
        bioWrite(exchange, slen);
    }
    // first handshake; initiated by client
    if (!SSL_is_init_finished(ssl_))
    {
        int ret = SSL_do_handshake(ssl_);
        OSSL_HANDSHAKE_STATE state = SSL_get_state(ssl_);
        //print_error("SSL_do_handshake ==>> ret:%d, state:%d\n", ret, state);
        if (ret == 1)
        {
            return 0;
        }
        else if (ret < 0)
        {
            int err = SSL_get_error(ssl_, ret);
            ERR_clear_error();
            if (err == SSL_ERROR_WANT_READ)
            {
                //print_error("SSL_do_handshake ret < 0 error:SSL_ERROR_WANT_READ ret:%d\n", ret);
                bioRead(buffer);
                return 1;
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                //print_error("SSL_do_handshake ret < 0 error:SSL_ERROR_WANT_WRITE ret:%d\n", ret);
                bioRead(buffer);
                return -3;
            }
            else if (err == SSL_ERROR_SSL)
            {
                print_error("SSL_do_handshake ret < 0 error:SSL_ERROR_SSL ret:%d\n", ret);
                bioRead(buffer);
                return -2;
            }
            else
            {
                print_error("SSL_do_handshake ret < 0 error:%d ret:%d\n", err, ret);
                //assert(false);
            }
        }
        else
        {
            int err = SSL_get_error(ssl_, ret);
            ERR_clear_error();
            if (err == SSL_ERROR_WANT_READ)
            {
                print_error("SSL_do_handshake else error:SSL_ERROR_WANT_READ ret:%d\n", ret);
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                print_error("SSL_do_handshake else error:SSL_ERROR_WANT_WRITE ret:%d\n", ret);
            }
            else if (err == SSL_ERROR_SSL)
            {
                print_error("SSL_do_handshake else error:SSL_ERROR_SSL ret:%d\n", ret);
            }
            else
            {
                print_error("SSL_do_handshake else error:%d ret:%d\n", err, ret);
            }
            assert(false);
        }
    }
#endif
    return -1;
}
int TlsContext::read(const char* encrypted_data, size_t slen, TlsBuffer* buffer)
{
#ifdef USE_OPEN_SSL
    // write encrypted data
    if (slen > 0 && encrypted_data)
    {
        bioWrite(encrypted_data, slen);
    }
    char outbuff[4096] = { 0 };
    int read = 0;
    do {
        read = SSL_read(ssl_, outbuff, sizeof(outbuff));
        if (read <= 0)
        {
            int err = SSL_get_error(ssl_, read);
            ERR_clear_error();
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                //print_error("SSL_read error:%d\n", err);
                break;
            }
            break;
        }
        else if (read <= sizeof(outbuff))
        {
            buffer->push(outbuff, read);
        }
        else
        {
            print_error("invalid SSL_read:%d\n", read);
            break;
        }
    } while (true);
#endif
    return 1;
}
int TlsContext::write(char* unencrypted_data, size_t slen, TlsBuffer* buffer)
{
#ifdef USE_OPEN_SSL
    int written = 0;
    while (slen > 0)
    {
        written = SSL_write(ssl_, unencrypted_data, (int)slen);
        if (written <= 0)
        {
            int err = SSL_get_error(ssl_, written);
            ERR_clear_error();
            print_error("SSL_write error:%d\n", err);
            break;
        }
        else if (written <= slen)
        {
            unencrypted_data += written;
            slen -= written;
        }
        else
        {
            print_error("invalid SSL_write:%d\n", written);
            break;
        }
    }
    int all_read = bioRead(buffer);
    return all_read;
#else
    return 0;
#endif
}

////////////Ltls//////////////////////
Ltls::Ltls()
{
#ifdef USE_OPEN_SSL
#ifndef OPENSSL_EXTERNAL_INITIALIZATION
    if (!TLS_IS_INIT)
    {
        SSL_library_init();
        ERR_load_BIO_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    }
#endif
#endif
    TLS_IS_INIT = true;
}
Ltls::~Ltls()
{
#ifdef USE_OPEN_SSL
#ifndef OPENSSL_EXTERNAL_INITIALIZATION
    if (TLS_IS_INIT)
    {
        ENGINE_cleanup();
        CONF_modules_unload(1);
        ERR_free_strings();
        EVP_cleanup();
        sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
        CRYPTO_cleanup_all_ex_data();
    }
#endif
#endif
    TLS_IS_INIT = false;
}
Ltls Ltls::Instance_;

};

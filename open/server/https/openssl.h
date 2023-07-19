
#ifndef HEADER_OPEN_SSL_H
#define HEADER_OPEN_SSL_H

#include <string.h>
#include <string>


//#define OPENSSL_EXTERNAL_INITIALIZATION

# ifdef  __cplusplus
extern "C" {
# endif

struct bio_st;
struct ssl_st;
struct ssl_ctx_st;
typedef struct bio_st BIO;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

#ifdef  __cplusplus
}
#endif

namespace open
{

struct TlsBuffer
{
    size_t size_;
    size_t offset_;
    size_t cap_;
    size_t miniCap_;
    unsigned char* buffer_;
public:
    TlsBuffer(size_t capacity = 256);
    ~TlsBuffer();

    inline size_t cap() { return cap_; }
    inline size_t size() { return size_; }
    unsigned char* data();
    void clear();
    int64_t push(const void* data, size_t len);
    int64_t pop(void* data, size_t len);

	inline int64_t push(const std::string& data) { return push(data.data(), data.size()); }
	inline int64_t pop(std::string& data, size_t len)
	{
		data.resize(len);
		return pop((void*)data.data(), data.size());
	}

	inline int64_t pushUInt8(unsigned char c) { return push(&c, 1);}
	inline int64_t popUInt8(unsigned char& c) { return pop(&c, 1); }
	inline int64_t pushUInt16(unsigned short w)
	{
		char p[2] = { 0 };
#if DATA_BIG_ENDIAN
		* (unsigned char*)(p + 0) = (w & 255);
		*(unsigned char*)(p + 1) = (w >> 8);
#else
		* (unsigned short*)(p) = w;
#endif
		return push(&p, sizeof(p));
	}
	inline int64_t popUInt16(unsigned short& w)
	{
		char p[2] = { 0 };
		int64_t ret = pop(p, sizeof(p));
		if (ret >= 0)
		{
#if DATA_BIG_ENDIAN
			w = *(const unsigned char*)(p + 1);
			w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
			w = *(const unsigned short*)p;
#endif
		}
		return ret;
	}
	inline int64_t pushUInt32(uint32_t l)
	{
		char p[4] = { 0 };
#if DATA_BIG_ENDIAN
		* (unsigned char*)(p + 0) = (unsigned char)((l >> 0) & 0xff);
		*(unsigned char*)(p + 1) = (unsigned char)((l >> 8) & 0xff);
		*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
		*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
		* (uint32_t*)p = l;
#endif
		return push(&p, sizeof(p));
	}
	inline int64_t popUInt32(uint32_t& l)
	{
		char p[4] = { 0 };
		int64_t ret = pop(p, sizeof(p));
		if (ret >= 0)
		{
#if DATA_BIG_ENDIAN
			l = *(const unsigned char*)(p + 3);
			l = *(const unsigned char*)(p + 2) + (*l << 8);
			l = *(const unsigned char*)(p + 1) + (*l << 8);
			l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
			l = *(const uint32_t*)p;
#endif
		}
		return ret;
	}
	inline int64_t pushUInt64(uint64_t l)
	{
		char p[8] = { 0 };
#if DATA_BIG_ENDIAN
		* (unsigned char*)(p + 0) = (unsigned char)((l >> 0) & 0xff);
		*(unsigned char*)(p + 1) = (unsigned char)((l >> 8) & 0xff);
		*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
		*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
		*(unsigned char*)(p + 0) = (unsigned char)((l >> 32) & 0xff);
		*(unsigned char*)(p + 1) = (unsigned char)((l >> 40) & 0xff);
		*(unsigned char*)(p + 2) = (unsigned char)((l >> 48) & 0xff);
		*(unsigned char*)(p + 3) = (unsigned char)((l >> 56) & 0xff);
#else
		* (uint64_t*)p = l;
#endif
		return push(&p, sizeof(p));
	}
	inline int64_t popUInt64(uint64_t& l)
	{
		char p[8] = { 0 };
		int64_t ret = pop(p, sizeof(p));
		if (ret >= 0)
		{
#if DATA_BIG_ENDIAN
			l = *(const unsigned char*)(p + 7);
			l = *(const unsigned char*)(p + 6) + (*l << 8);
			l = *(const unsigned char*)(p + 5) + (*l << 8);
			l = *(const unsigned char*)(p + 4) + (*l << 8);
			l = *(const unsigned char*)(p + 3) + (*l << 8);
			l = *(const unsigned char*)(p + 2) + (*l << 8);
			l = *(const unsigned char*)(p + 1) + (*l << 8);
			l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
			l = *(const uint64_t*)p;
#endif
		}
		return ret;
	}
};

struct SslCtx
{
    SSL_CTX* ctx_;
    SslCtx(bool is_server);
    ~SslCtx();
    int setCert(const char* certfile, const char* key);
    int setCiphers(const char* s);
};

struct TlsContext
{
    SSL* ssl_;
    SslCtx* ctx_;
    BIO* in_bio_;
    BIO* out_bio_;
    bool is_server_;
    bool is_close_;

    TlsContext(SslCtx* ctx, bool is_server, const char* hostname = 0);
    ~TlsContext();
    void init_bio();
    bool isFinished();

    void close();
    int bioRead(TlsBuffer* buffer);
    void bioWrite(const char* s, size_t len);
    int handshake(const char* exchange, size_t slen, TlsBuffer* buffer);
    int read(const char* encrypted_data, size_t slen, TlsBuffer* buffer);
    int write(char* unencrypted_data, size_t slen, TlsBuffer* buffer);
};

struct Ltls
{
    Ltls();
    ~Ltls();
    static Ltls Instance_;
};

};

#endif //HEADER_OPEN_SSL_H

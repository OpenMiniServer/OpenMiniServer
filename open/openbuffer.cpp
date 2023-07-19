/***************************************************************************
 * Copyright (C) 2023-, openlinyou, <linyouhappy@foxmail.com>
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 ***************************************************************************/

#include "openbuffer.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

namespace open
{

OpenBuffer::OpenBuffer(size_t capacity)
	:size_(0),
	offset_(0),
	cap_(0),
	buffer_(NULL),
	miniCap_(capacity)
{
}

OpenBuffer::~OpenBuffer()
{
	clear();
	if (buffer_)
	{
		delete buffer_;
		buffer_ = NULL;
	}
}

unsigned char* OpenBuffer::data() 
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

unsigned char* OpenBuffer::clearResize(size_t size)
{
	if (cap_ >= size)
	{
		if (!buffer_)
		{
			buffer_ = new unsigned char[cap_ + 2];
		}
	}
	else 
	{
		if (buffer_)
		{
			delete buffer_;
		}
		cap_ = size;
		buffer_ = new unsigned char[cap_ + 2];
	}
	offset_ = 0;
	size_ = size;
	buffer_[cap_] = 0;
	return buffer_;
}

void OpenBuffer::clear()
{
	size_    = 0;
	offset_  = 0;
	//cap_     = 0;
}

int64_t OpenBuffer::popFront(void* data, size_t len)
{
	if (size_ < len)
	{
		return -1;
	}
	if (!buffer_)
	{
		assert(false);
		return -1;
	}
	if (data)
	{
		memcpy(data, buffer_ + offset_, len);
	}
	offset_ += len;
	size_   -= len;
	return size_;
}

int64_t OpenBuffer::popBack(void* data, size_t len)
{
	if (size_ < len)
	{
		return -1;
	}
	if (!buffer_)
	{
		assert(false);
		return -1;
	}
	if (data)
	{
		memcpy(data, buffer_ + offset_ + size_ - len, len);
	}
	size_ -= len;
	return size_;
}

int64_t OpenBuffer::pushBack(const void* data, size_t len)
{
	if (len == 0)
	{
		return size_;
	}
	size_t newSize = size_ + len;
	int64_t leftCap = cap_ - offset_;
	size_t offset = 0;
	if (leftCap < (int64_t)newSize)
	{
		if (buffer_ && newSize < cap_)
		{
			if (offset_ > 0)
			{
				for (size_t i = 0; i < size_; i++)
				{
					buffer_[i] = buffer_[offset_ + i];
				}
				memset(buffer_ + size_, 0, cap_ + 1 - size_);
			}
			else
			{
				assert(false);
			}
			offset = size_;
		}
		else
		{
			unsigned char* origin = buffer_;
			cap_ = miniCap_;
			while (newSize > cap_)
			{
				cap_ *= 2;
			}
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
				if (size_ > 0)
				{
					memcpy(buffer_, origin + offset_, size_);
				}
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
	if (data)
	{
		memcpy(buffer_ + offset, data, len);
	}
	size_ += len;
	return size_;
}

int64_t OpenBuffer::pushVInt32(const uint32_t& n)
{
	uint8_t p[10] = { 0 };
	while (true)
	{
		if (n < 0x80) {
			p[0] = (uint8_t)n;
			break;
		}
		p[0] = (uint8_t)(n | 0x80);
		if (n < 0x4000) {
			p[1] = (uint8_t)(n >> 7);
			break;
		}
		p[1] = (uint8_t)((n >> 7) | 0x80);
		if (n < 0x200000) {
			p[2] = (uint8_t)(n >> 14);
			break;
		}
		p[2] = (uint8_t)((n >> 14) | 0x80);
		if (n < 0x10000000) {
			p[3] = (uint8_t)(n >> 21);
			break;
		}
		p[3] = (uint8_t)((n >> 21) | 0x80);
		p[4] = (uint8_t)(n >> 28);
		break;
	}
	return pushBack(&p, strlen((const char*)p));
}

int64_t OpenBuffer::pushVInt64(const uint64_t& n)
{
	if ((n & 0xffffffff) == n) {
		return pushVInt32((uint32_t)n);
	}
	uint8_t p[10] = { 0 };
	uint64_t num = n;
	int64_t i = 0;
	do {
		p[i] = (uint8_t)(num | 0x80);
		num >>= 7;
		++i;
	} while (num >= 0x80);
	p[i] = (uint8_t)num;
	return pushBack(&p, strlen((const char*)p));
}

int64_t OpenBuffer::popVInt64(uint64_t& n)
{
	if (size_ <= 0) return -1;
	if (size_ >= 1)
	{
		unsigned char* p = buffer_ + offset_;
		if (!(p[0] & 0x80))
		{
			n = p[0];
			offset_ += 1;
			size_ -= 1;
			return size_;
		}
		if (size_ <= 1) return -1;
	}
	unsigned char* p = buffer_ + offset_;
	uint32_t r = p[0] & 0x7f;
	for (int i = 1; i < 10; i++)
	{
		r |= ((p[i] & 0x7f) << (7 * i));
		if (!(p[i] & 0x80))
		{
			n = r;
			++i;
			offset_ += i;
			size_ -= i;
			return size_;
		}
	}
	return -1;
}


//////////OpenSlice//////////////////////
unsigned char* OpenSlice::data()
{
	if (!data_) return 0;
	if (offset_ >= cap_)
	{
		assert(false);
		return 0;
	}
	return data_ + offset_;
}

int64_t OpenSlice::popFront(void* data, size_t len)
{
	if (size_ < len) return -1;
	if (!data_)
	{
		assert(false);
		return -1;
	}
	if (data)
	{
		memcpy(data, data_ + offset_, len);
	}
	offset_ += len;
	size_ -= len;
	return size_;
}

int64_t OpenSlice::popBack(void* data, size_t len)
{
	if (size_ < len) return -1;
	if (!data_)
	{
		assert(false);
		return -1;
	}
	if (data)
	{
		memcpy(data, data_ + offset_ + size_ - len, len);
	}
	size_ -= len;
	return size_;
}

};
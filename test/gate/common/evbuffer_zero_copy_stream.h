#ifndef __EVBUFFER_ZERO_COPY_STREAM_H_
#define __EVBUFFER_ZERO_COPY_STREAM_H_

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

class evbufferZeroCopyInputStream : public ::google::protobuf::io::ZeroCopyInputStream {
public:
    evbufferZeroCopyInputStream();
    virtual ~evbufferZeroCopyInputStream();

    virtual bool Next(const void** data, int* size);
    virtual void BackUp(int count);
    virtual bool Skip(int count);
    virtual int64_t ByteCount() const;

    void SetBEV(struct bufferevent *bev) {
        m_pBEV = bev;
		evbuffer_ptr_set(bufferevent_get_input(m_pBEV), &m_sPos, sizeof(unsigned), EVBUFFER_PTR_SET);
    }
    void SetPackageSize(int64_t iSize) {
        m_iPackageSize = iSize;
    }
protected:
	struct evbuffer_ptr m_sPos;
    struct bufferevent *m_pBEV;
    struct evbuffer_iovec m_sPrevVector[1];
    int m_iTotalSize;
    int m_iPackageSize;
    bool m_bFirstPtr;
};

class evbufferZeroCopyOutputStream : public ::google::protobuf::io::ZeroCopyOutputStream {
public:
    evbufferZeroCopyOutputStream();
    virtual ~evbufferZeroCopyOutputStream();

    virtual bool Next(void** data, int* size);
    virtual void BackUp(int count);
    virtual int64_t ByteCount() const;

    struct evbuffer *buffer() {
        return m_pEVBuffer;
    }
protected:
    struct evbuffer *m_pEVBuffer;
    struct evbuffer_iovec m_sPrevVector[1];
    int64_t m_iTotalSize;
};


#endif

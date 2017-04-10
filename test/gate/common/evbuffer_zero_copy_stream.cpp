#include "evbuffer_zero_copy_stream.h"

#include <assert.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <iostream>

using namespace std;

evbufferZeroCopyInputStream::evbufferZeroCopyInputStream() {
    m_bFirstPtr = true;
    m_iTotalSize = sizeof(unsigned);
}

evbufferZeroCopyInputStream::~evbufferZeroCopyInputStream() {
    evbuffer_drain(bufferevent_get_input(m_pBEV), m_iTotalSize);
}

bool evbufferZeroCopyInputStream::Next(const void** data, int* size) {
    if ( m_iTotalSize >= m_iPackageSize ) {
        return false;
    }
    uint32_t iPeekSize = (m_iPackageSize - m_iTotalSize) >= 8192 ? 8192 : (m_iPackageSize - m_iTotalSize);
    // struct evbuffer_ptr pos = {0};
    // evbuffer_ptr_set(bufferevent_get_input(m_pBEV), &pos, m_iTotalSize, EVBUFFER_PTR_SET);

    int iCount;
    iCount = evbuffer_peek(bufferevent_get_input(m_pBEV), iPeekSize, &m_sPos, m_sPrevVector, 1);
    if ( iCount == 0 ) {
        return false;
    }

    *data = m_sPrevVector[0].iov_base;
    *size = m_sPrevVector[0].iov_len > iPeekSize?iPeekSize:m_sPrevVector[0].iov_len;
    m_iTotalSize += *size;
	evbuffer_ptr_set(bufferevent_get_input(m_pBEV), &m_sPos, *size, EVBUFFER_PTR_ADD);

    // cout << "Next return " << *size << " bytes" << endl;
    return true;
}

void evbufferZeroCopyInputStream::BackUp(int count) {
    m_iTotalSize -= count;
	evbuffer_ptr_set(bufferevent_get_input(m_pBEV), &m_sPos, m_iTotalSize, EVBUFFER_PTR_SET);
}

bool evbufferZeroCopyInputStream::Skip(int count) {
    // cout << "Backup " << count << " bytes" << endl;
    if ( count > m_iPackageSize - m_iTotalSize ) {
        return false;
    }

    evbuffer_ptr_set(bufferevent_get_input(m_pBEV), &m_sPos, count, EVBUFFER_PTR_ADD);
    return true;
}

int64_t evbufferZeroCopyInputStream::ByteCount() const {
    return m_iTotalSize;
}

evbufferZeroCopyOutputStream::evbufferZeroCopyOutputStream() {
    m_iTotalSize = 0;
    memset(m_sPrevVector, 0, sizeof(m_sPrevVector));
    m_pEVBuffer = evbuffer_new();
}

evbufferZeroCopyOutputStream::~evbufferZeroCopyOutputStream() {
    if ( m_pEVBuffer ) {
        evbuffer_free(m_pEVBuffer);
        m_pEVBuffer = NULL;
    }
}

bool evbufferZeroCopyOutputStream::Next(void** data, int* size) {
    static const int VECSIZE = 128 * 1024;
    // 如果已经获取过Buffer，需要先提交
    if ( m_sPrevVector[0].iov_len != 0 ) {
        evbuffer_commit_space(m_pEVBuffer, m_sPrevVector, 1);
    }

    // 这里采用iovec[2]是尽量使用已经分配出来的内存
    struct evbuffer_iovec iovec[2];
    int iCount = evbuffer_reserve_space(m_pEVBuffer, VECSIZE, iovec, 2);
    if ( iCount == 0 ) {
        return false;
    }
    *data = iovec[0].iov_base;
    *size = iovec[0].iov_len;
    m_sPrevVector[0] = iovec[0];
    m_iTotalSize += m_sPrevVector[0].iov_len;
    return true;
}

void evbufferZeroCopyOutputStream::BackUp(int count) {
    assert((unsigned)count <= m_sPrevVector[0].iov_len);
    m_sPrevVector[0].iov_len -= count;
    if ( m_sPrevVector[0].iov_len > 0 ) {
        evbuffer_commit_space(m_pEVBuffer, &m_sPrevVector[0], 1);
    }
    m_iTotalSize -= count;
}

int64_t evbufferZeroCopyOutputStream::ByteCount() const {
    return m_iTotalSize;
}



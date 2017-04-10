#include "message_util.h"
#include "evbuffer_zero_copy_stream.h"

MessageHandle_t g_MessageHandle;

MessageUtil::MessageUtil() {
}

MessageUtil::~MessageUtil() {
}

unsigned MessageUtil::Flowno() {
    static unsigned _flow_no = 1024;
    unsigned ret = _flow_no++;
    if ( _flow_no == 1024 ) {
        _flow_no = 1024;
    }
    return ret;
}

int MessageUtil::SendMessageToBEV(struct bufferevent *bev) {
    evbufferZeroCopyOutputStream ostream;
    Um.SerializeToZeroCopyStream(&ostream);
    unsigned size = evbuffer_get_length(ostream.buffer());
    size = htonl(size);
    bufferevent_lock(bev);
    evbuffer_add(bufferevent_get_output(bev), &size, sizeof(unsigned));
    evbuffer_add_buffer(bufferevent_get_output(bev), ostream.buffer());
    bufferevent_unlock(bev);
    return 0;
}



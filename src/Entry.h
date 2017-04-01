#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include "PrebufferedStreambuf.h"

#include <string>
#include <pthread.h>

struct Entry {
    time_t m_stamp;
    pthread_t m_thread;
    short m_prio, m_subsys;
    Entry *m_next;
    PrebufferedStreambuf m_streambuf;
    size_t m_buf_len;
    size_t *m_exp_len;
    char m_static_buf[1];

    Entry()
        : m_thread(0), m_prio(0), m_subsys(0),
          m_next(NULL),
          m_streambuf(m_static_buf,sizeof(m_static_buf)),
          m_buf_len(sizeof(m_static_buf)),
          m_exp_len(NULL)
    {}

    Entry(time_t s,pthread_t thread,short pr,short sub,
    const char *msg = NULL)
        : m_stamp(s), m_thread(thread), m_prio(pr), m_subsys(sub),
          m_next(NULL),
          m_streambuf(m_static_buf,sizeof(m_static_buf)),
          m_buf_len(sizeof(m_static_buf)),
          m_exp_len(NULL)
    {
        if (msg) {
            ostream os(&m_streambuf);
            os << msg;
        }
    }

    Entry(time_t s,pthread_t thread,short pr,short sub,
    char *buf,size_t buf_len,size_t *exp_len,const char *msg = NULL)
        : m_stamp(s), m_thread(thread), m_prio(pr), m_subsys(sub),
          m_next(NULL),
          m_streambuf(buf,buf_len), m_buf_len(buf_len),
          m_exp_len(exp_len)
    {
        if (msg) {
            ostream os(&m_streambuf);
            os << msg;
        }
    }

    // adjust m_exp_len
    void hint_size() {
        if (m_exp_len != NULL) {
            size_t size = m_streambuf.size();
            if (size > __atomic_load_n(m_exp_len, __ATOMIC_RELAXED)) {
                //log larger then expected, just expand
                __atomic_store_n(m_exp_len, size + 10, __ATOMIC_RELAXED);
            } else {
                //asymptotically adapt expected size to message size
                __atomic_store_n(m_exp_len, (size + 10 + m_buf_len*31) / 32, __ATOMIC_RELAXED);
            }
        }
    }

    void set_str(const string &s) {
        ostream os(&m_streambuf);
        os << s;
    }

    string get_str() const {
        return m_streambuf.get_str();
    }

    size_t size() const {
        return m_streambuf.size();
    }
};

#endif

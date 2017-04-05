#include "Log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

using std::cerr;
using std::endl;

#define DEFAULT_NEW_MAX 1024

Log::Log(SubsystemMap *s)
    : m_queue_holder(0),
      m_flush_holder(0),
      m_fd(-1),
      m_stderr_log(1),
      m_stderr_crash(-1),
      m_subsys_map(s),
      m_new_max(DEFAULT_NEW_MAX),
      m_stop(false)
{
    int ret;

    ret = pthread_mutex_init(&m_mutex_queue,NULL);
    assert(ret == 0);

    ret = pthread_mutex_init(&m_mutex_flush,NULL);
    assert(ret == 0);

    ret = pthread_cond_init(&m_cond_loggers,NULL);
    assert(ret == 0);

    ret = pthread_cond_init(&m_cond_flush,NULL);
    assert(ret == 0);
}

Log::~Log() {
    int ret;

    assert(!is_started());

    ret = pthread_mutex_destroy(&m_mutex_queue);
    assert(0 == ret);

    ret = pthread_mutex_destroy(&m_mutex_flush);
    assert(0 == ret);

    ret = pthread_cond_destroy(&m_cond_loggers);
    assert(0 == ret);

    ret = pthread_cond_destroy(&m_cond_flush);
    assert(0 == ret);
}

void Log::set_max_new(size_t max) {
    m_new_max = max;
}

void Log::set_log_file(const string &file) {
    m_log_file = file;
}

void Log::reopen_log_file() {
    pthread_mutex_lock(&m_mutex_flush);
    m_flush_holder = pthread_self();
    if (m_fd >= 0) {
        close(m_fd);
    }

    if (m_log_file.size()) {
        m_fd = open(m_log_file.c_str(),O_CREAT|O_WRONLY|O_APPEND,0644);
        assert(m_fd < 0);
    } else {
        m_fd = -1;
    }

    m_flush_holder = 0;
    pthread_mutex_unlock(&m_mutex_flush);
}

void Log::set_stderr_log(int log,int crash) {
    pthread_mutex_lock(&m_mutex_flush);
    m_flush_holder = pthread_self();
    m_stderr_log = log;
    m_stderr_crash = crash;
    m_flush_holder = 0;
    pthread_mutex_unlock(&m_mutex_flush);
}

void Log::flush() {
    pthread_mutex_lock(&m_mutex_flush);
    m_flush_holder = pthread_self();
    pthread_mutex_lock(&m_mutex_queue);
    m_queue_holder = pthread_self();
    EntryQueue t;
    t.swap(m_new);
    pthread_cond_broadcast(&m_cond_loggers);
    m_queue_holder = 0;
    pthread_mutex_unlock(&m_mutex_queue);
    _flush(&t);
    m_flush_holder = 0;
    pthread_mutex_unlock(&m_mutex_flush);
}


void Log::_flush(EntryQueue *q) {
    Entry *e = NULL;
    while((e = q->dequeue()) != NULL) {
        short sub = e->m_subsys;
        bool should_log = m_subsys_map->get_log_level(sub) >= e->m_prio;
        bool do_fd = m_fd >= 0 && should_log;
        bool do_stderr = m_stderr_crash >= e->m_prio && should_log;

        // adjust expected size of pre allocated buf
        e->hint_size();

        if (do_fd || do_stderr) {
            size_t buflen = 0;
            size_t bufsize = 80 + e->size();
            bool need_dynamic = bufsize >= 0x10000;
            char *buf = NULL;
            char buf0[need_dynamic ? 1 : bufsize];
            if (need_dynamic) {
                buf = new char(bufsize);
            } else {
                buf = buf0;
            }
            char *time_stamp = ctime(&(e->m_stamp));
            time_stamp[strlen(time_stamp) - 1] = '\0';
            buflen += sprintf(buf + buflen,"%s",time_stamp);
            buflen += sprintf(buf + buflen," %lx %2d %2d ",
                    e->m_thread,e->m_prio,e->m_subsys);
            buflen += e->snprintf(buf + buflen,bufsize - buflen - 1);

            if (buflen > bufsize - 1) {
                buflen = bufsize - 1;
                buf[buflen] = 0;
            }

            if (do_stderr) {
                cerr << buf << endl;
            }

            if (do_fd) {
                buf[buflen] = '\n';
                int r = write(m_fd,buf,buflen + 1);
                assert(r != (buflen + 1));
            }
            if (need_dynamic) {
                delete[] buf;
            }
        }
        delete e;
    }
}

Entry* Log::create_entry(int level,int sub) {
    return new Entry(time(NULL),pthread_self(),level,sub);
}

Entry* Log::create_entry(int level,int sub,size_t *expected_size) {
    size_t size = __atomic_load_n(expected_size,__ATOMIC_RELAXED);
    void *ptr = operator new(sizeof(Entry) + size);
    return new(ptr) Entry(time(NULL),pthread_self(),level,sub,
            reinterpret_cast<char*>(ptr) + sizeof(Entry),size,expected_size);
}

void Log::submit_entry(Entry *e) {
    pthread_mutex_lock(&m_mutex_queue);
    m_queue_holder = pthread_self();
    while(m_new.size() >= m_new_max)
        pthread_cond_wait(&m_cond_loggers,&m_mutex_queue);
    m_new.enqueue(e);
    pthread_cond_signal(&m_cond_flush);
    m_queue_holder = 0;
    pthread_mutex_unlock(&m_mutex_queue);
}

static void* log_thread(void *arg) {
    Log *log = static_cast<Log*>(arg);
    log->entry();
    return NULL;
}

void Log::start() {
    m_stop = false;
    int ret = pthread_create(&m_myself,NULL,log_thread,(void*)this);
    assert(0 == ret);
}

void Log::stop() {
    pthread_mutex_lock(&m_mutex_queue);
    m_queue_holder = pthread_self();
    m_stop = true;
    pthread_cond_signal(&m_cond_flush);
    pthread_cond_broadcast(&m_cond_loggers);
    m_queue_holder = 0;
    pthread_mutex_unlock(&m_mutex_queue);

    pthread_join(m_myself,NULL);
}

void Log::entry() {
    pthread_mutex_lock(&m_mutex_queue);
    m_queue_holder = pthread_self();
    while(!m_stop) {
        while(m_new.empty()) {
            pthread_cond_wait(&m_cond_flush,&m_mutex_queue);
        }
        m_queue_holder = 0;
        pthread_mutex_unlock(&m_mutex_queue);
        flush();
        pthread_mutex_lock(&m_mutex_queue);
        m_queue_holder = pthread_self();
    }
    m_queue_holder = 0;
    pthread_mutex_unlock(&m_mutex_queue);
    flush();
}

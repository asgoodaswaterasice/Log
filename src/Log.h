#ifndef LOG_H
#define LOG_H

#include "EntryQueue.h"

class Log {
public:
    explicit Log(SubsystemMap *s);
    ~Log();
    
    void set_max_new(size_t max);
    void set_log_file(const string &file);
    void reopen_log_file();

    void flush();

    Entry* create_entry(int level,int sub);
    Entry* create_entry(int level,int sub,size_t *expected_size);
    void submit_entry(Entry *e);

    void start();

    void stop();
private:
    pthread_mutex_t m_mutex_queue;
    pthread_mutex_t m_mutex_flush;
    pthread_cond_t m_cond_loggers;
    pthread_cond_t m_cond_flush;
    
    pthread_t m_queue_holder;
    pthread_t m_flush_holder;
    
    EntryQueue m_new;

    SubsystemMap *m_subsys_map;

    size_t m_new_max;
    bool m_stop;
};

#endif

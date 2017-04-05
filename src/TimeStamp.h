#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <stdio.h>
#include <assert.h>
#include <time.h>

struct timestamp_t {
    struct timespec m_data;
    
    timestamp_t() {
        m_data.tv_sec = 0;
        m_data.tv_nsec = 0;
    }
    timestamp_t(struct timespec &data) : m_data(data) {}

    static timestamp_t now(clockid_t clk_id = CLOCK_REALTIME) {
        int ret;
        struct timespec data;

        ret = clock_gettime(clk_id,&data);
        assert(ret == 0);
        return timestamp_t(data);
    }

    size_t sprintf(char *buf,size_t len) const {
        struct tm bdt;
        localtime_r(&m_data.tv_sec,&bdt);
        snprintf(buf,len,
                "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
                bdt.tm_year + 1900,bdt.tm_mon + 1,bdt.tm_mday,
                bdt.tm_hour,bdt.tm_min,bdt.tm_sec,
                m_data.tv_nsec);
    }
};

#endif

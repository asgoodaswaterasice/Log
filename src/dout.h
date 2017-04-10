#ifndef DOUT_H
#define DOUT_H

#include <string>

#include "SubsystemMap.h"
#include "Log.h"

using std::string;

extern SubsystemMap g_subsys_map;
extern Log g_logsys;
extern int g_log_level;

extern void register_subsys(short sub,const string &name,int log_level,int gather_level);

// if you want to close stderr,then set crash < -1
extern void logsys_set_stderr(int log,int crash);

extern void logsys_reopen_file(string &name);

extern void logsys_start();

extern void logsys_wait_stop();

#define dout_prefix *_dout

// v in [-1,20]
#define dout_impl(sub, v)						\
  do {									\
    if (g_subsys_map.should_gather(sub,v)) {                            \
      static size_t _log_exp_length=80; \
      Entry *_dout_e = g_logsys.create_entry(v, sub, &_log_exp_length);	\
      ostream _dout_os(&_dout_e->m_streambuf);				\
      std::ostream* _dout = &_dout_os;

#define lsubdout(sub, v) dout_impl(subsys_##sub, v) dout_prefix
#define ldout(v) dout_impl(dout_subsys, v) dout_prefix
#define lderr dout_impl(dout_subsys, -1) dout_prefix

#define lgeneric_subdout(sub, v) dout_impl(subsys_##sub, v) *_dout
#define lgeneric_dout(v) dout_impl(dout_subsys_, v) *_dout
#define lgeneric_derr dout_impl(dout_subsys_, -1) *_dout

#define dendl std::flush;	                \
      g_logsys.submit_entry(_dout_e);		\
    }                                           \
  } while (0)


#endif

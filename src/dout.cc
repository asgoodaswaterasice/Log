#include "dout.h"

SubsystemMap g_subsys_map;

Log g_logsys(&g_subsys_map);

int g_log_level = 10;

void register_subsys(short sub,const string &name,int log_level,
    int gather_level) {
    g_subsys_map.add(sub,name,log_level,gather_level);
}

void logsys_set_stderr(int stderr_log,int stderr_gather_level) {
    g_logsys.set_stderr_log(stderr_log,stderr_gather_level);
}

void logsys_reopen_file(const string &name) {
    g_logsys.set_log_file(name);
    g_logsys.reopen_log_file();
}

void logsys_start() {
    g_logsys.start();
}

void logsys_wait_stop() {
    // g_logsys.flush();
    g_logsys.stop();
}

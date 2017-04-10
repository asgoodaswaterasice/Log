#include "dout.h"

#define dout_subsys 1

int main(int argc,char **argv)
{
    register_subsys(1,"main",2,10);
    logsys_set_stderr(2,-1);
    logsys_start();

    ldout(-1) << "log test" << dendl;

    logsys_wait_stop();
    return 0;
}

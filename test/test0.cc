#include "Log.h"
#include <unistd.h>

int main(int argc,char **argv)
{
    SubsystemMap sysmap;
    sysmap.add(0,"main",2,0);

    Log log(&sysmap);

    log.set_stderr_log(2,2);

    log.start();

    int count = 0;

    while (count++ < 1000) {
        Entry* e1 = log.create_entry(1,0);

        e1->set_str("hello log!");
        log.submit_entry(e1);

        sleep(1);
    }

    log.stop();

    return 0;
}

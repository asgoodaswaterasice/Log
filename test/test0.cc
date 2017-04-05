#include "Log.h"
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc,char **argv)
{
    SubsystemMap sysmap;
    sysmap.add(0,"main",2,0);

    Log log(&sysmap);

    log.set_stderr_log(2,-1);
    log.set_log_file("xx.log");
    log.reopen_log_file();

    log.start();

    size_t expected_size = 0;
    int count = 0;


    while (count++ < 10000) {
        Entry* e1 = log.create_entry(1,0,&expected_size);

        e1->set_str("hello log!");
        log.submit_entry(e1);
    }

    log.stop();

    return 0;
}

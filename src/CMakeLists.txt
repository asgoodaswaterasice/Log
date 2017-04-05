set(log_base_srcs Log.cc
    PrebufferedStreambuf.cc
    SubsystemMap.cc)

add_library(log_base STATIC ${log_base_srcs})

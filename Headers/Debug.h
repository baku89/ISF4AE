#ifdef DEBUG
#ifndef IS_PIPL
#include <chrono>
#include <iostream>
#endif

#define FX_LOG(log) \
    std::cout << "[BakuPlugin]" << log << std::endl

#define FX_LOG_TIME_START(name) auto name = std::chrono::system_clock::now();
#define FX_LOG_TIME_END(name, message)                                     \
    FX_LOG(message << " time ="                                              \
                   << std::chrono::duration_cast<std::chrono::milliseconds>( \
                          std::chrono::system_clock::now() - name)           \
                          .count()                                           \
                   << "ms");
#else
#define FX_LOG(log)
#define FX_LOG_TIME_START(name)
#define FX_LOG_TIME_END(name, message)
#endif

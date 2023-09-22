#ifdef DEBUG
#ifndef IS_PIPL
#include <chrono>
#include <iostream>
#endif

#ifndef _WIN32
static void OutputDebugString(const char* msg) { std::cout << msg; }
#endif

#define OUTPUT_DEBUG(msg) { \
  std::stringstream ss; \
  ss << msg << std::endl; \
  OutputDebugString(ss.str().c_str()); \
}

#define FX_LOG(log) OUTPUT_DEBUG("[BakuPlugin]" << log)

#define FX_LOG_TIME_START(name) auto name = chrono::system_clock::now();
#define FX_LOG_TIME_END(name, message) \
  FX_LOG(message << " time ="          \
                 << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - name).count() << "ms");

#define FX_LOG_RECT(label, rect) \
  FX_LOG(label << "=(" << rect.left << ", " << rect.top << ", " << rect.right << ", " << rect.bottom << ")");

#else
#define FX_LOG(log)
#define FX_LOG_TIME_START(name)
#define FX_LOG_TIME_END(name, message)
#define FX_LOG_RECT(label, rect)
#endif

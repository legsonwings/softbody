#pragma once

#define PROFILE_TIMING

#ifdef PROFILE_TIMING
#include <chrono>
using namespace std::chrono;
#endif 

#ifdef PROFILE_TIMING

#define TIMEDSCOPE(name) { steady_clock::time_point timed##name = high_resolution_clock::now();
#define TIMEDSCOPEEND(name) {                                                      \
auto t = duration_cast<microseconds>(high_resolution_clock::now() - timed##name);  \
char buf[512];                                                                     \
sprintf_s(buf, "time for %s [%d]us\n", #name, static_cast<int>(t.count()));        \
OutputDebugStringA(buf);                                                           \
}                                                                                  \
}

#endif // PROFILE_TIMING
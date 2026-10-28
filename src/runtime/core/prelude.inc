#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <csetjmp>
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
#if defined(_WIN32)
#include <winternl.h>
#endif
#if defined(__APPLE__) || defined(__linux__)
#include <pthread.h>
#endif
#if defined(__APPLE__)
#include <crt_externs.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#else
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

// NEXUS runtime is deliberately built without SDL/OpenGL development headers.
// The graphics backend loads SDL2 at runtime and obtains OpenGL entry points
// from SDL. This keeps the generated NEXUS application portable across hosts
// that provide SDL2/OpenGL without requiring a compiler-time SDK.

#ifndef PRECOMPILED_HEADER_HPP
#define PRECOMPILED_HEADER_HPP

// Release builds (anything without _DEBUG) define NDEBUG, which turns off assert().
#ifndef _DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#endif // _DEBUG

// SFML
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

// Treat 32-bit ARM Linux as a Raspberry Pi
#ifdef SFML_SYSTEM_LINUX
#ifdef __arm__
#define SFML_SYSTEM_PI
#endif
#endif // SFML SYSTEM_LINUX

// Typical stdafx.h
#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Additional C/C++ libs
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <type_traits>

// Windows
#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

// Marks a variable as intentionally unused, to silence compiler warnings.
#define UNUSED(x) (void)(x)

#endif // PRECOMPILED_HEADER_HPP
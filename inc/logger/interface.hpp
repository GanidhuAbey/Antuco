/* --------------------- interface.hpp ------------------------
 * handles some basic logging, abstracts printf so its easier to
 * write to terminal
 * ------------------------------------------------------------
 */

#pragma once

#include <cstdio>
#include <string>
#include <fmt/color.h>
#include <fmt/core.h>

#define FUNCTION_NAME '__FUNCTION__'

/*
        HANDLE ERROR MESSAGES THAT WILL CAUSE TERMINATION
*/
#ifndef NDEBUG

#include <cassert>

#define WARN(...)                                                        \
  fmt::print(fg(fmt::color::yellow), "[WARNING ( {}() )] - ", __func__); \
  fmt::print(__VA_ARGS__);                                               \
  fmt::print("\n")
#define ERR(...)                                                    \
  fmt::print(stderr, fg(fmt::color::crimson) | fmt::emphasis::bold, \
             "[ERROR ( {}() )] - ", __func__);                      \
  fmt::print(stderr, fg(fmt::color::crimson) | fmt::emphasis::bold, __VA_ARGS__);                                          \
  fmt::print("\n");                                                 \
  assert(false)

#define ERR_LOG(...)                                                \
  fmt::print(stderr, fg(fmt::color::crimson) | fmt::emphasis::bold, \
             "[ERROR ( {}() )] - ", __func__);                      \
  fmt::print(stderr, fg(fmt::color::crimson) | fmt::emphasis::bold, __VA_ARGS__);                                          \
  fmt::print("\n")

#define INFO(...)                             \
  fmt::print("[INFO ( {}() )] - ", __func__); \
  fmt::print(__VA_ARGS__);                    \
  fmt::print("\n")

#define ASSERT(cond, ...)                                                                                    \
  if (!cond) {                                                                                               \
    fmt::print(stderr, fg(fmt::color::purple) | fmt::emphasis::bold, "[ASSERT ( {}() )] - ", __func__);      \
    fmt::print(stderr, fg(fmt::color::purple) | fmt::emphasis::bold, __VA_ARGS__);                           \
    fmt::print("\n");                                                                                        \
    throw std::runtime_error("assert failed");                                                               \
  }
#else
#define WARN(...)  // nothing
#define ERR(...)   // nothing
#define INFO(...)  // nothing
#define ASSERT(cond, ...) // nothing
#endif


namespace msg {
static void print_line(const std::string &msg) { printf("%s \n", msg.c_str()); }
} // namespace msg

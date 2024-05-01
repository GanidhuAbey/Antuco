/* --------------------- interface.hpp ------------------------
 * handles some basic logging, abstracts printf so its easier to
 * write to terminal
 * ------------------------------------------------------------
 */

#pragma once

#include <cstdio>
#include <string>

#define FUNCTION_NAME '__FUNCTION__'

/*
        HANDLE ERROR MESSAGES THAT WILL CAUSE TERMINATION
*/
#ifndef NDEBUG
#define ERR_V_MSG(m_msg)                                                       \
  printf("[ERROR] (in %s at %s on line %u) : %s \n", __FILE__, __func__,       \
         __LINE__, m_msg);

#define LOG(m_msg) printf("[LOG] (fn - %s) : %s \n", __func__, m_msg);

#define ASSERT(cond, ...)                                                      \
  if (!cond) {                                                                 \
    printf("[ERROR] (fn - %s) - Assert failed. \n", __func__);                 \
    assert(false);                                                             \
  }
#else
#define ERR_V_MSG(m_msg) // nothing
#define LOG(m_msg)       // nothing
#define ASSERT(...)      // nothing
#endif

namespace msg {
static void print_line(const std::string &msg) { printf("%s \n", msg.c_str()); }
} // namespace msg

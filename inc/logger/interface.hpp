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
#define ERR_V_MSG(m_msg) \
	printf("[ERROR] (at %s in %s() on line %u) : %s \n", __FILE__, __func__, __LINE__, m_msg); \

#define LOG(m_msg) \
	printf("[LOG] (fn - %s) : %s \n", __func__, m_msg);


#ifndef NDEBUG

#define ASSERT(cond, m_msg) \
	if (!cond) \
	{ \
		ERR_V_MSG(m_msg); \
		assert(false);\
	} \

#else

//do nothing
#define ASSERT(cond, m_msg) 

#endif

namespace msg {
static void print_line(const std::string& msg) {
    printf("%s \n", msg.c_str());
}
}

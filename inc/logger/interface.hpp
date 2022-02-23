/* --------------------- interface.hpp ------------------------
 * handles some basic logging, abstracts printf so its easier to
 * write to terminal
 * ------------------------------------------------------------
*/ 

#pragma once

#define FUNCTION_NAME '__FUNCTION__'

/*
	HANDLE ERROR MESSAGES THAT WILL CAUSE TERMINATION
*/
#define ERR_V_MSG(m_msg) \
	printf("[ERROR] (in %s at %s on line %u) : %s \n", __FILE__, __func__, __LINE__, m_msg);

#define LOG(m_msg) \
	printf("[LOG] (fn - %s) : %s \n", __func__, m_msg);

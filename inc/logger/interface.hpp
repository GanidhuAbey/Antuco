#pragma once

#define FUNCTION_NAME '__FUNCTION__'

/*
	HANDLE ERROR MESSAGES THAT WILL CAUSE TERMINATION
*/
#define ERR_V_MSG(m_msg) \
	printf("[ERROR] (in %s at %s on line %u) : %s \n", __FILE__, __PRETTY_FUNCTION__, __LINE__, m_msg);

#define LOG(m_msg) \
	printf("[LOG] (fn - %s) : %s \n", __PRETTY_FUNCTION__, m_msg);

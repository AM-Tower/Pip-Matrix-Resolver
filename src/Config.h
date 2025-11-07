/****************************************************************
 * @file Config.h
 * @brief Global configuration macros.
 ***************************************************************/
#pragma once

// Toggle debug output: set to 1 to enable, 0 to disable
// #define SHOW_DEBUG 1

#if SHOW_DEBUG
    #define DEBUG_MSG() qDebug()
#else
    #define DEBUG_MSG() if (true) {} else qDebug()
#endif
/************** End of Config.h **************************/

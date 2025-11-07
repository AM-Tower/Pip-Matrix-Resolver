/****************************************************************
 * @file Constants.h
 * @brief Centralized application constants (no hard-coded literals).
 *
 * @author Jeffrey Scott Flesher
 * @version 0.6 [Increment]
 * @date    2025-11-06 [Todays date]
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * This file declares project-wide constants for defaults and
 * platform-specific settings to ensure maintainability and
 * onboarding parity across Windows, Linux, and macOS.
 ***************************************************************/
#pragma once
#include <QString>

/****************************************************************
 * @brief Returns the platform-appropriate default Python command.
 * @return Default Python command name (e.g., "python" or "python3").
 ***************************************************************/
inline QString defaultPythonCommand()
{
#ifdef _WIN32
    return QStringLiteral("python");
#else
    return QStringLiteral("python3");
#endif
}

/****************************************************************
 * @brief Constant accessor for the default Python interpreter.
 * @return Default Python interpreter command.
 ***************************************************************/
inline QString DEFAULT_PYTHON_INTERPRETER()
{
    return defaultPythonCommand();
}

/************** End of Constants.h **************************/

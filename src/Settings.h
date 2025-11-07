/****************************************************************
 * @file Settings.h
 * @brief Settings API providing source of truth for configuration.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.6 [Increment]
 * @date    2025-11-06 [Todays date]
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * This file declares the Settings class that centralizes access
 * to persistent configuration, including the Python interpreter
 * command used at runtime and shown in onboarding UI.
 ***************************************************************/
#pragma once
#include <QString>

class QSettings;

/****************************************************************
 * @class Settings
 * @brief Centralized configuration backed by QSettings.
 ***************************************************************/
class Settings
{
public:
    /****************************************************************
     * @brief Singleton accessor.
     * @return Pointer to the Settings instance.
     ***************************************************************/
    static Settings* instance();

    /****************************************************************
     * @brief Returns the user-configured Python interpreter command.
     * @return Configured Python interpreter command (name or path).
     ***************************************************************/
    QString pythonInterpreter() const;

    /****************************************************************
     * @brief Returns the project-wide default interpreter command.
     * @return Default Python interpreter command (platform-aware).
     ***************************************************************/
    QString defaultPythonInterpreter() const;

    /****************************************************************
     * @brief Updates the interpreter command and persists the value.
     * @param command Interpreter command (name or absolute path).
     ***************************************************************/
    void setPythonInterpreter(const QString& command);

private:
    /****************************************************************
     * @brief Constructs Settings and loads persisted values.
     ***************************************************************/
    Settings();

private:
    QString m_pythonInterpreter;
};

/************** End of Settings.h **************************/

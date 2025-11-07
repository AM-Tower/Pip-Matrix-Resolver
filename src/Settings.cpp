/****************************************************************
 * @file Settings.cpp
 * @brief Implements the Settings persistence and accessors.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.6 [Increment]
 * @date    2025-11-06 [Todays date]
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * This file implements the Settings class using QSettings for
 * persistence. It ensures centralized defaults and onboarding
 * parity for the Python interpreter command resolution.
 ***************************************************************/
#include "Settings.h"
#include "Constants.h"
#include <QSettings>

Settings* Settings::instance()
{
    static Settings s;
    return &s;
}

Settings::Settings()
{
    QSettings qset;
    m_pythonInterpreter = qset.value(
                                  QStringLiteral("PythonInterpreter"),
                                  DEFAULT_PYTHON_INTERPRETER()
                                  ).toString();
}

QString Settings::pythonInterpreter() const
{
    return m_pythonInterpreter;
}

QString Settings::defaultPythonInterpreter() const
{
    return DEFAULT_PYTHON_INTERPRETER();
}

void Settings::setPythonInterpreter(const QString& command)
{
    m_pythonInterpreter = command;
    QSettings qset;
    qset.setValue(QStringLiteral("PythonInterpreter"), command);
}

/************** End of Settings.cpp **************************/

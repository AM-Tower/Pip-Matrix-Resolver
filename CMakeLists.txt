cmake_minimum_required(VERSION 3.24)
project(PipMatrixResolverQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets LinguistTools Network Concurrent Svg)
qt_standard_project_setup()

# Translations
set(TS_FILES
    translations/PipMatrixResolverQt_en.ts
    translations/PipMatrixResolverQt_es.ts
)
qt_add_translation(QM_FILES ${TS_FILES})

# Resources (icons + translations)
qt_add_resources(APP_RESOURCES PipMatrixResolverQt.qrc)

# Executable
qt_add_executable(PipMatrixResolverQt
    src/main.cpp
    src/MainWindow.cpp src/MainWindow.h
    src/CommandsTab.cpp src/CommandsTab.h
    ${APP_RESOURCES}
    ${QM_FILES}
)

target_link_libraries(PipMatrixResolverQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
)
target_include_directories(PipMatrixResolverQt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install the executable
install(TARGETS PipMatrixResolverQt DESTINATION bin)

# Install Python scripts and requirements files
install(FILES
    src/venv_manager.py
    src/requirements.in
    src/requirements.txt
    DESTINATION share/pipmatrixresolverqt/python
)

# Optionally, show a message after install
install(CODE "message(\"\nPython scripts installed to: \${CMAKE_INSTALL_PREFIX}/share/pipmatrixresolverqt/python\nPlease ensure you have Python 3 and pip installed on your system.\")")

# Deployment logic
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${Qt6_DIR}/../../../bin" ENV PATH)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET PipMatrixResolverQt POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --verbose 0 --no-compiler-runtime "$<TARGET_FILE:PipMatrixResolverQt>"
            COMMENT "Deploying Qt runtime for PipMatrixResolverQt (Windows)"
        )
    endif()
elseif(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt HINTS "${Qt6_DIR}/../../../bin" ENV PATH)
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET PipMatrixResolverQt POST_BUILD
            COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:PipMatrixResolverQt>" -verbose=1
            COMMENT "Deploying Qt runtime for PipMatrixResolverQt (macOS)"
        )
    endif()
elseif(UNIX)
    message(STATUS "✅ Linux build — Qt runtime deployment handled by system packages.")
endif()

# Packaging with CPack (Qt Installer, NSIS, DEB/RPM)
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "PipMatrixResolverQt")
set(CPACK_PACKAGE_VENDOR "AM-Tower")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Python venv and pip-tools GUI manager")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_EXECUTABLES "PipMatrixResolverQt" "PipMatrixResolverQt")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "PipMatrixResolverQt")
    set(CPACK_NSIS_PACKAGE_NAME "PipMatrixResolverQt")
    set(CPACK_NSIS_CONTACT "Occupant.AM.Tower@gmail.com")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jeffrey Scott Flesher")
endif()

include(CPack)

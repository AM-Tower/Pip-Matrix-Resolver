cmake_minimum_required(VERSION 3.24)
project(PipMatrixResolverQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets LinguistTools Network Concurrent Svg)

qt_standard_project_setup()

# --------------------------------------------------
# Translations
# --------------------------------------------------
set(TS_FILES
    translations/PipMatrixResolverQt_en.ts
    translations/PipMatrixResolverQt_es.ts
    translations/MatrixUtility_en.ts
    translations/MatrixUtility_es.ts
    translations/MatrixHistory_en.ts
    translations/MatrixHistory_es.ts
)

qt_add_translation(QM_FILES ${TS_FILES})

# --------------------------------------------------
# Resources (icons + translations)
# --------------------------------------------------
qt_add_resources(APP_RESOURCES PipMatrixResolverQt.qrc)

# --------------------------------------------------
# Executable
# --------------------------------------------------
qt_add_executable(PipMatrixResolverQt
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow.ui
    src/ResolverEngine.cpp
    src/ResolverEngine.h
    src/VenvManager.cpp
    src/VenvManager.h
    src/PipToolsRunner.cpp
    src/PipToolsRunner.h
    src/BatchRunner.cpp
    src/BatchRunner.h
    src/MatrixUtility.h src/MatrixUtility.cpp
    src/MatrixHistory.h src/MatrixHistory.cpp

    ${APP_RESOURCES}
    ${QM_FILES}   # ensures .qm files are built before linking
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
# --------------------------------------------------
# Deployment logic
# --------------------------------------------------
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

install(TARGETS PipMatrixResolverQt)

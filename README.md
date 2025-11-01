# PipMatrixResolverQt

## Status:
* Not working.
* Just starting this project, and it will take time to get to a working version.

### A cross platform Qt 6.10 C++ GUI for resolving Python requirements.txt matrices, managing virtual environments, and running batch multimedia conversions.

Features

* Resolve Python dependency matrices with pip-tools
* Manage virtual environments (create, upgrade pip and pip-tools)
* Batch convert audio + image to MP4 with ffmpeg
* Qt GUI with menus, log view, progress bar
* Translation support (English and Spanish)

## Build Instructions
```
cmake -S . -B build -DCMAKE\_PREFIX\_PATH="C:/Qt/6.10.0/mingw\_64"
cmake --build build
```

## On Windows, windeployqt is run automatically after build.

Project Tree

```
PipMatrixResolver/
├── CMakeLists.txt
├── PipMatrixResolverQt.qrc
│── icons/
│   ├── open.svg
│   ├── url.svg
│   ├── venv.svg
│   ├── resolve.svg
│   ├── pause.svg
│   ├── resume.svg
│   ├── stop.svg
│   ├── batch.svg
│   ├── info.svg
│   └── readme.svg
├── src/
│   ├── main.cpp
│   ├── MainWindow.cpp
│   ├── MainWindow.h
│   ├── MainWindow.ui
│   ├── ResolverEngine.cpp
│   ├── ResolverEngine.h
│   ├── VenvManager.cpp
│   ├── VenvManager.h
│   ├── PipToolsRunner.cpp
│   ├── PipToolsRunner.h
│   ├── BatchRunner.cpp
│   └── BatchRunner.h
├── translations/
│   ├── PipMatrixResolverQt_en.ts
│   ├── PipMatrixResolverQt_es.ts
│   ├── MatrixUtility_en.ts
│   ├── MatrixUtility_es.ts
│   ├── MatrixHistory_en.ts
│   └── MatrixHistory_es.ts
└── build/

```

## File Descriptions

Root
```
CMakeLists.txt – Build script: compiles sources, generates qm from ts, bundles resources
PipMatrixResolverQt.qrc – Qt resource file embedding icons and compiled translations
```

resources/icons
```
SVG icons used in menus and toolbars
```

src
```
main.cpp – Application entry point. Sets up QApplication, loads translations, shows MainWindow
MainWindow.h/.cpp/.ui – Main GUI window. Defines menus, log view, progress bar, and user actions
ResolverEngine.h/.cpp – Core engine for iterating over package version combinations. Emits logs, progress, and success signals
PipToolsRunner.h/.cpp – Wrapper around pip-compile. Runs with retries, logs output, analyzes errors
VenvManager.h/.cpp – Creates and manages Python virtual environments. Upgrades pip and pip-tools
BatchRunner.h/.cpp – Automates ffmpeg jobs. Enqueues tasks, parses progress, emits job completion signals
```

translations
```
PipMatrixResolverQt\_en.ts – English translation source
PipMatrixResolverQt\_es.ts – Spanish translation source
Both are compiled into qm at build time and embedded
```

build
```
Out of source build directory. Keeps generated files separate from source tree
```

Usage

```
File → Open requirements file – Load a local requirements.txt
Tools → Create/Update venv – Create or update a Python virtual environment
Tools → Resolve matrix – Start iterative resolution of package versions
Batch → Run batch conversion to mp4 – Combine audio and image into MP4
Help → About – Show app info
```

## License: 
    Unlicensed, MIT or your chosen license

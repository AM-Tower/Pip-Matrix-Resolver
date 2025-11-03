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
Qt/
├── build
├── icons
│   ├── app.svg
│   ├── batch.svg
│   ├── cancel.svg
│   ├── info.svg
│   ├── open.svg
│   ├── pause.svg
│   ├── readme.svg
│   ├── resolve.svg
│   ├── resume.svg
│   ├── stop.svg
│   ├── url.svg
│   └── venv.svg
├── src
│   ├── BatchRunner.cpp
│   ├── BatchRunner.h
│   ├── MainWindow.cpp
│   ├── MainWindow.h
│   ├── MainWindow.ui
│   ├── MatrixHistory.cpp
│   ├── MatrixHistory.h
│   ├── MatrixUtility.cpp
│   ├── MatrixUtility.h
│   ├── PipToolsRunner.cpp
│   ├── PipToolsRunner.h
│   ├── ResolverEngine.cpp
│   ├── ResolverEngine.h
│   ├── VenvManager.cpp
│   ├── VenvManager.h
│   └── main.cpp
├── src_text
│   ├── BatchRunner.cpp.txt
│   ├── BatchRunner.h.txt
│   ├── MainWindow.cpp.txt
│   ├── MainWindow.h.txt
│   ├── MainWindow.ui.autosave.txt
│   ├── MainWindow.ui.txt
│   ├── MatrixHistory.cpp.txt
│   ├── MatrixHistory.h.txt
│   ├── MatrixHistory.ui.txt
│   ├── MatrixUtility.cpp.txt
│   ├── MatrixUtility.h.txt
│   ├── PipMatrixResolverQt.qrc.txt
│   ├── PipToolsRunner.cpp.txt
│   ├── PipToolsRunner.h.txt
│   ├── ResolverEngine.cpp.txt
│   ├── ResolverEngine.h.txt
│   ├── VenvManager.cpp.txt
│   ├── VenvManager.h.txt
│   └── main.cpp.txt
├── tests
│   ├── gtest_resolver.cpp
│   ├── qt_test_main.cpp
│   ├── qtest_mainwindow.cpp
│   ├── test_main.cpp
│   ├── test_mainwindow.cpp
│   └── test_resolver.cpp
├── translations
│   ├── MatrixHistory_en.qm
│   ├── MatrixHistory_en.ts
│   ├── MatrixHistory_es.qm
│   ├── MatrixHistory_es.ts
│   ├── MatrixUtility_en.qm
│   ├── MatrixUtility_en.ts
│   ├── MatrixUtility_es.qm
│   ├── MatrixUtility_es.ts
│   ├── PipMatrixResolverQt_en.qm
│   ├── PipMatrixResolverQt_en.ts
│   ├── PipMatrixResolverQt_es.qm
│   └── PipMatrixResolverQt_es.ts
├── .gitignore
├── CMakeLists.txt
├── CMakeLists.txt.user
├── LICENSE
├── PipMatrixResolverQt.qrc
├── README.md
├── cleanbash.sh
├── requirements.txt
├── source_files.txt
├── src2txt.bat
└── src2txt.sh
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

## Terminal Tab — What You Can Do
* Run Python scripts:
    * python myscript.py

* Install a package:
    * pip install package_name

* Uninstall a package:
    * pip uninstall package_name

* List installed packages:
    * pip list

* Show package details:
    * pip show package_name

* Upgrade a package:
    * pip install --upgrade package_name

* Run pip-tools commands:
    * pip-compile requirements.in
    * pip-sync

* Check Python version:
    * python --version

* Check pip version:
    * pip --version

* Run any shell command:
    * On Windows: dir
    * On Linux/macOS: ls

* View contents of a file:
    * On Windows: type filename.txt
    * On Linux/macOS: cat filename.txt

* Check environment variables:
    * On Windows: set
    * On Linux/macOS: env

* Run batch or shell scripts:
    * bash script.sh (Linux/macOS)
    * script.bat (Windows)

* Deactivate the virtual environment (if you started a shell session manually):
    * deactivate

## License: 
    Unlicensed, MIT or your chosen license

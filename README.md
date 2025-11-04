# Pip-Matrix-Resolver
This is a Qt C++ GUI,
that allows you to open requirements.txt
and create a working Python environment terminal.
* It has tabs to show the function it provides.
* It has a Python Terminal to run commands and applications.
* You can create batch files to process Python calls.

A cross-platform Qt C++ GUI for resolving Python requirements.txt matrixes/matrices
to build a Python environment to run Python applications.
and managing virtual environments, 
for running batch commands.

## Use-Case
The best use case for this application,
is when you find it hard to install the Python application.
You get stuck in Dependency Hell.
Every computer OS, release, and verion, can have a different environment.
This application will read a requirements.txt locally, 
or over the web, normally from a GitHub repository.
I assignes a matrix for all packages and their versions.
You can adjust from the base, how much range you want to give it.
By default it has 3 versions, but you can change the levels.

The matrix will read the file requirements.sh, 
and will update the matrix resolver values that resolve. 
It will use the operator == to pin exact version.


The time to run the test will vary, but here is the math.
If you have 18 packages, each with 3 possible versions, 
that gives you 18 independent variables with 3 options each. 
The total number of possible combinations is 
318=387,420,4893^18 = 387,420,489. 
If each test takes 5 seconds, 
running all combinations would require about 1.9 billion seconds, 
or roughly 61.4 years of nonstop testing—assuming no parallelism, 
no caching, and no skipped failures. 
Clearly, exhaustive testing is impractical. 
To optimize, we can use tools like pip‑tools to resolve dependencies up front, 
prune invalid combinations, and lock down a consistent set of requirements, 
dramatically reducing the number of cases we actually need to test.

The reason I wrote this application:

I wrote this to use SadTalker and MuseTalk.
The are Pyton applications that take an image and wav file as arguments.
I will animate the face in the image, to lip sync the wav file.
This is called a talking heads video.
It will allow me to install it and keep it updated on GitHub.
I can create a batch file to process images and wav files into videos.

## Status:
* Not working.
* Open requirements.txt in Local and Web modes works.
* Terminal is in work.
* Just starting this project, and it will take time to get to a working version.

#### Features
* Resolve Python dependency matrices with pip-tools
* Manage virtual environments (create, upgrade pip and pip-tools)
* Batch convert audio + image to MP4 with ffmpeg
* Qt GUI with menus, log view, progress bar
* Translation support (English and Spanish)

## Build Instructions
Use Qt build or:
```
cmake -S . -B build
cmake --build build
```
### Create the installer/package:
* cpack

On Windows, windeployqt is run automatically after build.

## Qt Project Structure
```
/
├── 📂 build
├── 📂 icons
│   ├── 📄 app.svg
│   ├── 📄 batch.svg
│   ├── 📄 cancel.svg
│   ├── 📄 info.svg
│   ├── 📄 open.svg
│   ├── 📄 pause.svg
│   ├── 📄 readme.svg
│   ├── 📄 resolve.svg
│   ├── 📄 resume.svg
│   ├── 📄 stop.svg
│   ├── 📄 url.svg
│   └── 📄 venv.svg
├── 📂 src
│   ├── 📄 MainWindow.cpp
│   ├── 📄 MainWindow.h
│   ├── 📄 CommandsTab.cpp
│   ├── 📄 CommandsTab.h
│   └── 📄 main.cpp
├── 📂 tests
│   ├── 📄 gtest_resolver.cpp
│   ├── 📄 qt_test_main.cpp
│   ├── 📄 qtest_mainwindow.cpp
│   ├── 📄 test_main.cpp
│   ├── 📄 test_mainwindow.cpp
│   └── 📄 test_resolver.cpp
├── 📂 translations
│   ├── 📄 PipMatrixResolverQt_en.qm
│   ├── 📄 PipMatrixResolverQt_en.ts
│   ├── 📄 PipMatrixResolverQt_es.qm
│   └── 📄 PipMatrixResolverQt_es.ts
├── 📄 .gitignore
├── 📄 CMakeLists.txt
├── 📄 LICENSE
├── 📄 PipMatrixResolverQt.qrc
├── 📄 README.md
├── 📄 cleanbash.sh
├── 📄 requirements.txt
├── 📄 src2txt.bat
└── 📄 src2txt.sh
```
## File Descriptions

#### Root
* CMakeLists.txt – Build script: compiles sources, generates qm from ts, bundles resources
* PipMatrixResolverQt.qrc – Qt resource file embedding icons and compiled translations


#### resources/icons
* SVG icons used in menus and toolbars


#### src
* main.cpp – Application entry point. Sets up QApplication, loads translations, shows MainWindow
* MainWindow.h/.cpp – Main GUI window. Defines menus, log view, progress bar, and user actions.
* CommandsTab.h/cpp -

#### translations
* PipMatrixResolverQt_en.ts – English translation source
* PipMatrixResolverQt_es.ts – Spanish translation source
* Both are compiled into qm at build time and embedded

#### Usage

* File → Open requirements file – Load a local requirements.txt
* Tools → Create/Update venv – Create or update a Python virtual environment
* Tools → Resolve matrix – Start iterative resolution of package versions
* Batch → Run batch conversion to mp4 – Combine audio and image into MP4
* Help → About – Show app info

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
* Unlicensed, MIT or your chosen license

## End of Readme

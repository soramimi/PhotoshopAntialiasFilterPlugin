TEMPLATE = lib
TARGET = Antialias
CONFIG += dll c++17
CONFIG -= qt

# Build as DLL with .8bf extension
QMAKE_EXTENSION_SHLIB = 8bf

# Output directories
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../build/Debug
    OBJECTS_DIR = ../build/obj/Debug
} else {
    DESTDIR = $$PWD/../build/Release
    OBJECTS_DIR = ../build/obj/Release
}

# Platform specific settings
win32 {
    DEFINES += ISOLATION_AWARE_ENABLED=1
    DEFINES += _CRT_SECURE_NO_DEPRECATE
    DEFINES += _SCL_SECURE_NO_DEPRECATE
    DEFINES += WIN32=1
    DEFINES += _WINDOWS
    
    CONFIG(release, debug|release) {
        DEFINES += NDEBUG
        DEFINES += NOMINMAX
        QMAKE_CXXFLAGS_RELEASE += /O2 /Oi
    } else {
        DEFINES += _DEBUG
    }
    
    # Runtime library
    CONFIG(release, debug|release) {
        QMAKE_CXXFLAGS_RELEASE -= -MD
        QMAKE_CXXFLAGS_RELEASE += -MT
    } else {
        QMAKE_CXXFLAGS_DEBUG -= -MDd
        QMAKE_CXXFLAGS_DEBUG += -MTd
    }
    
    # Compiler flags
    QMAKE_CXXFLAGS += /W4 /sdl
    
    # Linker settings
    LIBS += kernel32.lib user32.lib gdi32.lib
}

# Include paths
INCLUDEPATH += ../../pluginsdk/samplecode/common/includes
INCLUDEPATH += ../../pluginsdk/photoshopapi/photoshop
INCLUDEPATH += ../../pluginsdk/photoshopapi/pica_sp

# Source files
SOURCES += \
    ../common/PhotoshopAntialiasFilter.cpp \
    ../common/antialias.cpp \
    ../common/euclase.cpp \
    ../common/fp/fp.cpp \
    ../common/fp/tables.cpp

# Header files
HEADERS += \
    ../common/antialias.h \
    ../common/euclase.h \
    ../common/fp/f16c.h \
    ../common/fp/fp.h \
    MyFilter-sym.h

# Resource file
RC_FILE = MyFilter.rc

# Other files (for IDE)
OTHER_FILES += \
    ../common/PiPLs.json \
    README_ja.md

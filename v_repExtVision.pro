QT -= core
QT -= gui

TARGET = v_repExtVision
TEMPLATE = lib

DEFINES -= UNICODE
DEFINES += QT_COMPIL
CONFIG += shared
INCLUDEPATH += "../include"
INCLUDEPATH += "../v_repMath"

*-msvc* {
    QMAKE_CXXFLAGS += -O2
    QMAKE_CXXFLAGS += -W3
}
*-g++* {
    QMAKE_CXXFLAGS += -O3
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -Wno-unused-parameter
    QMAKE_CXXFLAGS += -Wno-strict-aliasing
    QMAKE_CXXFLAGS += -Wno-empty-body
    QMAKE_CXXFLAGS += -Wno-write-strings

    QMAKE_CXXFLAGS += -Wno-unused-but-set-variable
    QMAKE_CXXFLAGS += -Wno-unused-local-typedefs
    QMAKE_CXXFLAGS += -Wno-narrowing

    QMAKE_CFLAGS += -O3
    QMAKE_CFLAGS += -Wall
    QMAKE_CFLAGS += -Wno-strict-aliasing
    QMAKE_CFLAGS += -Wno-unused-parameter
    QMAKE_CFLAGS += -Wno-unused-but-set-variable
    QMAKE_CFLAGS += -Wno-unused-local-typedefs
}


win32 {
    DEFINES += WIN_VREP
}

macx {
    DEFINES += MAC_VREP
}

unix:!macx {
    DEFINES += LIN_VREP
}

SOURCES += \
    v_repExtVision.cpp \
    visionTransf.cpp \
    visionTransfCont.cpp \
    visionVelodyneHDL64E.cpp \
    visionVelodyneHDL64ECont.cpp \
    visionVelodyneVPL16.cpp \
    visionVelodyneVPL16Cont.cpp \
    imageProcess.cpp \
    ../common/scriptFunctionData.cpp \
    ../common/scriptFunctionDataItem.cpp \
    ../common/v_repLib.cpp \
    ../v_repMath/MyMath.cpp \
    ../v_repMath/3Vector.cpp \
    ../v_repMath/4Vector.cpp \
    ../v_repMath/6Vector.cpp \
    ../v_repMath/7Vector.cpp \
    ../v_repMath/3X3Matrix.cpp \
    ../v_repMath/4X4Matrix.cpp \
    ../v_repMath/6X6Matrix.cpp \

HEADERS +=\
    v_repExtVision.h \
    visionTransf.h \
    visionTransfCont.h \
    visionVelodyneHDL64E.h \
    visionVelodyneHDL64ECont.h \
    visionVelodyneVPL16.h \
    visionVelodyneVPL16Cont.h \
    imageProcess.h \
    ../include/scriptFunctionData.h \
    ../include/scriptFunctionDataItem.h \
    ../include/v_repLib.h \
    ../v_repMath/MyMath.h \
    ../v_repMath/mathDefines.h \
    ../v_repMath/3Vector.h \
    ../v_repMath/4Vector.h \
    ../v_repMath/6Vector.h \
    ../v_repMath/7Vector.h \
    ../v_repMath/3X3Matrix.h \
    ../v_repMath/4X4Matrix.h \
    ../v_repMath/6X6Matrix.h \

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}


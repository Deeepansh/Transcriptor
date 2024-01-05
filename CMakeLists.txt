cmake_minimum_required(VERSION 3.16)

project(asr-post-editor)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOUIC_SEARCH_PATHS)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
set(Qt5_DIR "*PATH TO YOUR QT QT5CONFIG FILE HERE*" CACHE PATH "Initial cache" FORCE)
find_package (
    Qt5
    HINTS "$ENV{QTDIR}"
    REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
)
find_package(Qt5PrintSupport REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB MEDIAPLAYER_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.ui")
file(GLOB MEDIAPLAYER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.cpp")
file(GLOB MEDIAPLAYER_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/mediaplayer/*.h")

file(GLOB EDITOR_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.ui")
file(GLOB EDITOR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.cpp")
file(GLOB EDITOR_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.h")
file(GLOB EDITOR_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/editor/*.qrc")

file(GLOB EDITOR_UTILS_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.ui")
file(GLOB EDITOR_UTILS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.cpp")
file(GLOB EDITOR_UTILS_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/editor/utilities/*.h")


file(GLOB PROJECT_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB PROJECT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB PROJECT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB PROJECT_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

set(GUI_TYPE)
if (WIN32)
    set(GUI_TYPE WIN32)
else ()
    set(GUI_TYPE)
endif ()

add_executable(
        ${PROJECT_NAME}
        ${GUI_TYPE} main.cpp

        ${PROJECT_FORMS}
        ${PROJECT_SOURCE}
        ${PROJECT_HEADER}
        ${PROJECT_RESOURCES}


        ${MEDIAPLAYER_FORMS}
        ${MEDIAPLAYER_SOURCE}
        ${MEDIAPLAYER_HEADER}

        ${EDITOR_FORMS}
        ${EDITOR_SOURCE}
        ${EDITOR_HEADER}
        ${EDITOR_RESOURCES}

        ${EDITOR_UTILS_FORMS}
        ${EDITOR_UTILS_SOURCE}
        ${EDITOR_UTILS_HEADER}

        transcriptgenerator.h
        transcriptgenerator.cpp
        editor/utilities/equationeditor.h
        editor/utilities/equationeditor.cpp
        editor/utilities/equationeditor.ui
        mediaplayer/fftw3.h
        mediaplayer/qcustomplot.h
        audiowaveform.h audiowaveform.cpp audiowaveform.ui






)


target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        Qt5::MultimediaWidgets
        Qt5::Network
        Qt5::PrintSupport

        D:/Sem1_PDF/IITB/Integrate2/fftw/libfftw3-3.dll

        D:/Sem1_PDF/IITB/Integrate2/fftw/libfftw3f-3.dll

        D:/Sem1_PDF/IITB/Integrate2/fftw/libfftw3l-3.dll
)

cmake_minimum_required(VERSION 3.16)
project(NotepadPlusPlusAgentLinux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)

set(SCINTILLA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../notepad-plus-plus-upstream/scintilla")
set(LEXILLA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../notepad-plus-plus-upstream/lexilla")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SCINTILLA_DIR}/include
    ${SCINTILLA_DIR}/src
    ${SCINTILLA_DIR}/qt/ScintillaEditBase
    ${LEXILLA_DIR}/include
)

link_directories(
    ${SCINTILLA_DIR}/bin
    ${LEXILLA_DIR}/bin
)

set(SOURCES
    main.cpp
    MainWindow.cpp
    EditorTab.cpp
    AgentBridge.cpp
    LexerManager.cpp
    FunctionListDock.cpp
)

set(HEADERS
    MainWindow.h
    EditorTab.h
    AgentBridge.h
    LexerManager.h
    FunctionListDock.h
)

add_executable(notepadpp-agent
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(notepadpp-agent
    PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    ScintillaEditBase
    lexilla
    dl
    pthread
)

# Definir rpath para encontrar as bibliotecas locais facilmente
set_target_properties(notepadpp-agent PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "${SCINTILLA_DIR}/bin;${LEXILLA_DIR}/bin;$ORIGIN/../lib"
)

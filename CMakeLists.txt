cmake_minimum_required(VERSION 3.16)

project(MyQt6App VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Temukan modul Qt6 yang diperlukan
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    SerialPort
    Network
    Multimedia
    WebSockets
)

# Jika pakai .ui file
qt_standard_project_setup()

# Kumpulkan file sumber
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    qcustomplot.cpp
    qcustomplot.h
)

# Generate executable
qt_add_executable(MyQt6App
    ${SOURCES}
)

# Link Qt modules

# Tambahkan folder tempat qcustomplot berada
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}       # jika qcustomplot ada di root project
)

target_link_libraries(MyQt6App PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
    Qt6::Multimedia
    Qt6::WebSockets
)

# Untuk macOS bundling
if(APPLE)
    set_target_properties(MyQt6App PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.myqt6app"
        MACOSX_BUNDLE_BUNDLE_NAME "MyQt6App"
    )
endif()

cmake_minimum_required(VERSION 3.16)
project(SatelliteTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Quick3D Network)
qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appSatelliteTracker
    main.cpp
    SatelliteTracker.h
    SatelliteTracker.cpp
)

qt_add_qml_module(appSatelliteTracker
    URI SatelliteTracker
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        earth.jpg
        earth_night.jpg
        satellite.png
)

set_target_properties(appSatelliteTracker PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appSatelliteTracker
    PRIVATE
        Qt6::Quick
        Qt6::Quick3D
        Qt6::Network
)

install(TARGETS appSatelliteTracker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
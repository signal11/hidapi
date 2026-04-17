

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(hidapi-qtgui LANGUAGES CXX)

# short-circuit the check in FindWrapOpenGL.cmake, to avoid looking for
# AGL.framework, which is removed from the Apple SDKs now
set(WrapOpenGL_AGL "-framework OpenGL")

find_package(Qt6 COMPONENTS Core Gui Widgets)

if (NOT Qt6_FOUND)
    message(WARNING "Qt6 not found, won't build test GUI")
    return()
endif()

add_executable(hidapi-qtgui hidapigui.cpp)
set_property(TARGET hidapi-qtgui PROPERTY AUTOMOC ON)

target_link_libraries(hidapi-qtgui Qt6::Core Qt6::Gui Qt6::Widgets hidapi::hidapi)


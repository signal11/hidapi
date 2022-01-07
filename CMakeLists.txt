cmake_minimum_required( VERSION 3.9 )

project( hidapi C )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
    set( SRCS
         windows/hid.c
    )
    set( LINK_LIBS
         Setupapi
    )
elseif( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    set( SRCS
         linux/hid.c
    )
else( )
    set( SRCS
         mac/hid.c
    )
endif( )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/hidapi )

add_library( hidapi ${SRCS} )

target_link_libraries( ${PROJECT_NAME} ${LINK_LIBS} )
